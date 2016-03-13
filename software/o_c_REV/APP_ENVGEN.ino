#include "OC_apps.h"
#include "OC_bitmaps.h"
#include "OC_digital_inputs.h"
#include "OC_strings.h"
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

enum EnvelopeSettings {
  ENV_SETTING_TYPE,
  ENV_SETTING_SEG1_VALUE,
  ENV_SETTING_SEG2_VALUE,
  ENV_SETTING_SEG3_VALUE,
  ENV_SETTING_SEG4_VALUE,
  ENV_SETTING_TRIGGER_INPUT,
  ENV_SETTING_CV1,
  ENV_SETTING_CV2,
  ENV_SETTING_CV3,
  ENV_SETTING_CV4,
  ENV_SETTING_HARD_RESET,
  ENV_SETTING_ATTACK_SHAPE,
  ENV_SETTING_DECAY_SHAPE,
  ENV_SETTING_RELEASE_SHAPE,
  ENV_SETTING_LAST
};

enum CVMapping {
  CV_MAPPING_NONE,
  CV_MAPPING_SEG1,
  CV_MAPPING_SEG2,
  CV_MAPPING_SEG3,
  CV_MAPPING_SEG4,
  CV_MAPPING_LAST
};

enum EnvelopeType {
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

  void Init(OC::DigitalInput default_trigger);

  EnvelopeType get_type() const {
    return static_cast<EnvelopeType>(values_[ENV_SETTING_TYPE]);
  }

  OC::DigitalInput get_trigger_input() const {
    return static_cast<OC::DigitalInput>(values_[ENV_SETTING_TRIGGER_INPUT]);
  }

  CVMapping get_cv1_mapping() const {
    return static_cast<CVMapping>(values_[ENV_SETTING_CV1]);
  }

  CVMapping get_cv2_mapping() const {
    return static_cast<CVMapping>(values_[ENV_SETTING_CV2]);
  }

  CVMapping get_cv3_mapping() const {
    return static_cast<CVMapping>(values_[ENV_SETTING_CV3]);
  }

  CVMapping get_cv4_mapping() const {
    return static_cast<CVMapping>(values_[ENV_SETTING_CV4]);
  }

  bool get_hard_reset() const {
    return values_[ENV_SETTING_HARD_RESET];
  }

  peaks::EnvelopeShape get_attack_shape() const {
    return static_cast<peaks::EnvelopeShape>(values_[ENV_SETTING_ATTACK_SHAPE]);
  }

  peaks::EnvelopeShape get_decay_shape() const {
    return static_cast<peaks::EnvelopeShape>(values_[ENV_SETTING_DECAY_SHAPE]);
  }

