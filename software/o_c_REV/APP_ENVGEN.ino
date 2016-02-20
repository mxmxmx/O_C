#include "OC_apps.h"
#include "OC_digital_inputs.h"
#include "util/util_math.h"
#include "util/util_settings.h"
#include "util_ui.h"
#include "peaks_multistage_envelope.h"

// peaks::MultistageEnvelope allow setting of more parameters per stage, but
// that will involve more editing code, so keeping things simple for now
// with one value per stage.
//
// MultistageEnvelope maps times to lut_env_increments directly, so only 256 discrete values (no interpolation)
// Levels are 0-32767 to be positive on Peaks' bipolar output

enum EEnvelopeSettings {
  ENV_SETTING_TYPE,
  ENV_SETTING_SEG1_VALUE,
  ENV_SETTING_SEG2_VALUE,
  ENV_SETTING_SEG3_VALUE,
  ENV_SETTING_SEG4_VALUE,
  ENV_SETTING_LAST
};

enum EEnvelopeType {
  ENV_TYPE_AD,
  ENV_TYPE_ADSR,
  ENV_TYPE_ADR,
  ENV_TYPE_AR,
  ENV_TYPE_ADSAR,
  ENV_TYPE_ADAR,
  ENV_TYPE_AD_LOOP,
  ENV_TYPE_ADR_LOOP,
  ENV_TYPE_ADAR_LOOP,
  ENV_TYPE_LAST, ENV_TYPE_FIRST = ENV_TYPE_AD
};

class EnvelopeGenerator : public settings::SettingsBase<EnvelopeGenerator, ENV_SETTING_LAST> {
public:

  static constexpr int kMaxSegments = 4;

  void Init();

  EEnvelopeType get_type() const {
    return static_cast<EEnvelopeType>(values_[ENV_SETTING_TYPE]);
  }

  // Utils
  uint16_t get_segment_value(int segment) const {
    return values_[ENV_SETTING_SEG1_VALUE + segment];
  }

  int active_segments() const {
    switch (get_type()) {
      case ENV_TYPE_AD:
      case ENV_TYPE_AR:
      case ENV_TYPE_AD_LOOP:
        return 2;
      case ENV_TYPE_ADR:
      case ENV_TYPE_ADSR:
      case ENV_TYPE_ADSAR:
      case ENV_TYPE_ADAR:
      case ENV_TYPE_ADR_LOOP:
      case ENV_TYPE_ADAR_LOOP:
        return 4;
      default: break;
    }
    return 0;
  }

  template <OC::DigitalInput gate_input, DAC_CHANNEL dac_channel>
  void Update() {
    uint16_t s1 = USAT16(SCALE8_16(static_cast<uint32_t>(get_segment_value(0))));
    uint16_t s2 = USAT16(SCALE8_16(static_cast<uint32_t>(get_segment_value(1))));
    uint16_t s3 = USAT16(SCALE8_16(static_cast<uint32_t>(get_segment_value(2))));
    uint16_t s4 = USAT16(SCALE8_16(static_cast<uint32_t>(get_segment_value(3))));

    EEnvelopeType type = get_type();
    switch (type) {
      case ENV_TYPE_AD: env_.set_ad(s1, s2); break;
      case ENV_TYPE_ADSR: env_.set_adsr(s1, s2, s3>>1, s4); break;
      case ENV_TYPE_ADR: env_.set_adr(s1, s2, s3>>1, s4); break;
      case ENV_TYPE_AR: env_.set_ar(s1, s2); break;
      case ENV_TYPE_ADSAR: env_.set_adsar(s1, s2, s3>>1, s4); break;
      case ENV_TYPE_ADAR: env_.set_adar(s1, s2, s3>>1, s4); break;
      case ENV_TYPE_AD_LOOP: env_.set_ad_loop(s1, s2); break;
      case ENV_TYPE_ADR_LOOP: env_.set_adr_loop(s1, s2, s3>>1, s4); break;
      case ENV_TYPE_ADAR_LOOP: env_.set_adar_loop(s1, s2, s3>>1, s4); break;
      default:
      break;
    }

    if (type != last_type_) {
      last_type_ = type;
      env_.reset();
      env_.set_hard_reset(true);
    }

    uint8_t gate_state = 0;
    if (OC::DigitalInputs::clocked<gate_input>())
      gate_state |= peaks::CONTROL_GATE_RISING;

    bool gate_raised = OC::DigitalInputs::read_immediate<gate_input>();
    if (gate_raised)
      gate_state |= peaks::CONTROL_GATE;
    else if (gate_raised_)
      gate_state |= peaks::CONTROL_GATE_FALLING;
    gate_raised_ = gate_raised;

    // TODO Scale range or offset?
    uint32_t value = OC::calibration_data.octaves[_ZERO] + env_.ProcessSingleSample(gate_state);
    DAC::set<dac_channel>(value);
  }

