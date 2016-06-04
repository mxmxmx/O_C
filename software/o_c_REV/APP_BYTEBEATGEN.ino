// Copyright (c)  2015, 2016 Patrick Dowling, Max Stadler, Tim Churches
//
// Author of original O+C firmware: Max Stadler (mxmlnstdlr@gmail.com)
// Author of app scaffolding: Patrick Dowling (pld@gurkenkiste.com)
// Modified for byte beats: Tim Churches (tim.churches@gmail.com)
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

// Byte beats app

#include "OC_apps.h"
#include "OC_bitmaps.h"
#include "OC_digital_inputs.h"
#include "OC_strings.h"
#include "util/util_math.h"
#include "util/util_settings.h"
#include "OC_menus.h"
#include "peaks_bytebeat.h"

enum ByteBeatSettings {
  BYTEBEAT_SETTING_EQUATION,
  BYTEBEAT_SETTING_SPEED,
  BYTEBEAT_SETTING_P0,
  BYTEBEAT_SETTING_P1,
  BYTEBEAT_SETTING_P2,
  BYTEBEAT_SETTING_LOOP_MODE,
  BYTEBEAT_SETTING_LOOP_START,
  BYTEBEAT_SETTING_LOOP_START_MED,
  BYTEBEAT_SETTING_LOOP_START_FINE,
  BYTEBEAT_SETTING_LOOP_END,
  BYTEBEAT_SETTING_LOOP_END_MED,
  BYTEBEAT_SETTING_LOOP_END_FINE,
  BYTEBEAT_SETTING_TRIGGER_INPUT,
  BYTEBEAT_SETTING_STEP_MODE,
  BYTEBEAT_SETTING_CV1,
  BYTEBEAT_SETTING_CV2,
  BYTEBEAT_SETTING_CV3,
  BYTEBEAT_SETTING_CV4,
  BYTEBEAT_SETTING_LAST,
  BYTEBEAT_SETTING_FIRST=BYTEBEAT_SETTING_EQUATION,
};

enum ByteBeatCVMapping {
  BYTEBEAT_CV_MAPPING_NONE,
  BYTEBEAT_CV_MAPPING_EQUATION,
  BYTEBEAT_CV_MAPPING_SPEED,
  BYTEBEAT_CV_MAPPING_P0,
  BYTEBEAT_CV_MAPPING_P1,
  BYTEBEAT_CV_MAPPING_P2,
  BYTEBEAT_CV_MAPPING_LOOP_START,
  BYTEBEAT_CV_MAPPING_LOOP_START_MED,
  BYTEBEAT_CV_MAPPING_LOOP_START_FINE,
  BYTEBEAT_CV_MAPPING_LOOP_END,
  BYTEBEAT_CV_MAPPING_LOOP_END_MED,
  BYTEBEAT_CV_MAPPING_LOOP_END_FINE,
  BYTEBEAT_CV_MAPPING_LAST,
  BYTEBEAT_CV_MAPPING_FIRST=BYTEBEAT_CV_MAPPING_EQUATION
  
};

class ByteBeat : public settings::SettingsBase<ByteBeat, BYTEBEAT_SETTING_LAST> {
public:

  static constexpr size_t kHistoryDepth = 64;
  static constexpr int kMaxByteBeatParameters = 11;

  void Init(OC::DigitalInput default_trigger);

  OC::DigitalInput get_trigger_input() const {
    return static_cast<OC::DigitalInput>(values_[BYTEBEAT_SETTING_TRIGGER_INPUT]);
  }

  ByteBeatCVMapping get_cv1_mapping() const {
    return static_cast<ByteBeatCVMapping>(values_[BYTEBEAT_SETTING_CV1]);
  }

  ByteBeatCVMapping get_cv2_mapping() const {
    return static_cast<ByteBeatCVMapping>(values_[BYTEBEAT_SETTING_CV2]);
  }

  ByteBeatCVMapping get_cv3_mapping() const {
    return static_cast<ByteBeatCVMapping>(values_[BYTEBEAT_SETTING_CV3]);
  }

