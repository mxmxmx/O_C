#include "OC_bitmaps.h"
#include "OC_strings.h"
#include "tonnetz/tonnetz_state.h"
#include "util/util_settings.h"
#include "util/util_ringbuffer.h"

// NOTE: H1200 state is updated in the ISR, and we're accessing shared state
// (e.g. outputs) without any sync mechanism. So there is a chance of the
// display being slightly inconsistent but the risk seems acceptable.
// Similarly for changing settings, but this should also be safe(ish) since
// - Each setting should be atomic (int)
// - Changing more than one settings happens seldomly
// - Settings aren't modified in the ISR

enum OutputMode {
  OUTPUT_CHORD_VOICING,
  OUTPUT_TUNE,
  OUTPUT_MODE_LAST
};

enum TransformPriority {
  TRANSFORM_PRIO_XPLR,
  TRANSFORM_PRIO_XRPL,
  TRANSFORM_PRIO_LAST
};

enum H1200Setting {
  H1200_SETTING_ROOT_OFFSET,
  H1200_SETTING_MODE,
  H1200_SETTING_INVERSION,
  H1200_SETTING_TRANFORM_PRIO,
  H1200_SETTING_OUTPUT_MODE,
  H1200_SETTING_LAST
};

class H1200Settings : public settings::SettingsBase<H1200Settings, H1200_SETTING_LAST> {
public:

  int root_offset() const {
    return values_[H1200_SETTING_ROOT_OFFSET];
  }

  EMode mode() const {
    return static_cast<EMode>(values_[H1200_SETTING_MODE]);
  }

  int inversion() const {
    return values_[H1200_SETTING_INVERSION];
  }

  TransformPriority get_transform_priority() const {
    return static_cast<TransformPriority>(values_[H1200_SETTING_TRANFORM_PRIO]);
  }

  OutputMode output_mode() const {
    return static_cast<OutputMode>(values_[H1200_SETTING_OUTPUT_MODE]);
  }

  void Init() {
    InitDefaults();
  }
};

const char * const output_mode_names[] = {
  "chord",
  "tune"
};

const char * const trigger_mode_names[] = {
  "P>L>R",
  "R>P>L",
};

const char * const mode_names[] = {
  "maj", "min"
};

SETTINGS_DECLARE(H1200Settings, H1200_SETTING_LAST) {
  {12, -24, 36, "transpose", NULL, settings::STORAGE_TYPE_I8},
  {MODE_MAJOR, 0, MODE_LAST-1, "mode", mode_names, settings::STORAGE_TYPE_U8},
  {0, -3, 3, "inversion", NULL, settings::STORAGE_TYPE_I8},
  {TRANSFORM_PRIO_XPLR, 0, TRANSFORM_PRIO_LAST-1, "priority", trigger_mode_names, settings::STORAGE_TYPE_U8},
  {OUTPUT_CHORD_VOICING, 0, OUTPUT_MODE_LAST-1, "output", output_mode_names, settings::STORAGE_TYPE_U8}
};

static constexpr uint32_t TRIGGER_MASK_TR1 = OC::DIGITAL_INPUT_1_MASK;
static constexpr uint32_t TRIGGER_MASK_P = OC::DIGITAL_INPUT_2_MASK;
static constexpr uint32_t TRIGGER_MASK_L = OC::DIGITAL_INPUT_3_MASK;
static constexpr uint32_t TRIGGER_MASK_R = OC::DIGITAL_INPUT_4_MASK;
static constexpr uint32_t TRIGGER_MASK_DIRTY = 0x10;
static constexpr uint32_t TRIGGER_MASK_RESET = TRIGGER_MASK_TR1 | TRIGGER_MASK_DIRTY;

namespace H1200 {
  enum UserActions {
    ACTION_FORCE_UPDATE,
    ACTION_MANUAL_RESET
  };

  typedef uint32_t UiAction;
};

class H1200State {
public:
  static constexpr int kMaxInversion = 3;

  void Init() {
    cursor_pos = 0;
    display_notes = true;
  
    tonnetz_state.init();
  }

  void force_update() {
    ui_actions.Write(H1200::ACTION_FORCE_UPDATE);
  }

  void manual_reset() {
    ui_actions.Write(H1200::ACTION_MANUAL_RESET);
  }

  int cursor_pos;
  bool display_notes;

