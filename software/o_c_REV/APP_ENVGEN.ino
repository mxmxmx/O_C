#include "OC_apps.h"
#include "OC_digital_inputs.h"
#include "util/util_math.h"
#include "util/util_settings.h"
#include "util_ui.h"

enum EEnvelopeSettings {
  ENV_SETTING_TYPE,
  ENV_SETTING_SEG1_VALUE,
  ENV_SETTING_SEG2_VALUE,
  ENV_SETTING_LAST
};

enum EEnvelopeType {
  ENV_TYPE_AD,
  ENV_TYPE_LAST
};

class EnvelopeGenerator : public settings::SettingsBase<EnvelopeGenerator, ENV_SETTING_LAST> {
public:

  static constexpr int kMaxSegments = 4;

  void Init();

  int active_segments() const {
    return 2;
  }

  int get_level(int segment) const {
    return (segment & 1) ? 32767 : 0;
  }

  static constexpr int32_t kSmoothing = 16;
  SmoothedValue<int32_t, kSmoothing> cv1;
  SmoothedValue<int32_t, kSmoothing> cv2;
  SmoothedValue<int32_t, kSmoothing> cv3;
  SmoothedValue<int32_t, kSmoothing> cv4;
};

void EnvelopeGenerator::Init() {
  InitDefaults();
}

SETTINGS_DECLARE(EnvelopeGenerator, ENV_SETTING_LAST) {
  { ENV_TYPE_AD, ENV_TYPE_AD, ENV_TYPE_AD, "TYPE", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "A", NULL, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "D", NULL, settings::STORAGE_TYPE_U16 }
};

struct QuadEnvelopeGenerator {

  void Init() {
    for (auto &env : envelopes_)
      env.Init();

    ui.selected_channel = 0;
    ui.selected_segment = 0;
  }

  void ISR() {
  }

  EnvelopeGenerator envelopes_[4];

  struct {
    int selected_channel;
    int selected_segment;
  } ui;

  EnvelopeGenerator &selected() {
    return envelopes_[ui.selected_channel];
  }
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
    int value = env.get_value(ENV_SETTING_SEG1_VALUE + segment);
    weegfx::coord_t w = 1 + (value >> 3);
    endx += w;
    endy = 48 - (env.get_level(ENV_SETTING_SEG1_VALUE + segment) >> 10);

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
  selected_env.change_value(ENV_SETTING_SEG1_VALUE + envgen.ui.selected_segment, 32);
}

void ENVGEN_lowerButton() {
  auto &selected_env = envgen.selected();
  selected_env.change_value(ENV_SETTING_SEG1_VALUE + envgen.ui.selected_segment, -32); 
}

void ENVGEN_rightButton() {
  auto const &selected_env = envgen.selected();
  if (envgen.ui.selected_segment < selected_env.active_segments() - 1) {
    ++envgen.ui.selected_segment;
  } else {
    envgen.ui.selected_segment = 0;
  }
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
    if (left_value < 0) left_value = 0;
    else if (left_value > 3) left_value = 3;
    envgen.ui.selected_channel = left_value;
    right_value = 0;
    changed = true;
  }

  if (right_value) {
    auto &selected_env = envgen.selected();
    changed = selected_env.change_value(ENV_SETTING_SEG1_VALUE + envgen.ui.selected_segment, right_value);
  }

  encoder[LEFT].setPos(0);
  encoder[RIGHT].setPos(0);
  return changed;
}

void ENVGEN_isr() {
  envgen.ISR();
}
