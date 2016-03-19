#include "OC_apps.h"
#include "OC_bitmaps.h"
#include "OC_digital_inputs.h"
#include "OC_strings.h"
#include "util/util_math.h"
#include "util/util_settings.h"
#include "OC_menus.h"
#include "peaks_bouncing_balls.h"

enum BouncingBallSettings {
  BB_SETTING_GRAVITY,
  BB_SETTING_BOUNCE_LOSS,
  BB_SETTING_INITIAL_AMPLITUDE,
  BB_SETTING_INITIAL_VELOCITY,
  BB_SETTING_TRIGGER_INPUT,
  BB_SETTING_CV1,
  BB_SETTING_CV2,
  BB_SETTING_CV3,
  BB_SETTING_CV4,
  BB_SETTING_HARD_RESET,
  BB_SETTING_LAST
};

enum BallCVMapping {
  BB_CV_MAPPING_NONE,
  BB_CV_MAPPING_GRAVITY,
  BB_CV_MAPPING_BOUNCE_LOSS,
  BB_CV_MAPPING_INITIAL_AMPLITUDE,
  BB_CV_MAPPING_INITIAL_VELOCITY,
  BB_CV_MAPPING_LAST
};


class BouncingBall : public settings::SettingsBase<BouncingBall, BB_SETTING_LAST> {
public:

  static constexpr int kMaxBouncingBallParameters = 4;

  void Init(OC::DigitalInput default_trigger);

  OC::DigitalInput get_trigger_input() const {
    return static_cast<OC::DigitalInput>(values_[BB_SETTING_TRIGGER_INPUT]);
  }

  BallCVMapping get_cv1_mapping() const {
    return static_cast<BallCVMapping>(values_[BB_SETTING_CV1]);
  }

  BallCVMapping get_cv2_mapping() const {
    return static_cast<BallCVMapping>(values_[BB_SETTING_CV2]);
  }

  BallCVMapping get_cv3_mapping() const {
    return static_cast<BallCVMapping>(values_[BB_SETTING_CV3]);
  }

  BallCVMapping get_cv4_mapping() const {
    return static_cast<BallCVMapping>(values_[BB_SETTING_CV4]);
  }

  bool get_hard_reset() const {
    return values_[BB_SETTING_HARD_RESET];
  }

  uint8_t get_initial_amplitude() const {
    return values_[BB_SETTING_INITIAL_AMPLITUDE];
  }

  uint8_t get_initial_velocity() const {
    return values_[BB_SETTING_INITIAL_VELOCITY];
  }

  uint8_t get_gravity() const {
    return values_[BB_SETTING_GRAVITY];
  }

  uint8_t get_bounce_loss() const {
    return values_[BB_SETTING_BOUNCE_LOSS];
  }

  inline void apply_cv_mapping(BouncingBallSettings cv_setting, const int32_t cvs[ADC_CHANNEL_LAST], int32_t segments[kMaxBouncingBallParameters]) {
    int mapping = values_[cv_setting];
    uint8_t bb_cv_rshift = 12 ;
    switch (mapping) {
      case BB_CV_MAPPING_GRAVITY:
      case BB_CV_MAPPING_BOUNCE_LOSS:
        bb_cv_rshift = 13 ;
        break ;
      case BB_CV_MAPPING_INITIAL_AMPLITUDE:
        bb_cv_rshift = 12 ;
        break;
      case BB_CV_MAPPING_INITIAL_VELOCITY:
        bb_cv_rshift = 13 ;
        break;
      default:
        break;
    }
    segments[mapping - BB_CV_MAPPING_GRAVITY] += (cvs[cv_setting - BB_SETTING_CV1] * 65536) >> bb_cv_rshift ;
  }

  template <DAC_CHANNEL dac_channel>
  void Update(uint32_t triggers, const int32_t cvs[ADC_CHANNEL_LAST]) {

    int32_t s[kMaxBouncingBallParameters];
    s[0] = SCALE8_16(static_cast<int32_t>(get_gravity()));
    s[1] = SCALE8_16(static_cast<int32_t>(get_bounce_loss()));
    s[2] = SCALE8_16(static_cast<int32_t>(get_initial_amplitude()));
    s[3] = SCALE8_16(static_cast<int32_t>(get_initial_velocity()));

    apply_cv_mapping(BB_SETTING_CV1, cvs, s);
    apply_cv_mapping(BB_SETTING_CV2, cvs, s);
    apply_cv_mapping(BB_SETTING_CV3, cvs, s);
    apply_cv_mapping(BB_SETTING_CV4, cvs, s);

    s[0] = USAT16(s[0]);
    s[1] = USAT16(s[1]);
    s[2] = USAT16(s[2]);
    s[3] = USAT16(s[3]);
    
    bb_.Configure(s) ; 

    // hard reset forces the bouncing ball to start at level_[0] on rising gate.
    bb_.set_hard_reset(get_hard_reset());

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
    uint32_t value = OC::calibration_data.dac.octaves[_ZERO] + bb_.ProcessSingleSample(gate_state);
    DAC::set<dac_channel>(value);
  }


private:
  peaks::BouncingBall bb_;
  bool gate_raised_;
};