  ByteBeatCVMapping get_cv4_mapping() const {
    return static_cast<ByteBeatCVMapping>(values_[BYTEBEAT_SETTING_CV4]);
  }

  uint8_t get_equation() const {
    return values_[BYTEBEAT_SETTING_EQUATION];
  }

  bool get_step_mode() const {
    return values_[BYTEBEAT_SETTING_STEP_MODE];
  }

  uint8_t get_speed() const {
    return values_[BYTEBEAT_SETTING_SPEED];
  }

  uint8_t get_p0() const {
    return values_[BYTEBEAT_SETTING_P0];
  }

  uint8_t get_p1() const {
    return values_[BYTEBEAT_SETTING_P1];
  }

  uint8_t get_p2() const {
    return values_[BYTEBEAT_SETTING_P2];
  }

  bool get_loop_mode() const {
    return values_[BYTEBEAT_SETTING_LOOP_MODE];
  }

  uint8_t get_loop_start() const {
    return values_[BYTEBEAT_SETTING_LOOP_START];
  }

  uint8_t get_loop_start_med() const {
    return values_[BYTEBEAT_SETTING_LOOP_START_MED];
  }

  uint8_t get_loop_start_fine() const {
    return values_[BYTEBEAT_SETTING_LOOP_START_FINE];
  }

  uint8_t get_loop_end() const {
    return values_[BYTEBEAT_SETTING_LOOP_END];
  }

  uint8_t get_loop_end_med() const {
    return values_[BYTEBEAT_SETTING_LOOP_END_MED];
  }

  uint8_t get_loop_end_fine() const {
    return values_[BYTEBEAT_SETTING_LOOP_END_FINE];
  }

  int32_t get_s(uint8_t param) {
    return s_[param]; 
  }

  uint32_t get_t() {
    return static_cast<uint32_t>(bytebeat_.get_t()); 
  }

  uint32_t get_eqn_num() {
    return static_cast<uint32_t>(bytebeat_.get_eqn_num()); 
  }

  uint32_t get_phase() {
    return static_cast<uint32_t>(bytebeat_.get_phase()); 
  }

  uint32_t get_instance_loop_start() {
    return static_cast<uint32_t>(bytebeat_.get_loop_start()); 
  }

  uint32_t get_instance_loop_end() {
    return static_cast<uint32_t>(bytebeat_.get_loop_end()); 
  }

  uint16_t get_bytepitch() {
    return static_cast<uint16_t>(bytebeat_.get_bytepitch()); 
  }

  // Begin conditional menu items infrastructure
  // Maintain an internal list of currently available settings, since some are
  // dependent on others. It's kind of brute force, but eh, works :) If other
  // apps have a similar need, it can be moved to a common wrapper

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  ByteBeatSettings enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  void update_enabled_settings() {
    ByteBeatSettings *settings = enabled_settings_;
    *settings++ = BYTEBEAT_SETTING_EQUATION;
    *settings++ = BYTEBEAT_SETTING_SPEED;
    *settings++ = BYTEBEAT_SETTING_P0;
    *settings++ = BYTEBEAT_SETTING_P1;
    *settings++ = BYTEBEAT_SETTING_P2;
    *settings++ = BYTEBEAT_SETTING_LOOP_MODE;
    if (get_loop_mode()) {
      *settings++ = BYTEBEAT_SETTING_LOOP_START;
      *settings++ = BYTEBEAT_SETTING_LOOP_START_MED;
      *settings++ = BYTEBEAT_SETTING_LOOP_START_FINE;
      *settings++ = BYTEBEAT_SETTING_LOOP_END;
      *settings++ = BYTEBEAT_SETTING_LOOP_END_MED;
      *settings++ = BYTEBEAT_SETTING_LOOP_END_FINE;
    }
    *settings++ = BYTEBEAT_SETTING_TRIGGER_INPUT;
    *settings++ = BYTEBEAT_SETTING_STEP_MODE;
    *settings++ = BYTEBEAT_SETTING_CV1;
    *settings++ = BYTEBEAT_SETTING_CV2;
    *settings++ = BYTEBEAT_SETTING_CV3;
    *settings++ = BYTEBEAT_SETTING_CV4;

    num_enabled_settings_ = settings - enabled_settings_;
  }