  size_t render_preview(int16_t *values, uint32_t *segments, uint32_t *loops, uint32_t &current_phase) const {
    return env_.render_preview(values, segments, loops, current_phase);
  }

private:
  peaks::MultistageEnvelope env_;
  EEnvelopeType last_type_;
  bool gate_raised_;
};

void EnvelopeGenerator::Init() {
  InitDefaults();
  env_.Init();
  last_type_ = ENV_TYPE_LAST;
  gate_raised_ = false;
}

const char* const envelope_types[ENV_TYPE_LAST] = {
  "AD", "ADSR", "ADR", "AR", "ADSAR", "ADAR", "AD_LOOP", "ADR_LOOP", "ADAR_LOOP"
};

SETTINGS_DECLARE(EnvelopeGenerator, ENV_SETTING_LAST) {
  { ENV_TYPE_AD, ENV_TYPE_FIRST, ENV_TYPE_LAST-1, "TYPE", envelope_types, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "S1", NULL, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "S2", NULL, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "S3", NULL, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "S4", NULL, settings::STORAGE_TYPE_U16 }
};

class QuadEnvelopeGenerator {
public:
  static constexpr int32_t kCvSmoothing = 16;

  void Init() {
    for (auto &env : envelopes_)
      env.Init();

    ui.left_encoder_value = 0;
    ui.left_encoder_mode = MODE_SELECT_CHANNEL;
    ui.selected_channel = 0;
    ui.selected_segment = 0;
  }

  void ISR() {
    cv1.push(OC::ADC::value<ADC_CHANNEL_1>());
    cv2.push(OC::ADC::value<ADC_CHANNEL_2>());
    cv3.push(OC::ADC::value<ADC_CHANNEL_3>());
    cv4.push(OC::ADC::value<ADC_CHANNEL_4>());

    // Read digital

    envelopes_[0].Update<OC::DIGITAL_INPUT_1, DAC_CHANNEL_A>();
    envelopes_[1].Update<OC::DIGITAL_INPUT_2, DAC_CHANNEL_B>();
    envelopes_[2].Update<OC::DIGITAL_INPUT_3, DAC_CHANNEL_C>();
    envelopes_[3].Update<OC::DIGITAL_INPUT_4, DAC_CHANNEL_D>();
  }

  enum LeftEncoderMode {
    MODE_SELECT_CHANNEL,
    MODE_EDIT_TYPE
  };

  struct {
    LeftEncoderMode left_encoder_mode;
    int left_encoder_value;
    int selected_channel;
    int selected_segment;
  } ui;

  EnvelopeGenerator &selected() {
    return envelopes_[ui.selected_channel];
  }

  EnvelopeGenerator envelopes_[4];

  SmoothedValue<int32_t, kCvSmoothing> cv1;
  SmoothedValue<int32_t, kCvSmoothing> cv2;
  SmoothedValue<int32_t, kCvSmoothing> cv3;
  SmoothedValue<int32_t, kCvSmoothing> cv4;
};

QuadEnvelopeGenerator envgen;

void ENVGEN_init() {
  envgen.Init();
}

size_t ENVGEN_storageSize() {
  return 4 * EnvelopeGenerator::storageSize();
}

size_t ENVGEN_save(void *storage) {
  size_t s = 0;
  for (auto &env : envgen.envelopes_)
    s += env.Save(static_cast<byte *>(storage) + s);
  return s;
}

size_t ENVGEN_restore(const void *storage) {
  size_t s = 0;
  for (auto &env : envgen.envelopes_)
    s += env.Restore(static_cast<const byte *>(storage) + s);
  return s;
}

void ENVGEN_handleEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      encoder[LEFT].setPos(0);
      encoder[RIGHT].setPos(0);
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER:
      break;
  }
}

void ENVGEN_loop() {
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  buttons(BUTTON_TOP);
  buttons(BUTTON_BOTTOM);
  buttons(BUTTON_LEFT);
  buttons(BUTTON_RIGHT);
}

int16_t preview_values[128];
uint32_t preview_segments[peaks::kMaxNumSegments];
uint32_t preview_loops[peaks::kMaxNumSegments];

