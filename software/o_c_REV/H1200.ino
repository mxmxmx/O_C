#include "tonnetz_state.h"

enum EOutputMode {
  OUTPUT_CHORD_VOICING,
  OUTPUT_TUNE
};

enum ETriggerMapping {
  TRIGGER_MAP_XPLR,
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
};

H1200Settings h1200_settings = {
  MODE_MAJOR,
  0,
  TRIGGER_MAP_XPLR,
  OUTPUT_CHORD_VOICING
};

TonnetzState tonnetz_state;

#define OUTPUT_NOTE(i,dac_setter) \
do { \
  int note = tonnetz_state.outputs(i); \
  if (note > RANGE) note -= 12; \
  else if (note < 0) note += 12; \
  dac_setter(semitones[note]); \
} while (0)

void FASTRUN H1200_clock() {

  tonnetz::ETransformType transform = tonnetz::TRANSFORM_NONE;
  if (!digitalReadFast(TR1))
    tonnetz_state.reset(h1200_settings.mode);
  if (!digitalReadFast(TR2)) transform = tonnetz::TRANSFORM_P;
  if (!digitalReadFast(TR3)) transform = tonnetz::TRANSFORM_L;
  if (!digitalReadFast(TR4)) transform = tonnetz::TRANSFORM_R;

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
  }

  MENU_REDRAW = 1;
  UI_MODE = 1;
}

void H1200_init() {
  tonnetz_state.init();
  UI_MODE = 1;
  MENU_REDRAW = 1;
}

#define CLOCKIT() \
do { \
  if (CLK_STATE1) { \
    CLK_STATE1 = false; \
    H1200_clock(); \
  } \
} while (0)

void H1200_loop() {
  CLOCKIT();
  UI();
  CLOCKIT();
  if (_ADC) CV();
  CLOCKIT();

//  if (UI_MODE) timeout();
  CLOCKIT();
}