  static bool indentSetting(ByteBeatSettings s) {
    switch (s) {
      case BYTEBEAT_SETTING_LOOP_START:
      case BYTEBEAT_SETTING_LOOP_START_MED:
      case BYTEBEAT_SETTING_LOOP_START_FINE:
      case BYTEBEAT_SETTING_LOOP_END:
      case BYTEBEAT_SETTING_LOOP_END_MED:
      case BYTEBEAT_SETTING_LOOP_END_FINE:
        return true;
      default: break;
    }
    return false;
  }
  // end conditional menu items infrastructure

  inline void apply_cv_mapping(ByteBeatSettings cv_setting, const int32_t cvs[ADC_CHANNEL_LAST], int32_t segments[kMaxByteBeatParameters]) {
    int mapping = values_[cv_setting];
    uint8_t bytebeat_cv_rshift = 12;
    switch (mapping) {
      case BYTEBEAT_CV_MAPPING_EQUATION:
      case BYTEBEAT_CV_MAPPING_SPEED:
      case BYTEBEAT_CV_MAPPING_P0:
      case BYTEBEAT_CV_MAPPING_P1:
      case BYTEBEAT_CV_MAPPING_P2:
        bytebeat_cv_rshift = 12;
        break;
      case BYTEBEAT_CV_MAPPING_LOOP_START:
      case BYTEBEAT_CV_MAPPING_LOOP_START_MED:
      case BYTEBEAT_CV_MAPPING_LOOP_START_FINE:
      case BYTEBEAT_CV_MAPPING_LOOP_END:
      case BYTEBEAT_CV_MAPPING_LOOP_END_MED:
      case BYTEBEAT_CV_MAPPING_LOOP_END_FINE:
        bytebeat_cv_rshift = 12;
      default:
        break;
    }
    segments[mapping - BYTEBEAT_CV_MAPPING_FIRST] += (cvs[cv_setting - BYTEBEAT_SETTING_CV1] * 65536) >> bytebeat_cv_rshift;
  }

  template <DAC_CHANNEL dac_channel>
  void Update(uint32_t triggers, const int32_t cvs[ADC_CHANNEL_LAST]) {

    int32_t s[kMaxByteBeatParameters];
    s[0] = SCALE8_16(static_cast<int32_t>(get_equation() << 4));
    s[1] = SCALE8_16(static_cast<int32_t>(get_speed()));
    s[2] = SCALE8_16(static_cast<int32_t>(get_p0()));
    s[3] = SCALE8_16(static_cast<int32_t>(get_p1()));
    s[4] = SCALE8_16(static_cast<int32_t>(get_p2()));
    s[5] = SCALE8_16(static_cast<int32_t>(get_loop_start()));
    s[6] = SCALE8_16(static_cast<int32_t>(get_loop_start_med()));
    s[7] = SCALE8_16(static_cast<int32_t>(get_loop_start_fine()));
    s[8] = SCALE8_16(static_cast<int32_t>(get_loop_end()));
    s[9] = SCALE8_16(static_cast<int32_t>(get_loop_end_med()));
    s[10] = SCALE8_16(static_cast<int32_t>(get_loop_end_fine()));

    apply_cv_mapping(BYTEBEAT_SETTING_CV1, cvs, s);
    apply_cv_mapping(BYTEBEAT_SETTING_CV2, cvs, s);
    apply_cv_mapping(BYTEBEAT_SETTING_CV3, cvs, s);
    apply_cv_mapping(BYTEBEAT_SETTING_CV4, cvs, s);

    for (uint_fast8_t i = 0; i < 11; ++i) {
      s[i] = USAT16(s[i]) ;
      s_[i] = s[i] ;
    }
       
    bytebeat_.Configure(s, get_step_mode(), get_loop_mode()) ; 

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
    uint16_t b = bytebeat_.ProcessSingleSample(gate_state);
    uint32_t value = OC::DAC::get_zero_offset(dac_channel) + (int16_t)b;
    OC::DAC::set<dac_channel>(value);


    b >>= 8;
    if (b != history_.last()) // This make the effect a bit different
      history_.Push(b);
  }

