
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
  BYTEBEAT_SETTING_LOOP_START_FINE,
  BYTEBEAT_SETTING_LOOP_END,
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
  BYTEBEAT_CV_MAPPING_LOOP_START_FINE,
  BYTEBEAT_CV_MAPPING_LOOP_END,
  BYTEBEAT_CV_MAPPING_LOOP_END_FINE,
  BYTEBEAT_CV_MAPPING_LAST,
  BYTEBEAT_CV_MAPPING_FIRST=BYTEBEAT_CV_MAPPING_EQUATION
  
};

class ByteBeat : public settings::SettingsBase<ByteBeat, BYTEBEAT_SETTING_LAST> {
public:

  static constexpr int kMaxByteBeatParameters = 9;

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

  uint8_t get_loop_start_fine() const {
    return values_[BYTEBEAT_SETTING_LOOP_START_FINE];
  }

  uint8_t get_loop_end() const {
    return values_[BYTEBEAT_SETTING_LOOP_END];
  }

  uint8_t get_loop_end_fine() const {
    return values_[BYTEBEAT_SETTING_LOOP_END_FINE];
  }

  int32_t get_s(uint8_t param) {
    return s_[param] ; 
  }

  uint32_t get_t() {
    return static_cast<uint32_t>(bytebeat_.get_t()) ; 
  }

  uint32_t get_phase() {
    return static_cast<uint32_t>(bytebeat_.get_phase()) ; 
  }

  uint32_t get_instance_loop_start() {
    return static_cast<uint32_t>(bytebeat_.get_loop_start()) ; 
  }

  uint32_t get_instance_loop_end() {
    return static_cast<uint32_t>(bytebeat_.get_loop_end()) ; 
  }

  uint16_t get_bytepitch() {
    return static_cast<uint16_t>(bytebeat_.get_bytepitch()) ; 
  }

  inline void apply_cv_mapping(ByteBeatSettings cv_setting, const int32_t cvs[ADC_CHANNEL_LAST], int32_t segments[kMaxByteBeatParameters]) {
    int mapping = values_[cv_setting];
    uint8_t bytebeat_cv_rshift = 12 ;
    switch (mapping) {
      case BYTEBEAT_CV_MAPPING_EQUATION:
      case BYTEBEAT_CV_MAPPING_SPEED:
      case BYTEBEAT_CV_MAPPING_P0:
      case BYTEBEAT_CV_MAPPING_P1:
      case BYTEBEAT_CV_MAPPING_P2:
        bytebeat_cv_rshift = 12 ;
        break;
      case BYTEBEAT_CV_MAPPING_LOOP_START:
      case BYTEBEAT_CV_MAPPING_LOOP_START_FINE:
      case BYTEBEAT_CV_MAPPING_LOOP_END:
      case BYTEBEAT_CV_MAPPING_LOOP_END_FINE:
        bytebeat_cv_rshift = 12 ;
      default:
        break;
    }
    segments[mapping - BYTEBEAT_CV_MAPPING_FIRST] += (cvs[cv_setting - BYTEBEAT_SETTING_CV1] * 65536) >> bytebeat_cv_rshift ;
  }

  template <DAC_CHANNEL dac_channel>
  void Update(uint32_t triggers, const int32_t cvs[ADC_CHANNEL_LAST]) {

    int32_t s[kMaxByteBeatParameters];
    s[0] = SCALE8_16(static_cast<int32_t>(get_equation() << 6));
    s[1] = SCALE8_16(static_cast<int32_t>(get_speed()));
    s[2] = SCALE8_16(static_cast<int32_t>(get_p0()));
    s[3] = SCALE8_16(static_cast<int32_t>(get_p1()));
    s[4] = SCALE8_16(static_cast<int32_t>(get_p2()));
    s[5] = SCALE8_16(static_cast<int32_t>(get_loop_start()));
    s[6] = SCALE8_16(static_cast<int32_t>(get_loop_start_fine()));
    s[7] = SCALE8_16(static_cast<int32_t>(get_loop_end()));
    s[8] = SCALE8_16(static_cast<int32_t>(get_loop_end_fine()));

    apply_cv_mapping(BYTEBEAT_SETTING_CV1, cvs, s);
    apply_cv_mapping(BYTEBEAT_SETTING_CV2, cvs, s);
    apply_cv_mapping(BYTEBEAT_SETTING_CV3, cvs, s);
    apply_cv_mapping(BYTEBEAT_SETTING_CV4, cvs, s);

    s[0] = USAT16(s[0]);
    s[1] = USAT16(s[1]);
    s[2] = USAT16(s[2]);
    s[3] = USAT16(s[3]);
    s[4] = USAT16(s[4]);
    s[5] = USAT16(s[5]);
    s[6] = USAT16(s[6]);
    s[7] = USAT16(s[7]);
    s[8] = USAT16(s[8]);

    s_[0] = s[0] ;
    s_[1] = s[1] ;
    s_[2] = s[2] ;
    s_[3] = s[3] ;
    s_[4] = s[4] ;
    s_[5] = s[5] ;
    s_[6] = s[6] ;
    s_[7] = s[7] ;
    s_[8] = s[8] ;
        
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
    uint32_t value = OC::calibration_data.dac.octaves[_ZERO] + bytebeat_.ProcessSingleSample(gate_state);
    DAC::set<dac_channel>(value);
  }


private:
  peaks::ByteBeat bytebeat_;
  bool gate_raised_;
  int32_t s_[kMaxByteBeatParameters];
};