  TonnetzState tonnetz_state;
  util::RingBuffer<H1200::UiAction, 4> ui_actions;
};

H1200Settings h1200_settings;
H1200State h1200_state;

#define H1200_OUTPUT_NOTE(i,dac) \
do { \
  int32_t note = h1200_state.tonnetz_state.outputs(i); \
  note += 3 * 12; \
  note <<= 7; \
  DAC::set<dac>(DAC::pitch_to_dac(note, 0)); \
} while (0)

void FASTRUN H1200_clock(uint32_t triggers) {
  // Reset has priority
  if (triggers & TRIGGER_MASK_TR1) {
    h1200_state.tonnetz_state.reset(h1200_settings.mode());
  }
  // Since there can be simultaneous triggers, there is a definable priority.
  // Reset always has top priority
  //
  // Note: Proof-of-concept code, do not copy/paste all combinations ;)
  switch (h1200_settings.get_transform_priority()) {
    case TRANSFORM_PRIO_XPLR:
      if (triggers & TRIGGER_MASK_P) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
      if (triggers & TRIGGER_MASK_L) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
      if (triggers & TRIGGER_MASK_R) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
      break;

    case TRANSFORM_PRIO_XRPL:
      if (triggers & TRIGGER_MASK_R) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
      if (triggers & TRIGGER_MASK_P) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
      if (triggers & TRIGGER_MASK_L) h1200_state.tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
      break;

    default: break;
  }

  // Skip the quantizer since we just want semitones
  int32_t root = (OC::ADC::pitch_value(ADC_CHANNEL_1) >> 7) + h1200_settings.root_offset();
  
  int inversion = h1200_settings.inversion() + (OC::ADC::value<ADC_CHANNEL_4>() >> 9);
  CONSTRAIN(inversion, -H1200State::kMaxInversion, H1200State::kMaxInversion);

  h1200_state.tonnetz_state.render(root, inversion);

  switch (h1200_settings.output_mode()) {
    case OUTPUT_CHORD_VOICING: {
      H1200_OUTPUT_NOTE(0,DAC_CHANNEL_A);
      H1200_OUTPUT_NOTE(1,DAC_CHANNEL_B);
      H1200_OUTPUT_NOTE(2,DAC_CHANNEL_C);
      H1200_OUTPUT_NOTE(3,DAC_CHANNEL_D);
    }
    break;
    case OUTPUT_TUNE: {
      H1200_OUTPUT_NOTE(0,DAC_CHANNEL_A);
      H1200_OUTPUT_NOTE(0,DAC_CHANNEL_B);
      H1200_OUTPUT_NOTE(0,DAC_CHANNEL_C);
      H1200_OUTPUT_NOTE(0,DAC_CHANNEL_D);
    }
    break;
    default: break;
  }

  if (triggers)
    MENU_REDRAW = 1;
}

void H1200_init() {
  h1200_settings.Init();
  h1200_state.Init();
  init_circle_lut();
}

size_t H1200_storageSize() {
  return H1200Settings::storageSize();
}

size_t H1200_save(void *storage) {
  return h1200_settings.Save(storage);
}

size_t H1200_restore(const void *storage) {
  return h1200_settings.Restore(storage);
}

void H1200_handleEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      encoder[LEFT].setPos(0);
      encoder[RIGHT].setPos(0);
      h1200_state.tonnetz_state.reset(h1200_settings.mode());
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER:
      break;
  }
}

void H1200_isr() {
  uint32_t triggers = OC::DigitalInputs::clocked();

  while (h1200_state.ui_actions.readable()) {
    switch (h1200_state.ui_actions.Read()) {
      case H1200::ACTION_FORCE_UPDATE:
        triggers |= TRIGGER_MASK_DIRTY;
        break;
      case H1200::ACTION_MANUAL_RESET:
        triggers |= TRIGGER_MASK_RESET;
        break;
      default:
        break;
    }
  }

  H1200_clock(triggers);
}

void H1200_loop() {
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  buttons(BUTTON_BOTTOM);
  buttons(BUTTON_TOP);
  buttons(BUTTON_LEFT);
  buttons(BUTTON_RIGHT);
}

void H1200_topButton() {
  if (h1200_settings.change_value(H1200_SETTING_INVERSION, 1)) {
    h1200_state.force_update();
  }
}