  inline void ReadHistory(uint8_t *history) const {
    history_.Read(history);
  }

private:
  peaks::ByteBeat bytebeat_;
  bool gate_raised_;
  int32_t s_[kMaxByteBeatParameters];

  int num_enabled_settings_;
  ByteBeatSettings enabled_settings_[BYTEBEAT_SETTING_LAST];

  util::History<uint8_t, kHistoryDepth> history_;
};

void ByteBeat::Init(OC::DigitalInput default_trigger) {
  InitDefaults();
  apply_value(BYTEBEAT_SETTING_TRIGGER_INPUT, default_trigger);
  bytebeat_.Init();
  gate_raised_ = false;
  update_enabled_settings();
  history_.Init(0);
}

const char* const bytebeat_cv_mapping_names[BYTEBEAT_CV_MAPPING_LAST] = {
  "off", "equ", "spd", "p0", "p1", "p2", "beg++", "beg+", "beg", "end++", "end+", "end"  
};

const char* const bytebeat_equation_names[] = {
  "hope", "love", "life", "age", "clysm", "monk", "NERV", "Trurl", "Pirx", "Snaut", "Hari" , "Kris", "Tichy", "Bregg", "Avon", "Orac"
};

SETTINGS_DECLARE(ByteBeat, BYTEBEAT_SETTING_LAST) {
  { 0, 0, 15, "Equation", bytebeat_equation_names, settings::STORAGE_TYPE_U8 },
  { 255, 0, 255, "Speed", NULL, settings::STORAGE_TYPE_U8 },
  { 126, 0, 255, "Parameter 0", NULL, settings::STORAGE_TYPE_U8 }, 
  { 126, 0, 255, "Parameter 1", NULL, settings::STORAGE_TYPE_U8 }, 
  { 127, 0, 255, "Parameter 2", NULL, settings::STORAGE_TYPE_U8 }, 
  { 0, 0, 1, "Loop mode", OC::Strings::no_yes, settings::STORAGE_TYPE_U4 },
  { 0, 0, 255, "Loop begin ++", NULL, settings::STORAGE_TYPE_U8 }, 
  { 0, 0, 255, "Loop begin +", NULL, settings::STORAGE_TYPE_U8 }, 
  { 0, 0, 255, "Loop begin", NULL, settings::STORAGE_TYPE_U8 }, 
  { 0, 0, 255, "Loop end ++", NULL, settings::STORAGE_TYPE_U8 }, 
  { 1, 0, 255, "Loop end +", NULL, settings::STORAGE_TYPE_U8 }, 
  { 255, 0, 255, "Loop end", NULL, settings::STORAGE_TYPE_U8 }, 
  { OC::DIGITAL_INPUT_1, OC::DIGITAL_INPUT_1, OC::DIGITAL_INPUT_4, "Trigger input", OC::Strings::trigger_input_names, settings::STORAGE_TYPE_U4 },
  { 0, 0, 1, "Step mode", OC::Strings::no_yes, settings::STORAGE_TYPE_U4 },
  { BYTEBEAT_CV_MAPPING_NONE, BYTEBEAT_CV_MAPPING_NONE, BYTEBEAT_CV_MAPPING_LAST - 1, "CV1 -> ", bytebeat_cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { BYTEBEAT_CV_MAPPING_NONE, BYTEBEAT_CV_MAPPING_NONE, BYTEBEAT_CV_MAPPING_LAST - 1, "CV2 -> ", bytebeat_cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { BYTEBEAT_CV_MAPPING_NONE, BYTEBEAT_CV_MAPPING_NONE, BYTEBEAT_CV_MAPPING_LAST - 1, "CV3 -> ", bytebeat_cv_mapping_names, settings::STORAGE_TYPE_U4 },
  { BYTEBEAT_CV_MAPPING_NONE, BYTEBEAT_CV_MAPPING_NONE, BYTEBEAT_CV_MAPPING_LAST - 1, "CV4 -> ", bytebeat_cv_mapping_names, settings::STORAGE_TYPE_U4 },
};

class QuadByteBeats {
public:
  static constexpr int32_t kCvSmoothing = 16;

  // bb = bytebeat, balls_ = bytebeats_, BouncingBall = Bytebeat
  // QuadBouncingBalls = QuadByteBeats, bbgen = bytebeatgen, BBGEN = BYTEBEATGEN

  void Init() {
    int input = OC::DIGITAL_INPUT_1;
    for (auto &bytebeat : bytebeats_) {
      bytebeat.Init(static_cast<OC::DigitalInput>(input));
      ++input;
    }

    ui.left_encoder_value = 0;
    ui.left_edit_mode = MODE_EDIT_SETTINGS;

    ui.selected_channel = 0;
    ui.selected_segment = 0;
    ui.cursor.Init(BYTEBEAT_SETTING_EQUATION, BYTEBEAT_SETTING_LAST - 1);
    ui.cursor.AdjustEnd(bytebeats_[0].num_enabled_settings() - 1);
  }

  void ISR() {
    cv1.push(OC::ADC::value<ADC_CHANNEL_1>());
    cv2.push(OC::ADC::value<ADC_CHANNEL_2>());
    cv3.push(OC::ADC::value<ADC_CHANNEL_3>());
    cv4.push(OC::ADC::value<ADC_CHANNEL_4>());

    const int32_t cvs[ADC_CHANNEL_LAST] = { cv1.value(), cv2.value(), cv3.value(), cv4.value() };
    uint32_t triggers = OC::DigitalInputs::clocked();

    bytebeats_[0].Update<DAC_CHANNEL_A>(triggers, cvs);
    bytebeats_[1].Update<DAC_CHANNEL_B>(triggers, cvs);
    bytebeats_[2].Update<DAC_CHANNEL_C>(triggers, cvs);
    bytebeats_[3].Update<DAC_CHANNEL_D>(triggers, cvs);
  }

  enum LeftEditMode {
    MODE_SELECT_CHANNEL,
    MODE_EDIT_SETTINGS
  };

  struct {
    LeftEditMode left_edit_mode;
    int left_encoder_value;
    // bool editing;

    int selected_channel;
    int selected_segment;
    menu::ScreenCursor<menu::kScreenLines> cursor;
  } ui;

  ByteBeat &selected() {
    return bytebeats_[ui.selected_channel];
  }

  ByteBeat bytebeats_[4];

  SmoothedValue<int32_t, kCvSmoothing> cv1;
  SmoothedValue<int32_t, kCvSmoothing> cv2;
  SmoothedValue<int32_t, kCvSmoothing> cv3;
  SmoothedValue<int32_t, kCvSmoothing> cv4;
};

QuadByteBeats bytebeatgen;

void BYTEBEATGEN_init() {
  bytebeatgen.Init();
}

size_t BYTEBEATGEN_storageSize() {
  return 4 * ByteBeat::storageSize();
}

size_t BYTEBEATGEN_save(void *storage) {
  size_t s = 0;
  for (auto &bytebeat : bytebeatgen.bytebeats_)
    s += bytebeat.Save(static_cast<byte *>(storage) + s);
  return s;
}

size_t BYTEBEATGEN_restore(const void *storage) {
  size_t s = 0;
  for (auto &bytebeat : bytebeatgen.bytebeats_) {
    s += bytebeat.Restore(static_cast<const byte *>(storage) + s);
    bytebeat.update_enabled_settings();
  }
  bytebeatgen.ui.cursor.AdjustEnd(bytebeatgen.bytebeats_[0].num_enabled_settings() - 1);
  return s;
}


void BYTEBEATGEN_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      bytebeatgen.ui.cursor.set_editing(false);
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
      break;
  }
}