void ByteBeat::Init(OC::DigitalInput default_trigger) {
  InitDefaults();
  apply_value(BYTEBEAT_SETTING_TRIGGER_INPUT, default_trigger);
  bytebeat_.Init();
  gate_raised_ = false;
}

const char* const bytebeat_cv_mapping_names[BYTEBEAT_CV_MAPPING_LAST] = {
  "off", "equ", "spd", "p0", "p1", "p2", "beg+", "beg", "end+", "end"  
};

const char* const bytebeat_equation_names[] = {
  "hope", "love", "life", "pain" 
};

SETTINGS_DECLARE(ByteBeat, BYTEBEAT_SETTING_LAST) {
  { 0, 0, 3, "Equation", bytebeat_equation_names, settings::STORAGE_TYPE_U8 },
  { 255, 0, 255, "Speed", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "Parameter 0", NULL, settings::STORAGE_TYPE_U8 }, 
  { 128, 0, 255, "Parameter 1", NULL, settings::STORAGE_TYPE_U8 }, 
  { 128, 0, 255, "Parameter 2", NULL, settings::STORAGE_TYPE_U8 }, 
  { 0, 0, 1, "Loop mode", OC::Strings::no_yes, settings::STORAGE_TYPE_U4 },
  { 0, 0, 255, "Loop begin ++", NULL, settings::STORAGE_TYPE_U8 }, 
  { 0, 0, 255, "Loop begin", NULL, settings::STORAGE_TYPE_U8 }, 
  { 1, 0, 255, "Loop end ++", NULL, settings::STORAGE_TYPE_U8 }, 
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
    // ui.editing = false;
    ui.selected_channel = 0;
    ui.selected_segment = 0;
    ui.cursor.Init(BYTEBEAT_SETTING_EQUATION, BYTEBEAT_SETTING_LAST - 1);
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
  for (auto &bytebeat : bytebeatgen.bytebeats_)
    s += bytebeat.Restore(static_cast<const byte *>(storage) + s);
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
    const int current = settings_list.Next(list_item);
    list_item.DrawDefault(bytebeat.get_value(current), ByteBeat::value_attr(current));
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
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    if (bytebeatgen.ui.cursor.editing()) {
      auto &selected = bytebeatgen.selected();
      selected.change_value(bytebeatgen.ui.cursor.cursor_pos(), event.value);
    } else {
      bytebeatgen.ui.cursor.Scroll(event.value);
    }
  }
}

void BYTEBEATGEN_debug() {
  graphics.setPrintPos(2, 12);
  graphics.print(bytebeatgen.bytebeats_[0].get_phase(), 12);
  // graphics.print(bytebeatgen.bytebeats_[0].get_s(0));

  graphics.setPrintPos(2, 22);
  graphics.print(bytebeatgen.bytebeats_[0].get_t(), 12);
  
  graphics.setPrintPos(2, 32);
  // graphics.print(bytebeatgen.bytebeats_[0].get_bytepitch());
  // graphics.print(bytebeatgen.bytebeats_[0].get_loop_start(), 16);
  // graphics.setPrintPos(66, 32);
  graphics.print(bytebeatgen.bytebeats_[0].get_s(5));

  graphics.setPrintPos(2, 42);
  // graphics.print(bytebeatgen.bytebeats_[0].get_speed());
  // graphics.setPrintPos(66, 42);
  graphics.print(bytebeatgen.bytebeats_[0].get_s(6));
}

void BYTEBEATGEN_screensaver() {
  OC::scope_render();
}

void FASTRUN BYTEBEATGEN_isr() {
  bytebeatgen.ISR();
}


