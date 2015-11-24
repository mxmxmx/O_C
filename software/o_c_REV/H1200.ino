
#include "tonnetz_state.h"
#include "util_settings.h"

extern uint16_t semitones[RANGE+1];

enum EOutputMode {
  OUTPUT_CHORD_VOICING,
  OUTPUT_TUNE,
  OUTPUT_MODE_LAST
};

enum ETriggerMapping {
  TRIGGER_MAP_XPLR,
  TRIGGER_MAP_LAST
};

enum ESettings {
  SETTING_ROOT_OFFSET,
  SETTING_MODE,
  SETTING_INVERSION,
  SETTING_TRIGGER_MAP,
  SETTING_OUTPUT_MODE,
  SETTING_LAST
};

static const int MAX_INVERSION = 9;

class H1200Settings {
public:

  void init() {
    for (size_t i = 0; i < SETTING_LAST; ++i)
      values_[i] = value_attr_[i].default_value();
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

  ETriggerMapping trigger_mapping() const {
    return static_cast<ETriggerMapping>(values_[SETTING_TRIGGER_MAP]);
  }

  EOutputMode output_mode() const {
    return static_cast<EOutputMode>(values_[SETTING_OUTPUT_MODE]);
  }

  int get_value(size_t index) {
    return values_[index];
  }

  static int clamp_value(size_t index, int value) {
    return value_attr_[index].clamp(value);
  }

  bool apply_value(size_t index, int value) {
    if (index < SETTING_LAST) {
      const int clamped = value_attr_[index].clamp(value);
      if (values_[index] != clamped) {
        values_[index] = clamped;
        return true;
      }
    }
    return false;
  }

  static const settings::value_attr &value_attr(size_t i) {
    return value_attr_[i];
  }

//private:

  int values_[SETTING_LAST];
  static const settings::value_attr value_attr_[];
};


const char *output_mode_names[] = {
  "CHORD",
  "TUNE"
};

const char *trigger_mode_names[] = {
  "@PLR"
};

const char *mode_names[] = {
  "maj", "min"
};

/*static*/ const settings::value_attr H1200Settings::value_attr_[] = {
  {12, -24, 36, "TRANSPOSE", NULL},
  {MODE_MAJOR, 0, MODE_LAST-1, "MODE", mode_names},
  {0, -3, 3, "INVERSION", NULL},
  {TRIGGER_MAP_XPLR, 0, TRIGGER_MAP_LAST-1, "TRIGGERS", trigger_mode_names},
  {OUTPUT_CHORD_VOICING, 0, OUTPUT_MODE_LAST-1, "OTUPUT", output_mode_names}
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

#define OUTPUT_NOTE(i,dac_setter) \
do { \
  int note = tonnetz_state.outputs(i); \
  while (note > RANGE) note -= 12; \
  while (note < 0) note += 12; \
  const uint16_t dac_code = semitones[note]; \
  asr_outputs[i] = dac_code; \
  dac_setter(dac_code); \
} while (0)

void FASTRUN H1200_clock(uint32_t triggers) {

  tonnetz::ETransformType transform = tonnetz::TRANSFORM_NONE;
  if (triggers & 0x11 | triggers & 0x2 | triggers & 0x4) {
    tonnetz_state.reset_transform_indicator();
  }
  if (triggers & 0x11)
    tonnetz_state.reset(h1200_settings.mode());
  if (triggers & 0x2) {
    transform = tonnetz::TRANSFORM_P;
    tonnetz_state.set_transform_indicator('P',0);
  }
  if (triggers & 0x4) {
    transform = tonnetz::TRANSFORM_L;
    tonnetz_state.set_transform_indicator('L',1);
 }
  if (triggers & 0x8) {
    transform = tonnetz::TRANSFORM_R;
    tonnetz_state.set_transform_indicator('R',2);
  }

  //int trigger_mode = 8 + cvval[2]; // -> +- 8 notes
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
  tonnetz_state.render(root, transform, inversion);

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

  MENU_REDRAW = 1;
}

void H1200_init() {
  h1200_settings.init();
  tonnetz_state.init();
  UI_MODE = 1;
  MENU_REDRAW = 1;
}

#define CLOCKIT() \
do { \
  uint32_t triggers = 0; \
  if (CLK_STATE[TR1]) { triggers |= 0x1; CLK_STATE[TR1] = false; } \
  if (CLK_STATE[TR2]) { triggers |= 0x2; CLK_STATE[TR2] = false; } \
  if (CLK_STATE[TR3]) { triggers |= 0x4; CLK_STATE[TR3] = false; } \
  if (CLK_STATE[TR4]) { triggers |= 0x8; CLK_STATE[TR4] = false; } \
  if (menu_state.value_changed) { triggers |= 0x10; menu_state.value_changed = false; } \
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