void BYTEBEATGEN_loop() {
}

void BYTEBEATGEN_menu() {
  
  menu::QuadTitleBar::Draw();
  for (uint_fast8_t i = 0; i < 4; ++i) {
    menu::QuadTitleBar::SetColumn(i);
    graphics.print((char)('A' + i));
  }
  menu::QuadTitleBar::Selected(bytebeatgen.ui.selected_channel);

  auto const &bytebeat = bytebeatgen.selected();
  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(bytebeatgen.ui.cursor);
  menu::SettingsListItem list_item;
  while (settings_list.available()) {
    const int setting = bytebeat.enabled_setting_at(settings_list.Next(list_item));
    const int value = bytebeat.get_value(setting);
    const settings::value_attr &attr = ByteBeat::value_attr(setting);
    if (ByteBeat::indentSetting(static_cast<ByteBeatSettings>(setting)))
      list_item.x += menu::kIndentDx;
    list_item.DrawDefault(value, attr);
  }
}

void BYTEBEATGEN_topButton() {
  auto &selected_bytebeat = bytebeatgen.selected();
  selected_bytebeat.change_value(BYTEBEAT_SETTING_EQUATION + bytebeatgen.ui.selected_segment, 1);
}

void BYTEBEATGEN_lowerButton() {
  auto &selected_bytebeat = bytebeatgen.selected();
  selected_bytebeat.change_value(BYTEBEAT_SETTING_EQUATION + bytebeatgen.ui.selected_segment, -1); 
}

