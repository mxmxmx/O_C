#include "tonnetz_state.h"

enum EOutputMode {
  OUTPUT_CHORD_VOICING,
  OUTPUT_TUNE,
  OUTPUT_MODE_LAST
};

enum ETriggerMapping {
  TRIGGER_MAP_XPLR,
  TRIGGER_MAP_LAST
};

struct H1200Settings {
  EMode mode;
  int inversion;
  ETriggerMapping trigger_mapping;
  EOutputMode output_mode;

  int get_value(size_t index) {
    switch (index) {
      case 0: return mode;
      case 1: return inversion;
      case 2: return trigger_mapping;
      case 3: return output_mode;
    }
    return -1;
  }

  int clamp_value(size_t index, int value) {
    if (index < 4)
      return value_attr_[index].clamp(value);
    else
      return value;
  }

  bool apply_value(size_t index, int value) {
    if (index < 4) {
      const int clamped = value_attr_[index].clamp(value);
      switch (index) {
        case 0:
          if (mode != clamped) {
            mode = static_cast<EMode>(clamped);
            return true;
          }
          break;
        case 1:
        if (inversion != clamped) {
          inversion = clamped;
          return true;
        }
        break;
        case 2:
          if (trigger_mapping != clamped) {
            trigger_mapping = static_cast<ETriggerMapping>(clamped);
            return true;
          }
          break;
        case 3:
          if (output_mode != clamped) {
            output_mode = static_cast<EOutputMode>(clamped);
            return true;
          }
          break;
      }
    }
    return false;
  }

  struct value_attr {
    int min_, max_;
    int clamp(int value) const {
      if (value < min_) return min_;
      else if (value > max_) return max_;
      else return value;
    }
  };
  static const value_attr value_attr_[];
};
/*static*/ const H1200Settings::value_attr H1200Settings::value_attr_[] = {
  {0, MODE_LAST-1},
  {-3, 3},
  {0, TRIGGER_MAP_LAST-1},
  {0, OUTPUT_MODE_LAST-1}
};

H1200Settings h1200_settings = {
  MODE_MAJOR,
  0,
  TRIGGER_MAP_XPLR,
  OUTPUT_CHORD_VOICING
};


struct H1200_menu_state {
  int cursor_pos;
  int cursor_value;
  bool value_changed;
};

H1200_menu_state menu_state = {
  0,
  0,
  false,
};

TonnetzState tonnetz_state;

#define OUTPUT_NOTE(i,dac_setter) \
do { \
  int note = tonnetz_state.outputs(i); \
  if (note > RANGE) note -= 12; \
  else if (note < 0) note += 12; \
  dac_setter(semitones[note]); \
} while (0)

void FASTRUN H1200_clock(uint32_t triggers) {

  tonnetz::ETransformType transform = tonnetz::TRANSFORM_NONE;
  if (triggers & 0x11)
    tonnetz_state.reset(h1200_settings.mode);
  if (triggers & 0x2) transform = tonnetz::TRANSFORM_P;
  if (triggers & 0x4) transform = tonnetz::TRANSFORM_L;
  if (triggers & 0x8) transform = tonnetz::TRANSFORM_R;

  //int trigger_mode = 8 + cvval[2]; // -> +- 8 notes
  int inversion = h1200_settings.inversion;// + cvval[3]; // => octave in original

  int32_t sample = cvval[0];

  int root;
  if (sample < 0)
    root = 0;
  else if (sample < S_RANGE)
    root = sample >> 5;
  else
    root = RANGE;
  
  tonnetz_state.render(root, transform, inversion);

  switch (h1200_settings.output_mode) {
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
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) H1200_update_ENC();
  CLOCKIT();
  buttons(BUTTON_TOP);
  CLOCKIT();
  buttons(BUTTON_BOTTOM);
  CLOCKIT();
  buttons(BUTTON_LEFT);
  CLOCKIT();
  buttons(BUTTON_RIGHT);
  CLOCKIT();
  if (UI_MODE) timeout(); 
  CLOCKIT();
}