  peaks::EnvelopeShape get_release_shape() const {
    return static_cast<peaks::EnvelopeShape>(values_[ENV_SETTING_RELEASE_SHAPE]);
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

  inline void apply_cv_mapping(EnvelopeSettings cv_setting, const int32_t cvs[ADC_CHANNEL_LAST], int32_t segments[kMaxSegments]) {
    int mapping = values_[cv_setting];
    switch (mapping) {
      case CV_MAPPING_SEG1:
      case CV_MAPPING_SEG2:
      case CV_MAPPING_SEG3:
      case CV_MAPPING_SEG4:
        segments[mapping - CV_MAPPING_SEG1] += (cvs[cv_setting - ENV_SETTING_CV1] * 65536) >> 12;
        break;
      default:
        break;
    }
  }

  template <DAC_CHANNEL dac_channel>
  void Update(uint32_t triggers, const int32_t cvs[ADC_CHANNEL_LAST]) {

    int32_t s[kMaxSegments];
    s[0] = SCALE8_16(static_cast<int32_t>(get_segment_value(0)));
    s[1] = SCALE8_16(static_cast<int32_t>(get_segment_value(1)));
    s[2] = SCALE8_16(static_cast<int32_t>(get_segment_value(2)));
    s[3] = SCALE8_16(static_cast<int32_t>(get_segment_value(3)));

    apply_cv_mapping(ENV_SETTING_CV1, cvs, s);
    apply_cv_mapping(ENV_SETTING_CV2, cvs, s);
    apply_cv_mapping(ENV_SETTING_CV3, cvs, s);
    apply_cv_mapping(ENV_SETTING_CV4, cvs, s);

    s[0] = USAT16(s[0]);
    s[1] = USAT16(s[1]);
    s[2] = USAT16(s[2]);
    s[3] = USAT16(s[3]);

    EnvelopeType type = get_type();
    switch (type) {
      case ENV_TYPE_AD: env_.set_ad(s[0], s[1]); break;
      case ENV_TYPE_ADSR: env_.set_adsr(s[0], s[1], s[2]>>1, s[3]); break;
      case ENV_TYPE_ADR: env_.set_adr(s[0], s[1], s[2]>>1, s[3]); break;
      case ENV_TYPE_AR: env_.set_ar(s[0], s[1]); break;
      case ENV_TYPE_ADSAR: env_.set_adsar(s[0], s[1], s[2]>>1, s[3]); break;
      case ENV_TYPE_ADAR: env_.set_adar(s[0], s[1], s[2]>>1, s[3]); break;
      case ENV_TYPE_AD_LOOP: env_.set_ad_loop(s[0], s[1]); break;
      case ENV_TYPE_ADR_LOOP: env_.set_adr_loop(s[0], s[1], s[2]>>1, s[3]); break;
      case ENV_TYPE_ADAR_LOOP: env_.set_adar_loop(s[0], s[1], s[2]>>1, s[3]); break;
      default:
      break;
    }

    if (type != last_type_) {
      last_type_ = type;
      env_.reset();
    }

    // hard reset forces the envelope to start at level_[0] on rising gate.
    env_.set_hard_reset(get_hard_reset());

    // set the envelope segment shapes
    env_.set_attack_shape(get_attack_shape());
    env_.set_decay_shape(get_decay_shape());
    env_.set_release_shape(get_release_shape());

    OC::DigitalInput trigger_input = get_trigger_input();
    uint8_t gate_state = 0;
    if (triggers & DIGITAL_INPUT_MASK(trigger_input))
      gate_state |= peaks::CONTROL_GATE_RISING;

    bool gate_raised = OC::DigitalInputs::read_immediate(trigger_input);
    if (gate_raised)
      gate_state |= peaks::CONTROL_GATE;
    else if (gate_raised_)
      gate_state |= peaks::CONTROL_GATE_FALLING;
    gate_raised_ = gate_raised;

    // TODO Scale range or offset?
    uint32_t value = OC::calibration_data.dac.octaves[_ZERO] + env_.ProcessSingleSample(gate_state);
    DAC::set<dac_channel>(value);
  }

  size_t render_preview(int16_t *values, uint32_t *segments, uint32_t *loops, uint32_t &current_phase) const {
    return env_.render_preview(values, segments, loops, current_phase);
  }

private:
  peaks::MultistageEnvelope env_;
  EnvelopeType last_type_;
  bool gate_raised_;
};

void EnvelopeGenerator::Init(OC::DigitalInput default_trigger) {
  InitDefaults();
  apply_value(ENV_SETTING_TRIGGER_INPUT, default_trigger);
  env_.Init();
  last_type_ = ENV_TYPE_LAST;
  gate_raised_ = false;
}

const char* const envelope_types[ENV_TYPE_LAST] = {
  "AD", "ADSR", "ADR", "AR", "ADSAR", "ADAR", "AD_LOOP", "ADR_LOOP", "ADAR_LOOP"
};

const char* const envelope_shapes[peaks::ENV_SHAPE_LAST] = {
  "Lin", "Exp", "Quart", "Sine", "Ledge", "Cliff", "BgDip", "MeDip", "LtDip", "Wiggl"
};

const char* const cv_mapping_names[CV_MAPPING_LAST] = {
  "off", "S1", "S2", "S3", "S4"
};

SETTINGS_DECLARE(EnvelopeGenerator, ENV_SETTING_LAST) {
  { ENV_TYPE_AD, ENV_TYPE_FIRST, ENV_TYPE_LAST-1, "TYPE", envelope_types, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "S1", NULL, settings::STORAGE_TYPE_U16 }, // u16 in case resolution proves insufficent
  { 128, 0, 255, "S2", NULL, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "S3", NULL, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "S4", NULL, settings::STORAGE_TYPE_U16 },
  { OC::DIGITAL_INPUT_1, OC::DIGITAL_INPUT_1, OC::DIGITAL_INPUT_4, "Trigger input", OC::Strings::trigger_input_names, settings::STORAGE_TYPE_U8 },
  { CV_MAPPING_NONE, CV_MAPPING_NONE, CV_MAPPING_SEG4, "CV1 -> ", cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { CV_MAPPING_NONE, CV_MAPPING_NONE, CV_MAPPING_SEG4, "CV2 -> ", cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { CV_MAPPING_NONE, CV_MAPPING_NONE, CV_MAPPING_SEG4, "CV3 -> ", cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { CV_MAPPING_NONE, CV_MAPPING_NONE, CV_MAPPING_SEG4, "CV4 -> ", cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { 0, 0, 1, "Hard reset", OC::Strings::no_yes, settings::STORAGE_TYPE_U4 },
  { peaks::ENV_SHAPE_QUARTIC, peaks::ENV_SHAPE_LINEAR, peaks::ENV_SHAPE_LAST - 1, "Attack shape", envelope_shapes, settings::STORAGE_TYPE_U4 },
  { peaks::ENV_SHAPE_EXPONENTIAL, peaks::ENV_SHAPE_LINEAR, peaks::ENV_SHAPE_LAST - 1, "Decay shape", envelope_shapes, settings::STORAGE_TYPE_U4 },
  { peaks::ENV_SHAPE_EXPONENTIAL, peaks::ENV_SHAPE_LINEAR, peaks::ENV_SHAPE_LAST - 1, "Release shape", envelope_shapes, settings::STORAGE_TYPE_U4 },
};

class QuadEnvelopeGenerator {
public:
  static constexpr int32_t kCvSmoothing = 16;

  void Init() {
    int input = OC::DIGITAL_INPUT_1;
    for (auto &env : envelopes_) {
      env.Init(static_cast<OC::DigitalInput>(input));
      ++input;
    }

    ui.left_encoder_value = 0;
    ui.left_edit_mode = MODE_SELECT_CHANNEL;
    ui.editing = false;
    ui.selected_channel = 0;
    ui.selected_segment = 0;
    ui.selected_setting = ENV_SETTING_TRIGGER_INPUT;
  }

  void ISR() {
    cv1.push(OC::ADC::value<ADC_CHANNEL_1>());
    cv2.push(OC::ADC::value<ADC_CHANNEL_2>());
    cv3.push(OC::ADC::value<ADC_CHANNEL_3>());
    cv4.push(OC::ADC::value<ADC_CHANNEL_4>());

    const int32_t cvs[ADC_CHANNEL_LAST] = { cv1.value(), cv2.value(), cv3.value(), cv4.value() };
    uint32_t triggers = OC::DigitalInputs::clocked();

    envelopes_[0].Update<DAC_CHANNEL_A>(triggers, cvs);
    envelopes_[1].Update<DAC_CHANNEL_B>(triggers, cvs);
    envelopes_[2].Update<DAC_CHANNEL_C>(triggers, cvs);
    envelopes_[3].Update<DAC_CHANNEL_D>(triggers, cvs);
  }

  enum LeftEditMode {
    MODE_SELECT_CHANNEL,
    MODE_EDIT_SETTINGS
  };

  struct {
    LeftEditMode left_edit_mode;
    int left_encoder_value;
    bool editing;

    int selected_channel;
    int selected_segment;
    int selected_setting;
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

void ENVGEN_menu_preview() {
  auto const &env = envgen.selected();
  UI_START_MENU(0);

  graphics.print(EnvelopeGenerator::value_attr(ENV_SETTING_TYPE).value_names[env.get_type()]);
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
}

void ENVGEN_menu_settings() {
  auto const &env = envgen.selected();
  UI_START_MENU(0);
  if (env.get_type() == envgen.ui.left_encoder_value)
    graphics.drawBitmap8(2, y + 1, 4, OC::bitmap_indicator_4x8);
  graphics.print(' ');
  graphics.print(EnvelopeGenerator::value_attr(ENV_SETTING_TYPE).value_names[envgen.ui.left_encoder_value]);

  int first_visible_param = envgen.ui.selected_setting - 2;
  if (first_visible_param < ENV_SETTING_TRIGGER_INPUT)
    first_visible_param = ENV_SETTING_TRIGGER_INPUT;

  UI_BEGIN_ITEMS_LOOP(0, first_visible_param, ENV_SETTING_LAST, envgen.ui.selected_setting, 1);
    UI_DRAW_EDITABLE(envgen.ui.editing);
    UI_DRAW_SETTING(EnvelopeGenerator::value_attr(current_item), env.get_value(current_item), kUiWideMenuCol1X);
  UI_END_ITEMS_LOOP();
}

void ENVGEN_menu() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem
  static const weegfx::coord_t kStartX = 0;
  UI_DRAW_TITLE(kStartX);

  for (int i = 0, x = 0; i < 4; ++i, x += 32) {
    graphics.setPrintPos(x + 6, 2);
    graphics.print((char)('A' + i));
    graphics.setPrintPos(x + 14, 2);
    if (i == envgen.ui.selected_channel) {
      graphics.invertRect(x, 0, 32, 11);
    }
  }

  if (QuadEnvelopeGenerator::MODE_SELECT_CHANNEL == envgen.ui.left_edit_mode)
    ENVGEN_menu_preview();
  else
    ENVGEN_menu_settings();

  // TODO Draw phase anyway?
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

  if (QuadEnvelopeGenerator::MODE_SELECT_CHANNEL == envgen.ui.left_edit_mode) {
    auto const &selected_env = envgen.selected();
    int segment = envgen.ui.selected_segment + 1;
    if (segment > selected_env.active_segments() - 1)
      segment = 0;
    if (segment != envgen.ui.selected_segment) {
      envgen.ui.selected_segment = segment;
      encoder[RIGHT].setPos(0);
    }
  } else {
    envgen.ui.editing = !envgen.ui.editing;
  }
}

void ENVGEN_leftButton() {
  if (QuadEnvelopeGenerator::MODE_EDIT_SETTINGS == envgen.ui.left_edit_mode) {
    envgen.selected().apply_value(ENV_SETTING_TYPE, envgen.ui.left_encoder_value);
    envgen.ui.left_edit_mode = QuadEnvelopeGenerator::MODE_SELECT_CHANNEL;
  } else {
    envgen.ui.left_edit_mode = QuadEnvelopeGenerator::MODE_EDIT_SETTINGS;
    envgen.ui.left_encoder_value = envgen.selected().get_type();
  }
  encoder[LEFT].setPos(0);
  encoder[RIGHT].setPos(0);
}

void ENVGEN_leftButtonLong() { }
bool ENVGEN_encoders() {
  long left_value = encoder[LEFT].pos();
  long right_value = encoder[RIGHT].pos();
  bool changed = left_value || right_value;

  if (QuadEnvelopeGenerator::MODE_SELECT_CHANNEL == envgen.ui.left_edit_mode) {
    if (left_value) {
      left_value += envgen.ui.selected_channel;
      CONSTRAIN(left_value, 0, 3);
      envgen.ui.selected_channel = left_value;
    }
    if (right_value) {
      auto &selected_env = envgen.selected();
      selected_env.change_value(ENV_SETTING_SEG1_VALUE + envgen.ui.selected_segment, right_value);
    }
  } else {
    if (left_value) {
      left_value += envgen.ui.left_encoder_value;
      CONSTRAIN(left_value, ENV_TYPE_FIRST, ENV_TYPE_LAST - 1);
      envgen.ui.left_encoder_value = left_value;
    }
    if (right_value) {
      if (envgen.ui.editing) {
        auto &selected_env = envgen.selected();
        selected_env.change_value(envgen.ui.selected_setting, right_value);
      } else {
        right_value += envgen.ui.selected_setting;
        CONSTRAIN(right_value, ENV_SETTING_TRIGGER_INPUT, ENV_SETTING_LAST - 1);
        envgen.ui.selected_setting = right_value;
      }
    }
  }

  encoder[LEFT].setPos(0);
  encoder[RIGHT].setPos(0);
  return changed;
}

void ENVGEN_screensaver() {
  GRAPHICS_BEGIN_FRAME(false);
  scope_render();
  GRAPHICS_END_FRAME();
}

void FASTRUN ENVGEN_isr() {
  envgen.ISR();
}