void BYTEBEATGEN_handleButtonEvent(const UI::Event &event) {
  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        BYTEBEATGEN_topButton();
        break;
      case OC::CONTROL_BUTTON_DOWN:
        BYTEBEATGEN_lowerButton();
        break;
      case OC::CONTROL_BUTTON_L:
        break;
      case OC::CONTROL_BUTTON_R:
        bytebeatgen.ui.cursor.toggle_editing();
        break;
    }
  }
}

void BYTEBEATGEN_handleEncoderEvent(const UI::Event &event) {

  if (OC::CONTROL_ENCODER_L == event.control) {
    int left_value = bytebeatgen.ui.selected_channel + event.value;
    CONSTRAIN(left_value, 0, 3);
    bytebeatgen.ui.selected_channel = left_value;
    auto &selected = bytebeatgen.selected();
    bytebeatgen.ui.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    if (bytebeatgen.ui.cursor.editing()) {
      auto &selected = bytebeatgen.selected();
      ByteBeatSettings setting = selected.enabled_setting_at(bytebeatgen.ui.cursor.cursor_pos());
      selected.change_value(setting, event.value);
      switch (setting) {
        case BYTEBEAT_SETTING_LOOP_MODE:
          selected.update_enabled_settings();
          bytebeatgen.ui.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
          break;
        default: break;
      }
    } else {
      bytebeatgen.ui.cursor.Scroll(event.value);
    }
  }
}

void BYTEBEATGEN_debug() {
  for (int i = 0; i < 4; ++i) { 
    uint8_t ypos = 10*(i + 1) + 2 ; 
    graphics.setPrintPos(2, ypos);
    graphics.print(i) ;
    graphics.setPrintPos(12, ypos);
    graphics.print("t=") ;
    graphics.setPrintPos(40, ypos);
    graphics.print(bytebeatgen.bytebeats_[i].get_eqn_num(), 12);
  }
}

uint8_t bb_history[ByteBeat::kHistoryDepth];

void BYTEBEATGEN_screensaver() {

  // Display raw history values "radiating" from center point by mirroring
  // on x and y. Oldest value is at start of buffer after reading history.

  weegfx::coord_t y = 0;
  for (const auto & bb : bytebeatgen.bytebeats_) {
    bb.ReadHistory(bb_history);
    const uint8_t *history = bb_history + ByteBeat::kHistoryDepth - 1;
    for (int i = 0; i < 64; ++i) {
      uint8_t b = *history-- ;
      graphics.drawAlignedByte(64 + i, y + 8, b);
      graphics.drawAlignedByte(64 - i -1, y + 8, b);
      b = util::reverse_byte(b);
      graphics.drawAlignedByte(64 + i, y, b);
      graphics.drawAlignedByte(64 - i -1, y, b);
    }
    y += 16;
  }
}

void FASTRUN BYTEBEATGEN_isr() {
  bytebeatgen.ISR();
}
