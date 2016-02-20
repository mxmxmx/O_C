#include "OC_apps.h"
#include "OC_digital_inputs.h"
#include "util/util_math.h"
#include "util/util_settings.h"
#include "util_ui.h"
#include "peaks_multistage_envelope.h"

// peaks::MultistageEnvelope allow setting of more parameters per stage, but
// that will involve more editing code, so keeping things simple for now
// with one value per stage.

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
  ENV_TYPE_AR,
  ENV_TYPE_ASDR,
  ENV_TYPE_LAST
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
        return 2;
      case ENV_TYPE_ASDR:
        return 4;
      default: break;
    }
    return 0;
  }

  template <DAC_CHANNEL dac_channel>
  void Update() {
    uint16_t s1 = USAT16(SCALE8_16(static_cast<uint32_t>(get_segment_value(0))));
    uint16_t s2 = USAT16(SCALE8_16(static_cast<uint32_t>(get_segment_value(1))));
    uint16_t s3 = USAT16(SCALE8_16(static_cast<uint32_t>(get_segment_value(2))));
    uint16_t s4 = USAT16(SCALE8_16(static_cast<uint32_t>(get_segment_value(3))));

    EEnvelopeType type = get_type();
    switch (type) {
      case ENV_TYPE_AD: env_.set_ad(s1, s2); break;
      case ENV_TYPE_AR: env_.set_ar(s1, s1); break;
      case ENV_TYPE_ASDR: env_.set_adsr(s1, s2, s3>>1, s4); break;
      default:
        break;
    }
    if (type != last_type_) {
      last_type_ = type;
      env_.set_hard_reset(true);
    }

/*
    // Update env if necessary
    // generate gates

    uint8_t control_flags = 0;
    uint16_t value = env_.ProcessSingleSample(control_flags);

    // scale range? Offset to 0
    DAC::set<dac_channel>(value);
*/
  }

  size_t render_preview(int16_t *values, uint32_t *segments) const {
    return env_.render_preview(values, segments);
  }

private:
  peaks::MultistageEnvelope env_;
  EEnvelopeType last_type_;
};

void EnvelopeGenerator::Init() {
  InitDefaults();
  env_.Init();
  last_type_ = ENV_TYPE_LAST;
}

const char* const envelope_types[ENV_TYPE_LAST] = {
  "AD", "AR", "ADSR"
};

SETTINGS_DECLARE(EnvelopeGenerator, ENV_SETTING_LAST) {
  { ENV_TYPE_AD, ENV_TYPE_AD, ENV_TYPE_LAST-1, "TYPE", envelope_types, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "S1T", NULL, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "S2T", NULL, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "S3T", NULL, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "S4T", NULL, settings::STORAGE_TYPE_U16 }
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

    envelopes_[0].Update<DAC_CHANNEL_A>();
    envelopes_[1].Update<DAC_CHANNEL_B>();
    envelopes_[2].Update<DAC_CHANNEL_C>();
    envelopes_[3].Update<DAC_CHANNEL_D>();
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

  static weegfx::coord_t pt = 0;
  weegfx::coord_t x = 0;
  size_t w = env.render_preview(preview_values, preview_segments);
  int16_t *data = preview_values;
  while (w--) {
    graphics.setPixel(x++, 61 - (*data++ >> 10));
  }

  uint32_t *segments = preview_segments;
  while (*segments != 0xffffffff) {
    weegfx::coord_t x = *segments++;
    weegfx::coord_t value = preview_values[x] >> 10;
    graphics.drawVLine(x, 61 - value, value);
  }

  const int selected_segment = envgen.ui.selected_segment;
  graphics.setPrintPos(2 + preview_segments[selected_segment], y);
  graphics.print(env.get_segment_value(selected_segment));

  // test "fps" indicator
  graphics.drawHLine(0, 63, pt);
  pt = (pt + 1) & 127;

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
      CONSTRAIN(left_value, ENV_TYPE_AD, ENV_TYPE_LAST - 1);
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
