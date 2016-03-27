// Copyright (c)  2015, 2016 Patrick Dowling, Max Stadler, Tim Churches
//
// Author of original O+C firmware: Max Stadler (mxmlnstdlr@gmail.com)
// Author of app scaffolding: Patrick Dowling (pld@gurkenkiste.com)
// Modified for bouncing balls: Tim Churches (tim.churches@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Bouncing balls app

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
    ui.selected_channel = 0;
    ui.selected_segment = 0;
    ui.cursor.Init(BB_SETTING_GRAVITY, BB_SETTING_LAST - 1);
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

    int selected_channel;
    int selected_segment;
    menu::ScreenCursor<menu::kScreenLines> cursor;
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

void BBGEN_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      bbgen.ui.cursor.set_editing(false);
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
      break;
  }
}

void BBGEN_loop() {
}

void BBGEN_menu() {

  menu::QuadTitleBar::Draw();
  for (uint_fast8_t i = 0; i < 4; ++i) {
    menu::QuadTitleBar::SetColumn(i);
    graphics.print((char)('A' + i));
  }
  menu::QuadTitleBar::Selected(bbgen.ui.selected_channel);

  auto const &bb = bbgen.selected();
  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(bbgen.ui.cursor);
  menu::SettingsListItem list_item;
  while (settings_list.available()) {
    const int current = settings_list.Next(list_item);
    list_item.DrawDefault(bb.get_value(current), BouncingBall::value_attr(current));
  }
}

void BBGEN_topButton() {
  auto &selected_bb = bbgen.selected();
  selected_bb.change_value(BB_SETTING_GRAVITY + bbgen.ui.selected_segment, 32);
}

void BBGEN_lowerButton() {
  auto &selected_bb = bbgen.selected();
  selected_bb.change_value(BB_SETTING_GRAVITY + bbgen.ui.selected_segment, -32); 
}

void BBGEN_handleButtonEvent(const UI::Event &event) {
  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        BBGEN_topButton();
        break;
      case OC::CONTROL_BUTTON_DOWN:
        BBGEN_lowerButton();
        break;
      case OC::CONTROL_BUTTON_L:
        break;
      case OC::CONTROL_BUTTON_R:
        bbgen.ui.cursor.toggle_editing();
        break;
    }
  }
}

void BBGEN_handleEncoderEvent(const UI::Event &event) {

  if (OC::CONTROL_ENCODER_L == event.control) {
    int left_value = bbgen.ui.selected_channel + event.value;
    CONSTRAIN(left_value, 0, 3);
    bbgen.ui.selected_channel = left_value;
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    if (bbgen.ui.cursor.editing()) {
      auto &selected = bbgen.selected();
      selected.change_value(bbgen.ui.cursor.cursor_pos(), event.value);
    } else {
      bbgen.ui.cursor.Scroll(event.value);
    }
  }
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
  OC::scope_render();
}

void FASTRUN BBGEN_isr() {
  bbgen.ISR();
}