void H1200_lowerButton() {
  if (h1200_settings.change_value(H1200_SETTING_INVERSION, -1)) {
    h1200_state.force_update();
  }
}

void H1200_rightButton() {
  ++h1200_state.cursor_pos;
  if (h1200_state.cursor_pos >= H1200_SETTING_LAST)
    h1200_state.cursor_pos = 0;
}

void H1200_leftButton() {
  h1200_state.display_notes = !h1200_state.display_notes;
}

void H1200_leftButtonLong() {
  h1200_settings.InitDefaults();
  h1200_state.manual_reset();
}

bool H1200_encoders() {
  bool changed = false;
  int value = encoder[LEFT].pos();
  encoder[LEFT].setPos(0);

  if (value) {
    value += h1200_state.cursor_pos;
    CONSTRAIN(value, 0, H1200_SETTING_LAST - 1);
    h1200_state.cursor_pos = value;
    changed = true;
  }

  value = encoder[RIGHT].pos();
  encoder[RIGHT].setPos(0);
  if (value && h1200_settings.change_value(h1200_state.cursor_pos, value)) {
    h1200_state.force_update();
    changed = true;
  }

  return changed;
}

void H1200_menu() {
  GRAPHICS_BEGIN_FRAME(false);
  graphics.setFont(UI_DEFAULT_FONT);

  static const uint8_t kStartX = 0;

  const EMode current_mode = h1200_state.tonnetz_state.current_chord().mode();
  int outputs[4];
  h1200_state.tonnetz_state.get_outputs(outputs);

  UI_DRAW_TITLE(kStartX);
  if (h1200_state.display_notes)
    graphics.print(note_name(outputs[0]));
  else
    graphics.print(outputs[0]);
  graphics.print(mode_names[current_mode]);

  graphics.setPrintPos(64, kUiTitleTextY);
  if (h1200_state.display_notes) {
    for (size_t i=1; i < 4; ++i) {
      if (i > 1) graphics.print(' ');
      graphics.print(note_name(outputs[i]));
    }
  } else {
    for (size_t i=1; i < 4; ++i) {
      if (i > 1) graphics.print(' ');
      graphics.print(outputs[i]);
    }
  }

  int first_visible = h1200_state.cursor_pos - 2;
  if (first_visible < 0)
    first_visible = 0;

  UI_START_MENU(kStartX);
  UI_BEGIN_ITEMS_LOOP(kStartX, first_visible, H1200_SETTING_LAST, h1200_state.cursor_pos, 0)
    const settings::value_attr &attr = H1200Settings::value_attr(current_item);
    UI_DRAW_SETTING(attr, h1200_settings.get_value(current_item), kUiWideMenuCol1X);
  UI_END_ITEMS_LOOP();

  GRAPHICS_END_FRAME();
}

void H1200_screensaver() {
  GRAPHICS_BEGIN_FRAME(false);

  uint8_t y = 0;
  static const uint8_t x_col_0 = 66;
  static const uint8_t x_col_1 = 66 + 24;
  static const uint8_t x_col_2 = 66 + 38;
  static const uint8_t line_h = 16;
  static const weegfx::coord_t note_circle_x = 32;
  static const weegfx::coord_t note_circle_y = 32;

  //u8g.setFont(u8g_font_timB12); BBX 19x27
  //graphics.setFont(u8g_font_10x20); // fixed-width makes positioning a bit easier
 
  uint32_t history = h1200_state.tonnetz_state.history();
  int outputs[4];
  h1200_state.tonnetz_state.get_outputs(outputs);

  uint8_t normalized[3];
  y = 8;
  for (size_t i=0; i < 3; ++i, y += line_h) {
    int value = outputs[i + 1];

    graphics.setPrintPos(x_col_1, y);
    graphics.print(value / 12);

    value = (value + 120) % 12;
    graphics.setPrintPos(x_col_2, y);
    graphics.print(OC::Strings::note_names[value]);
    normalized[i] = value;
  }
  y = 0;

  size_t len = 4;
  while (len--) {
    graphics.setPrintPos(x_col_0, y);
    graphics.print(history & 0x80 ? '+' : '-');
    graphics.print(tonnetz::transform_names[static_cast<tonnetz::ETransformType>(history & 0x7f)]);
    y += line_h;
    history >>= 8;
  }

  visualize_pitch_classes(normalized, note_circle_x, note_circle_y);

  GRAPHICS_END_FRAME();
}
