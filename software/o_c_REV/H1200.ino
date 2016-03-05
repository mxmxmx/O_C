
#include "tonnetz_state.h"
#include "util/util_settings.h"

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

enum EH1200Settings {
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

  ETransformPriority get_transform_priority() const {
    return static_cast<ETransformPriority>(values_[H1200_SETTING_TRANFORM_PRIO]);
  }

  EOutputMode output_mode() const {
    return static_cast<EOutputMode>(values_[H1200_SETTING_OUTPUT_MODE]);
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
  {TRANSFORM_PRIO_XPLR, 0, TRANSFORM_PRIO_LAST-1, "trigger prio", trigger_mode_names, settings::STORAGE_TYPE_U8},
  {OUTPUT_CHORD_VOICING, 0, OUTPUT_MODE_LAST-1, "output", output_mode_names, settings::STORAGE_TYPE_U8}
};

struct H1200State {

  static constexpr int kMaxInversion = 7;

  void init() {
    cursor_pos = 0;
    value_changed = false;
    display_notes = true;
    last_draw_millis = 0;
  
    tonnetz_state.init();
  }

  int cursor_pos;
  bool value_changed;
  bool display_notes;

  uint32_t last_draw_millis;

  TonnetzState tonnetz_state;
};

H1200Settings h1200_settings;
H1200State h1200_state;

#define H1200_OUTPUT_NOTE(i,dac) \
do { \
  int32_t note = h1200_state.tonnetz_state.outputs(i) << 7; \
  note += 3 * 12 << 7; \
  DAC::set<dac>(DAC::pitch_to_dac(note, 0)); \
} while (0)

static constexpr uint32_t TRIGGER_MASK_TR1 = OC::DIGITAL_INPUT_1_MASK;
static constexpr uint32_t TRIGGER_MASK_P = OC::DIGITAL_INPUT_2_MASK;
static constexpr uint32_t TRIGGER_MASK_L = OC::DIGITAL_INPUT_3_MASK;
static constexpr uint32_t TRIGGER_MASK_R = OC::DIGITAL_INPUT_4_MASK;
static constexpr uint32_t TRIGGER_MASK_DIRTY = 0x10;
static constexpr uint32_t TRIGGER_MASK_RESET = TRIGGER_MASK_TR1 | TRIGGER_MASK_DIRTY;

void FASTRUN H1200_clock(uint32_t triggers) {

  // Since there can be simultaneous triggers, there is a definable priority.
  // Reset always has top priority
  //
  // Note: Proof-of-concept code, do not copy/paste all combinations ;)
  if (triggers & TRIGGER_MASK_RESET)
    h1200_state.tonnetz_state.reset(h1200_settings.mode());

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
  
  int inversion = h1200_settings.inversion() + (OC::ADC::value<ADC_CHANNEL_4>() >> 9); //cvval[3]; // => octave in original
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

  if (triggers || millis() - h1200_state.last_draw_millis > 1000) {
    MENU_REDRAW = 1;
    h1200_state.last_draw_millis = millis();
  }
}

void H1200_init() {
  h1200_settings.InitDefaults();
  h1200_state.init();
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
      encoder[LEFT].setPos(h1200_state.cursor_pos);
      encoder[RIGHT].setPos(h1200_settings.get_value(h1200_state.cursor_pos));
      h1200_state.tonnetz_state.reset(h1200_settings.mode());
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER:
      break;
  }
}

#define CLOCKIT() \
do { \
  uint32_t triggers = \
    OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>() | \
    OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_2>() | \
    OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_3>() | \
    OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_4>(); \
  if (h1200_state.value_changed) { triggers |= TRIGGER_MASK_DIRTY; h1200_state.value_changed = false; } \
  H1200_clock(triggers); \
} while (0)

void H1200_loop() {
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
