#include "OC_apps.h"
#include "OC_digital_inputs.h"
#include "util/util_math.h"
#include "util/util_settings.h"
#include "util_ui.h"

enum EEnvelopeSettings {
  ENV_SETTING_TYPE,
  ENV_SETTING_SEG1_TIME,
  ENV_SETTING_SEG2_TIME,
  ENV_SETTING_SEG3_TIME,
  ENV_SETTING_SEG4_TIME,
  ENV_SETTING_LAST
};

enum EEnvelopeType {
  ENV_TYPE_AD,
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

  int get_segment_time(int segment) const {
    return values_[ENV_SETTING_SEG1_TIME + segment];
  }

  // Utils
  int get_segment_level(int segment) const {
    return (segment & 1) ? 32767 : 0;
  }

  int active_segments() const {
    return 2;
  }

  void ISR() {
  }

};

void EnvelopeGenerator::Init() {
  InitDefaults();
}

const char* const envelope_types[ENV_TYPE_LAST] = {
  "AD", "ADSR"
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

    ui.selected_channel = 0;
    ui.selected_segment = 0;
  }

  void ISR() {
    cv1.push(OC::ADC::value<ADC_CHANNEL_1>());
    cv2.push(OC::ADC::value<ADC_CHANNEL_2>());
    cv3.push(OC::ADC::value<ADC_CHANNEL_3>());
    cv4.push(OC::ADC::value<ADC_CHANNEL_4>());

    // Read digital

    for (auto &env : envelopes_)
      env.ISR();
  }

  struct {
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
  weegfx::coord_t endx = 0, endy = 48;
  for (int segment = 0; segment < env.active_segments(); ++segment) {
    weegfx::coord_t startx = endx;
    weegfx::coord_t starty = endy;
    int value = env.get_segment_time(segment);
    weegfx::coord_t w = 1 + (value >> 3);
    endx += w;
    endy = 48 - (env.get_segment_level(ENV_SETTING_SEG1_TIME + segment) >> 10);

    graphics.drawLine(startx, starty, endx, endy);
    if (segment == envgen.ui.selected_segment) {
      graphics.setPrintPos(startx, 50);
      graphics.print(value);
    }
  }

  GRAPHICS_END_FRAME();
}

void ENVGEN_topButton() {
  auto &selected_env = envgen.selected();
  selected_env.change_value(ENV_SETTING_SEG1_TIME + envgen.ui.selected_segment, 32);
}

void ENVGEN_lowerButton() {
  auto &selected_env = envgen.selected();
  selected_env.change_value(ENV_SETTING_SEG1_TIME + envgen.ui.selected_segment, -32); 
}

void ENVGEN_rightButton() {
  auto const &selected_env = envgen.selected();
  ++envgen.ui.selected_segment;
  CONSTRAIN(envgen.ui.selected_segment, 0, selected_env.active_segments() - 1);
  encoder[RIGHT].setPos(0);
}

void ENVGEN_leftButton() { }
void ENVGEN_leftButtonLong() { }
bool ENVGEN_encoders() {
  bool changed = false;
  long left_value = encoder[LEFT].pos();
  long right_value = encoder[RIGHT].pos();

  if (left_value) {
    left_value += envgen.ui.selected_channel;
    CONSTRAIN(left_value, 0, 3);
    envgen.ui.selected_channel = left_value;
    right_value = 0;
    changed = true;
  }

  if (right_value) {
    auto &selected_env = envgen.selected();
    changed = selected_env.change_value(ENV_SETTING_SEG1_TIME + envgen.ui.selected_segment, right_value);
  }

  encoder[LEFT].setPos(0);
  encoder[RIGHT].setPos(0);
  return changed;
}

void ENVGEN_isr() {
  envgen.ISR();
}
