
#include "tonnetz_state.h"
#include "util_settings.h"

extern uint16_t semitones[RANGE+1];

enum EOutputMode {
  OUTPUT_CHORD_VOICING,
  OUTPUT_TUNE,
  OUTPUT_MODE_LAST
};

enum ETransformPriority {
  TRANSFORM_PRIO_XPLR,
  TRANSFORM_PRIO_XRPL,
  TRANSFORM_PRIO_LAST
};

enum ESettings {
  SETTING_ROOT_OFFSET,
  SETTING_MODE,
  SETTING_INVERSION,
  SETTING_TRANFORM_PRIO,
  SETTING_OUTPUT_MODE,
  SETTING_LAST
};

static const int MAX_INVERSION = 9;

class H1200Settings : public settings::SettingsBase<H1200Settings, SETTING_LAST> {
public:

  void init() {
    init_defaults();
  }

  int root_offset() const {
    return values_[SETTING_ROOT_OFFSET];
  }

  EMode mode() const {
    return static_cast<EMode>(values_[SETTING_MODE]);
  }

  int inversion() const {
    return values_[SETTING_INVERSION];
  }

  ETransformPriority get_transform_priority() const {
    return static_cast<ETransformPriority>(values_[SETTING_TRANFORM_PRIO]);
  }

  EOutputMode output_mode() const {
    return static_cast<EOutputMode>(values_[SETTING_OUTPUT_MODE]);
  }
};

const char *output_mode_names[] = {
  "CHORD",
  "TUNE"
};

const char *trigger_mode_names[] = {
  "P>L>R",
  "R>P>L",
};

const char *mode_names[] = {
  "maj", "min"
};

/*static*/ template<> const settings::value_attr settings::SettingsBase<H1200Settings, SETTING_LAST>::value_attr_[] = {
  {12, -24, 36, "transpose", NULL},
  {MODE_MAJOR, 0, MODE_LAST-1, "mode", mode_names},
  {0, -3, 3, "inversion", NULL},
  {TRANSFORM_PRIO_XPLR, 0, TRANSFORM_PRIO_LAST-1, "trigger prio", trigger_mode_names},
  {OUTPUT_CHORD_VOICING, 0, OUTPUT_MODE_LAST-1, "output", output_mode_names}
};

struct H1200_menu_state {
  int cursor_pos;
  int cursor_value;
  bool value_changed;
  bool display_notes;
};

H1200Settings h1200_settings;
H1200_menu_state menu_state = {
  0,
  0,
  false,
  true,
};

TonnetzState tonnetz_state;
uint32_t last_draw_millis = 0;

#define OUTPUT_NOTE(i,dac_setter) \
do { \
  int note = tonnetz_state.outputs(i); \
  while (note > RANGE) note -= 12; \
  while (note < 0) note += 12; \
  const uint16_t dac_code = semitones[note]; \
  asr_outputs[i] = dac_code; \
  dac_setter(dac_code); \
} while (0)

static const uint32_t TRIGGER_MASK_TR1 = 0x1;
static const uint32_t TRIGGER_MASK_P = 0x2;
static const uint32_t TRIGGER_MASK_L = 0x4;
static const uint32_t TRIGGER_MASK_R = 0x8;
static const uint32_t TRIGGER_MASK_DIRTY = 0x10;
static const uint32_t TRIGGER_MASK_RESET = TRIGGER_MASK_TR1 | TRIGGER_MASK_DIRTY;

void FASTRUN H1200_clock(uint32_t triggers) {

  // Since there can be simultaneous triggers, there is a definable priority.
  // Reset always has top priority
  //
  // Note: Proof-of-concept code, do not copy/paste all combinations ;)
  if (triggers & TRIGGER_MASK_RESET)
    tonnetz_state.reset(h1200_settings.mode());

  switch (h1200_settings.get_transform_priority()) {
    case TRANSFORM_PRIO_XPLR:
      if (triggers & TRIGGER_MASK_P) tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
      if (triggers & TRIGGER_MASK_L) tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
      if (triggers & TRIGGER_MASK_R) tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
      break;

    case TRANSFORM_PRIO_XRPL:
      if (triggers & TRIGGER_MASK_R) tonnetz_state.apply_transformation(tonnetz::TRANSFORM_R);
      if (triggers & TRIGGER_MASK_P) tonnetz_state.apply_transformation(tonnetz::TRANSFORM_P);
      if (triggers & TRIGGER_MASK_L) tonnetz_state.apply_transformation(tonnetz::TRANSFORM_L);
      break;

    default: break;
  }

  int32_t sample = cvval[0];
  int root;
  if (sample < 0)
    root = 0;
  else if (sample < S_RANGE)
    root = sample >> 5;
  else
    root = RANGE;
  root += h1200_settings.root_offset();
  
  int inversion = h1200_settings.inversion() + cvval[3]; // => octave in original
  if (inversion > MAX_INVERSION) inversion = MAX_INVERSION;
  else if (inversion < -MAX_INVERSION) inversion = -MAX_INVERSION;
  tonnetz_state.render(root, inversion);

  switch (h1200_settings.output_mode()) {
    case OUTPUT_CHORD_VOICING: {
      OUTPUT_NOTE(0,set8565_CHA);
      OUTPUT_NOTE(1,set8565_CHB);
      OUTPUT_NOTE(2,set8565_CHC);
      OUTPUT_NOTE(3,set8565_CHD);
    }
    break;
    case OUTPUT_TUNE: {
      OUTPUT_NOTE(0,set8565_CHA);
      OUTPUT_NOTE(0,set8565_CHB);
      OUTPUT_NOTE(0,set8565_CHC);
      OUTPUT_NOTE(0,set8565_CHD);
    }
    break;
    default: break;
  }

  if (triggers || millis() - last_draw_millis > 1000) {
    MENU_REDRAW = 1;
    last_draw_millis = millis();
  }
}

void H1200_init() {
  h1200_settings.init();
  tonnetz_state.init();
  init_circle_lut();
  UI_MODE = 1;
  MENU_REDRAW = 1;
}

#define CLOCKIT() \
do { \
  uint32_t triggers = 0; \
  if (CLK_STATE[TR1]) { triggers |= TRIGGER_MASK_TR1; CLK_STATE[TR1] = false; } \
  if (CLK_STATE[TR2]) { triggers |= TRIGGER_MASK_P; CLK_STATE[TR2] = false; } \
  if (CLK_STATE[TR3]) { triggers |= TRIGGER_MASK_L; CLK_STATE[TR3] = false; } \
  if (CLK_STATE[TR4]) { triggers |= TRIGGER_MASK_R; CLK_STATE[TR4] = false; } \
  if (menu_state.value_changed) { triggers |= TRIGGER_MASK_DIRTY; menu_state.value_changed = false; } \
  H1200_clock(triggers); \
} while (0)

void H1200_loop() {
  CLOCKIT();
  UI();
  CLOCKIT();
  if (_ADC) CV();
  CLOCKIT();
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  CLOCKIT();
  buttons(BUTTON_BOTTOM);
  CLOCKIT();
  buttons(BUTTON_TOP);
  CLOCKIT();
  buttons(BUTTON_LEFT);
  CLOCKIT();
  buttons(BUTTON_RIGHT);
  CLOCKIT();
}