void BouncingBall::Init(OC::DigitalInput default_trigger) {
  InitDefaults();
  apply_value(BB_SETTING_TRIGGER_INPUT, default_trigger);
  bb_.Init();
  gate_raised_ = false;
}

const char* const bb_cv_mapping_names[BB_CV_MAPPING_LAST] = {
  "off", "grav", "bnce", "ampl", "vel" 
};

SETTINGS_DECLARE(BouncingBall, BB_SETTING_LAST) {
  { 128, 0, 255, "Gravity", NULL, settings::STORAGE_TYPE_U8 },
  { 96, 0, 255, "Bounce loss", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 255, "Amplitude", NULL, settings::STORAGE_TYPE_U8 }, 
  { 228, 0, 255, "Velocity", NULL, settings::STORAGE_TYPE_U8 },
  { OC::DIGITAL_INPUT_1, OC::DIGITAL_INPUT_1, OC::DIGITAL_INPUT_4, "Trigger input", OC::Strings::trigger_input_names, settings::STORAGE_TYPE_U8 },
  { BB_CV_MAPPING_NONE, BB_CV_MAPPING_NONE, BB_CV_MAPPING_LAST - 1, "CV1 -> ", bb_cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { BB_CV_MAPPING_NONE, BB_CV_MAPPING_NONE, BB_CV_MAPPING_LAST - 1, "CV2 -> ", bb_cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { BB_CV_MAPPING_NONE, BB_CV_MAPPING_NONE, BB_CV_MAPPING_LAST - 1, "CV3 -> ", bb_cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { BB_CV_MAPPING_NONE, BB_CV_MAPPING_NONE, BB_CV_MAPPING_LAST - 1, "CV4 -> ", bb_cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { 0, 0, 1, "Hard reset", OC::Strings::no_yes, settings::STORAGE_TYPE_U4 },
};

class QuadBouncingBalls {
public:
  static constexpr int32_t kCvSmoothing = 16;

  // bb = env, balls_ = envelopes_, BouncingBall = EnvelopeGenerator
  // QuadBouncingBalls = QuadEnvelopeGenerator, bbgen = envgen, BBGEN = ENVGEN

  void Init() {
    int input = OC::DIGITAL_INPUT_1;
    for (auto &bb : balls_) {
      bb.Init(static_cast<OC::DigitalInput>(input));
      ++input;
    }

    ui.left_encoder_value = 0;
    ui.left_edit_mode = MODE_EDIT_SETTINGS;
    ui.editing = false;
    ui.selected_channel = 0;
    ui.selected_segment = 0;
    ui.selected_setting = BB_SETTING_TRIGGER_INPUT;
  }

  void ISR() {
    cv1.push(OC::ADC::value<ADC_CHANNEL_1>());
    cv2.push(OC::ADC::value<ADC_CHANNEL_2>());
    cv3.push(OC::ADC::value<ADC_CHANNEL_3>());
    cv4.push(OC::ADC::value<ADC_CHANNEL_4>());

    const int32_t cvs[ADC_CHANNEL_LAST] = { cv1.value(), cv2.value(), cv3.value(), cv4.value() };
    uint32_t triggers = OC::DigitalInputs::clocked();

    balls_[0].Update<DAC_CHANNEL_A>(triggers, cvs);
    balls_[1].Update<DAC_CHANNEL_B>(triggers, cvs);
    balls_[2].Update<DAC_CHANNEL_C>(triggers, cvs);
    balls_[3].Update<DAC_CHANNEL_D>(triggers, cvs);
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

  BouncingBall &selected() {
    return balls_[ui.selected_channel];
  }

  BouncingBall balls_[4];

  SmoothedValue<int32_t, kCvSmoothing> cv1;
  SmoothedValue<int32_t, kCvSmoothing> cv2;
  SmoothedValue<int32_t, kCvSmoothing> cv3;
  SmoothedValue<int32_t, kCvSmoothing> cv4;
};

QuadBouncingBalls bbgen;

void BBGEN_init() {
  bbgen.Init();
}

size_t BBGEN_storageSize() {
  return 4 * BouncingBall::storageSize();
}

size_t BBGEN_save(void *storage) {
  size_t s = 0;
  for (auto &bb : bbgen.balls_)
    s += bb.Save(static_cast<byte *>(storage) + s);
  return s;
}

size_t BBGEN_restore(const void *storage) {
  size_t s = 0;
  for (auto &bb : bbgen.balls_)
    s += bb.Restore(static_cast<const byte *>(storage) + s);
  return s;
}

void BBGEN_handleEvent(OC::AppEvent event) {
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

void BBGEN_loop() {
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  buttons(BUTTON_TOP);
  buttons(BUTTON_BOTTOM);
  buttons(BUTTON_LEFT);
  buttons(BUTTON_RIGHT);
}

void BBGEN_menu_settings() {
  auto const &bb = bbgen.selected();
  UI_START_MENU(0);

  int first_visible_param = bbgen.ui.selected_setting - 3;
  if (first_visible_param < BB_SETTING_GRAVITY)
    first_visible_param = BB_SETTING_GRAVITY;

  UI_BEGIN_ITEMS_LOOP(kStartX, first_visible_param, BB_SETTING_LAST, bbgen.ui.selected_setting, 0); // was 1
    UI_DRAW_EDITABLE(bbgen.ui.editing);
    UI_DRAW_SETTING(BouncingBall::value_attr(current_item), bb.get_value(current_item), kUiWideMenuCol1X);
  UI_END_ITEMS_LOOP();
}

void BBGEN_menu() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem
  static const weegfx::coord_t kStartX = 0;
  UI_DRAW_TITLE(kStartX);

  for (int i = 0, x = 0; i < 4; ++i, x += 32) {
    graphics.setPrintPos(x + 6, 2);
    graphics.print((char)('A' + i));
    graphics.setPrintPos(x + 14, 2);
    if (i == bbgen.ui.selected_channel) {
      graphics.invertRect(x, 0, 32, 11);
    }
  }

  BBGEN_menu_settings();

  // TODO Draw phase anyway?
  GRAPHICS_END_FRAME();
}

void BBGEN_topButton() {
  auto &selected_bb = bbgen.selected();
  selected_bb.change_value(BB_SETTING_GRAVITY + bbgen.ui.selected_segment, 32);
}

void BBGEN_lowerButton() {
  auto &selected_bb = bbgen.selected();
  selected_bb.change_value(BB_SETTING_GRAVITY + bbgen.ui.selected_segment, -32); 
}

void BBGEN_rightButton() {
    bbgen.ui.editing = !bbgen.ui.editing;
    encoder[LEFT].setPos(0);
    encoder[RIGHT].setPos(0);
}

void BBGEN_leftButton() {
//  if (QuadBouncingBalls::MODE_EDIT_SETTINGS == bbgen.ui.left_edit_mode) {
//    bbgen.ui.left_edit_mode = QuadBouncingBalls::MODE_SELECT_CHANNEL;
//  } else {
//    bbgen.ui.left_edit_mode = QuadBouncingBalls::MODE_EDIT_SETTINGS;
//  }
  encoder[LEFT].setPos(0);
  encoder[RIGHT].setPos(0);  
}

void BBGEN_leftButtonLong() { }

bool BBGEN_encoders() {
  long left_value = encoder[LEFT].pos();
  long right_value = encoder[RIGHT].pos();
  bool changed = left_value || right_value;

  if (QuadBouncingBalls::MODE_SELECT_CHANNEL == bbgen.ui.left_edit_mode) {
    if (left_value) {
      left_value += bbgen.ui.selected_channel;
      CONSTRAIN(left_value, 0, 3);
      bbgen.ui.selected_channel = left_value;
    }
    if (right_value) {
      auto &selected_bb = bbgen.selected();
      selected_bb.change_value(BB_SETTING_GRAVITY + bbgen.ui.selected_segment, right_value);
    }
  } else {
    if (left_value) {
      left_value += bbgen.ui.selected_channel;
      CONSTRAIN(left_value, 0, 3);
      bbgen.ui.selected_channel = left_value;
    }
    if (right_value) {
      if (bbgen.ui.editing) {
        auto &selected_bb = bbgen.selected();
        selected_bb.change_value(bbgen.ui.selected_setting, right_value);
      } else {
        right_value += bbgen.ui.selected_setting;
        CONSTRAIN(right_value, BB_SETTING_GRAVITY, BB_SETTING_LAST - 1);
        bbgen.ui.selected_setting = right_value;
      }
    }
  }

  encoder[LEFT].setPos(0);
  encoder[RIGHT].setPos(0);
  return changed;
}

void BBGEN_debug() {
  graphics.setPrintPos(2, 12);
  graphics.print(bbgen.cv1.value());
  graphics.setPrintPos(2, 22);
  graphics.print(bbgen.cv2.value());
  graphics.setPrintPos(2, 32);
  graphics.print(bbgen.cv3.value());
  graphics.setPrintPos(2, 42);
  graphics.print(bbgen.cv4.value());
}

void BBGEN_screensaver() {
   GRAPHICS_BEGIN_FRAME(false);
   scope_render();
   GRAPHICS_END_FRAME();
}

void FASTRUN BBGEN_isr() {
  bbgen.ISR();
}