void ENVGEN_menu() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem
  static const weegfx::coord_t kStartX = 0;
  UI_DRAW_TITLE(kStartX);

  for (int i = 0, x = 0; i < 4; ++i, x += 32) {
    graphics.setPrintPos(x + 4, 2);
    graphics.print((char)('A' + i));
    graphics.setPrintPos(x + 14, 2);
    if (i == envgen.ui.selected_channel) {
      graphics.invertRect(x, 0, 32, 11);
    }
  }

  auto const &env = envgen.selected();

  UI_START_MENU(0);
  if (QuadEnvelopeGenerator::MODE_EDIT_TYPE == envgen.ui.left_encoder_mode) {
    if (env.get_type() == envgen.ui.left_encoder_value)
      graphics.print('>');
    else
      graphics.print(' ');
    graphics.print(EnvelopeGenerator::value_attr(ENV_SETTING_TYPE).value_names[envgen.ui.left_encoder_value]);
    graphics.invertRect(0, y, kUiDisplayWidth, kUiLineH - 1); \
  } else {
    graphics.print(EnvelopeGenerator::value_attr(ENV_SETTING_TYPE).value_names[env.get_type()]);
  }
  y += kUiLineH;

  uint32_t current_phase = 0;
  weegfx::coord_t x = 0;
  size_t w = env.render_preview(preview_values, preview_segments, preview_loops, current_phase);
  int16_t *data = preview_values;
  while (w--) {
    graphics.setPixel(x++, 58 - (*data++ >> 10));
  }

  uint32_t *segments = preview_segments;
  while (*segments != 0xffffffff) {
    weegfx::coord_t x = *segments++;
    weegfx::coord_t value = preview_values[x] >> 10;
    graphics.drawVLine(x, 58 - value, value);
  }

  uint32_t *loops = preview_loops;
  if (*loops != 0xffffffff) {
    weegfx::coord_t start = *loops++;
    weegfx::coord_t end = *loops++;
    graphics.setPixel(start, 60);
    graphics.setPixel(end, 60);
    graphics.drawHLine(start, 61, end - start + 1);
  }

  const int selected_segment = envgen.ui.selected_segment;
  graphics.setPrintPos(2 + preview_segments[selected_segment], y);
  graphics.print(env.get_segment_value(selected_segment));

  graphics.drawHLine(0, 63, current_phase);

  GRAPHICS_END_FRAME();
}

void ENVGEN_topButton() {
  auto &selected_env = envgen.selected();
  selected_env.change_value(ENV_SETTING_SEG1_VALUE + envgen.ui.selected_segment, 32);
}

void ENVGEN_lowerButton() {
  auto &selected_env = envgen.selected();
  selected_env.change_value(ENV_SETTING_SEG1_VALUE + envgen.ui.selected_segment, -32); 
}

void ENVGEN_rightButton() {
  auto const &selected_env = envgen.selected();
  int segment = envgen.ui.selected_segment + 1;
  if (segment > selected_env.active_segments() - 1)
    segment = 0;
  if (segment != envgen.ui.selected_segment) {
    envgen.ui.selected_segment = segment;
    encoder[RIGHT].setPos(0);
  }
}

void ENVGEN_leftButton() {
  if (QuadEnvelopeGenerator::MODE_EDIT_TYPE == envgen.ui.left_encoder_mode) {
    envgen.selected().apply_value(ENV_SETTING_TYPE, envgen.ui.left_encoder_value);
    envgen.ui.left_encoder_mode = QuadEnvelopeGenerator::MODE_SELECT_CHANNEL;
  } else {
    envgen.ui.left_encoder_mode = QuadEnvelopeGenerator::MODE_EDIT_TYPE;
    envgen.ui.left_encoder_value = envgen.selected().get_type();
  }
  encoder[LEFT].setPos(0);
}

void ENVGEN_leftButtonLong() { }
bool ENVGEN_encoders() {
  bool changed = false;
  long left_value = encoder[LEFT].pos();
  long right_value = encoder[RIGHT].pos();

  if (left_value) {
    if (QuadEnvelopeGenerator::MODE_SELECT_CHANNEL == envgen.ui.left_encoder_mode) {
      left_value += envgen.ui.selected_channel;
      CONSTRAIN(left_value, 0, 3);
      envgen.ui.selected_channel = left_value;
    } else {
      left_value += envgen.ui.left_encoder_value;
      CONSTRAIN(left_value, ENV_TYPE_FIRST, ENV_TYPE_LAST - 1);
      envgen.ui.left_encoder_value = left_value;
    }
    right_value = 0;
    changed = true;
  }

  if (right_value) {
    auto &selected_env = envgen.selected();
    selected_env.change_value(ENV_SETTING_SEG1_VALUE + envgen.ui.selected_segment, right_value);
    changed = true;
  }

  encoder[LEFT].setPos(0);
  encoder[RIGHT].setPos(0);
  return changed;
}

void FASTRUN ENVGEN_isr() {
  envgen.ISR();
}
