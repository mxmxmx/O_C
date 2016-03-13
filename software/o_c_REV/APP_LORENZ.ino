#include "streams_lorenz_generator.h"
#include "util/util_math.h"
#include "OC_digital_inputs.h"

enum LORENZ_SETTINGS {
  LORENZ_SETTING_FREQ1,
  LORENZ_SETTING_FREQ2,
  LORENZ_SETTING_RHO1,
  LORENZ_SETTING_RHO2,
  LORENZ_SETTING_OUT_A,
  LORENZ_SETTING_OUT_B,
  LORENZ_SETTING_OUT_C,
  LORENZ_SETTING_OUT_D,
  LORENZ_SETTING_LAST
};

enum ELorenzOutputMap {
  LORENZ_OUTPUT_X1,
  LORENZ_OUTPUT_Y1,
  LORENZ_OUTPUT_Z1,
  LORENZ_OUTPUT_X2,
  LORENZ_OUTPUT_Y2,
  LORENZ_OUTPUT_Z2,
  ROSSLER_OUTPUT_X1,
  ROSSLER_OUTPUT_Y1,
  ROSSLER_OUTPUT_Z1,
  ROSSLER_OUTPUT_X2,
  ROSSLER_OUTPUT_Y2,
  ROSSLER_OUTPUT_Z2,
  LORENZ_OUTPUT_LX1_PLUS_RX1,
  LORENZ_OUTPUT_LX1_PLUS_RZ1,
  LORENZ_OUTPUT_LX1_PLUS_LY2,
  LORENZ_OUTPUT_LX1_PLUS_LZ2,
  LORENZ_OUTPUT_LX1_PLUS_RX2,
  LORENZ_OUTPUT_LX1_PLUS_RZ2,
  LORENZ_OUTPUT_LAST,
};


const char * const lorenz_output_names[] = {
  "Lx1",
  "Ly1",
  "Lz1",
  "Lx2",
  "Ly2",
  "Lz2",
  "Rx1",
  "Ry1",
  "Rz1",
  "Rx2",
  "Ry2",
  "Rz2",
  "Lx1+Rx1",
  "Lx1+Rz1",
  "Lx1+Ly2",
  "Lx1+Lz2",
  "Lx1+Rx2",
  "Lx1_Rz2",
};

class LorenzGenerator : public settings::SettingsBase<LorenzGenerator, LORENZ_SETTING_LAST> {
public:

  uint16_t get_freq1() const {
    return values_[LORENZ_SETTING_FREQ1];
  }

  uint16_t get_freq2() const {
    return values_[LORENZ_SETTING_FREQ2];
  }

  uint16_t get_rho1() const {
    return values_[LORENZ_SETTING_RHO1];
  }

  uint16_t get_rho2() const {
    return values_[LORENZ_SETTING_RHO2];
  }

  uint8_t get_out_a() const {
    return values_[LORENZ_SETTING_OUT_A];
  }

  uint8_t get_out_b() const {
    return values_[LORENZ_SETTING_OUT_B];
  }

  uint8_t get_out_c() const {
    return values_[LORENZ_SETTING_OUT_C];
  }

  uint8_t get_out_d() const {
    return values_[LORENZ_SETTING_OUT_D];
  }

  void Init();

  void freeze() {
    frozen_ = true;
  }

  void thaw() {
    frozen_ = false;
  }

  bool frozen() const {
    return frozen_;
  }

  streams::LorenzGenerator lorenz;
  bool frozen_;

  // ISR update is at 16.666kHz, we don't need it that fast so smooth the values to ~1Khz
  static constexpr int32_t kSmoothing = 16;

  SmoothedValue<int32_t, kSmoothing> cv_freq1;
  SmoothedValue<int32_t, kSmoothing> cv_freq2;
  SmoothedValue<int32_t, kSmoothing> cv_rho1;
  SmoothedValue<int32_t, kSmoothing> cv_rho2;
};

void LorenzGenerator::Init() {
  InitDefaults();
  lorenz.Init(0);
  lorenz.Init(1);
  frozen_= false;
}

SETTINGS_DECLARE(LorenzGenerator, LORENZ_SETTING_LAST) {
  { 128, 0, 255, "FREQ 1", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "FREQ 2", NULL, settings::STORAGE_TYPE_U8 },
  { 63, 4, 127, "RHO/C 1", NULL, settings::STORAGE_TYPE_U8 }, 
  { 63, 4, 127, "RHO/C 2", NULL, settings::STORAGE_TYPE_U8 }, 
  {LORENZ_OUTPUT_X1, LORENZ_OUTPUT_X1, LORENZ_OUTPUT_LAST - 1, "outA ", lorenz_output_names, settings::STORAGE_TYPE_U8},
  {LORENZ_OUTPUT_Y1, LORENZ_OUTPUT_X1, LORENZ_OUTPUT_LAST - 1, "outB ", lorenz_output_names, settings::STORAGE_TYPE_U8},
  {LORENZ_OUTPUT_X2, LORENZ_OUTPUT_X1, LORENZ_OUTPUT_LAST - 1, "outC ", lorenz_output_names, settings::STORAGE_TYPE_U8},
  {LORENZ_OUTPUT_Y2, LORENZ_OUTPUT_X1, LORENZ_OUTPUT_LAST - 1, "outD ", lorenz_output_names, settings::STORAGE_TYPE_U8},
};

LorenzGenerator lorenz_generator;
struct {
  int selected_param;
  bool selected_generator;
  bool editing;
} lorenz_generator_state;

void FASTRUN LORENZ_isr() {

  bool reset1_phase = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>();
  bool reset2_phase = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_2>();
  bool reset_both_phase = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_3>();
  bool freeze = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_4>();

  lorenz_generator.cv_freq1.push(OC::ADC::value<ADC_CHANNEL_1>());
  lorenz_generator.cv_rho1.push(OC::ADC::value<ADC_CHANNEL_2>());
  lorenz_generator.cv_freq2.push(OC::ADC::value<ADC_CHANNEL_3>());
  lorenz_generator.cv_rho2.push(OC::ADC::value<ADC_CHANNEL_4>());

  // Range in settings is (0-256] so this gets scaled to (0,65535]
  // CV value is 12 bit so also needs scaling

  int32_t freq1 = SCALE8_16(lorenz_generator.get_freq1()) + (lorenz_generator.cv_freq1.value() * 16);
  freq1 = USAT16(freq1);

  int32_t freq2 = SCALE8_16(lorenz_generator.get_freq2()) + (lorenz_generator.cv_freq2.value() * 16);
  freq2 = USAT16(freq2);

  const int32_t rho_lower_limit = 4 << 8 ;
  const int32_t rho_upper_limit = 127 << 8 ;

  int32_t rho1 = SCALE8_16(lorenz_generator.get_rho1()) + (lorenz_generator.cv_rho1.value() * 16) ;
  if (rho1 < rho_lower_limit) rho1 = rho_lower_limit;
  else if (rho1 > rho_upper_limit) rho1 = rho_upper_limit ;
  lorenz_generator.lorenz.set_rho1(USAT16(rho1));

  int32_t rho2 = SCALE8_16(lorenz_generator.get_rho2()) + (lorenz_generator.cv_rho2.value() * 16) ;
  if (rho2 < rho_lower_limit) rho2 = rho_lower_limit;
  else if (rho2 > rho_upper_limit) rho2 = rho_upper_limit ;
  lorenz_generator.lorenz.set_rho2(USAT16(rho2));

  uint8_t out_a = lorenz_generator.get_out_a() ;
  lorenz_generator.lorenz.set_out_a(out_a);

  uint8_t out_b = lorenz_generator.get_out_b() ;
  lorenz_generator.lorenz.set_out_b(out_b);

  uint8_t out_c = lorenz_generator.get_out_c() ;
  lorenz_generator.lorenz.set_out_c(out_c);

  uint8_t out_d = lorenz_generator.get_out_d() ;
  lorenz_generator.lorenz.set_out_d(out_d);

  if (reset_both_phase) {
    reset1_phase = true ;
    reset2_phase = true ;
  }
  if (!freeze && !lorenz_generator.frozen())
    lorenz_generator.lorenz.Process(freq1, freq2, reset1_phase, reset2_phase);

  DAC::set<DAC_CHANNEL_A>(lorenz_generator.lorenz.dac_code(0));
  DAC::set<DAC_CHANNEL_B>(lorenz_generator.lorenz.dac_code(1));
  DAC::set<DAC_CHANNEL_C>(lorenz_generator.lorenz.dac_code(2));
  DAC::set<DAC_CHANNEL_D>(lorenz_generator.lorenz.dac_code(3));
}

void LORENZ_init() {
  lorenz_generator_state.selected_param = LORENZ_SETTING_RHO1;
  lorenz_generator_state.selected_generator = 0; 
  lorenz_generator_state.editing = false;
  lorenz_generator.Init();
}

size_t LORENZ_storageSize() {
  return LorenzGenerator::storageSize();
}

size_t LORENZ_save(void *storage) {
  return lorenz_generator.Save(storage);
}

size_t LORENZ_restore(const void *storage) {
  return lorenz_generator.Restore(storage);
}

void LORENZ_loop() {
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  buttons(BUTTON_TOP);
  buttons(BUTTON_BOTTOM);
  buttons(BUTTON_LEFT);
  buttons(BUTTON_RIGHT);
}

void LORENZ_menu() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem

  graphics.setFont(UI_DEFAULT_FONT);

  static const weegfx::coord_t kStartX = 0;
  UI_DRAW_TITLE(kStartX);
  graphics.setPrintPos(2, 2);
  graphics.print("FREQ1 ");
  int32_t freq1 = SCALE8_16(lorenz_generator.get_freq1()) + (lorenz_generator.cv_freq1.value() * 16);
  freq1 = USAT16(freq1);
   graphics.print(freq1 >> 8);
  graphics.setPrintPos(66, 2);
  graphics.print("FREQ2 ");
  int32_t freq2 = SCALE8_16(lorenz_generator.get_freq2()) + (lorenz_generator.cv_freq2.value() * 16);
  freq2 = USAT16(freq2);
  graphics.print(freq2 >> 8);
  if (lorenz_generator_state.selected_generator) {
      graphics.invertRect(66, 0, 127, 10);
  } else {
      graphics.invertRect(2, 0, 64, 10);    
  }
  

  int first_visible_param = lorenz_generator_state.selected_param - 3;
  if (first_visible_param < LORENZ_SETTING_RHO1)
    first_visible_param = LORENZ_SETTING_RHO1;

  UI_START_MENU(kStartX);
  UI_BEGIN_ITEMS_LOOP(kStartX, first_visible_param, LORENZ_SETTING_LAST, lorenz_generator_state.selected_param, 0)
    UI_DRAW_EDITABLE(lorenz_generator_state.editing);
    const settings::value_attr &attr = LorenzGenerator::value_attr(current_item);
    UI_DRAW_SETTING(attr, lorenz_generator.get_value(current_item), kUiWideMenuCol1X-12);
  UI_END_ITEMS_LOOP();

  GRAPHICS_END_FRAME();
}

void LORENZ_screensaver() {
  GRAPHICS_BEGIN_FRAME(false);

  scope_render();

  GRAPHICS_END_FRAME();
}

void LORENZ_handleEvent(OC::AppEvent event) {
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

void LORENZ_topButton() {
  if (lorenz_generator_state.selected_generator) {
    lorenz_generator.change_value(LORENZ_SETTING_FREQ2, 32);
  } else {
    lorenz_generator.change_value(LORENZ_SETTING_FREQ1, 32);
  }
}

void LORENZ_lowerButton() {
  if (lorenz_generator_state.selected_generator) {
    lorenz_generator.change_value(LORENZ_SETTING_FREQ2, -32);
  } else {
    lorenz_generator.change_value(LORENZ_SETTING_FREQ1, -32);
  }
}

void LORENZ_rightButton() {
  lorenz_generator_state.editing = !lorenz_generator_state.editing;
}

void LORENZ_leftButton() {
  lorenz_generator_state.selected_generator = 1 - lorenz_generator_state.selected_generator;
}

void LORENZ_leftButtonLong() {
}

bool LORENZ_encoders() {
  bool changed = false;
  int value = encoder[LEFT].pos();
  encoder[LEFT].setPos(0);
  if (value) {
    if (lorenz_generator_state.selected_generator) {
      lorenz_generator.change_value(LORENZ_SETTING_FREQ2, value);
    } else {
      lorenz_generator.change_value(LORENZ_SETTING_FREQ1, value);
    }
    changed = true;
  }

  value = encoder[RIGHT].pos();
  encoder[RIGHT].setPos(0);
  if (value) {
    if (lorenz_generator_state.editing) {
      lorenz_generator.change_value(lorenz_generator_state.selected_param, value);
    } else {
      value += lorenz_generator_state.selected_param;
      CONSTRAIN(value, LORENZ_SETTING_RHO1, LORENZ_SETTING_LAST - 1);
      lorenz_generator_state.selected_param = value;
    }
    changed = true;
  }

  return changed;
}

void LORENZ_debug() {
  graphics.setPrintPos(2, 12);
  graphics.print(lorenz_generator.cv_freq1.value());
  graphics.print(" ");
  int32_t value = SCALE8_16(lorenz_generator.get_freq1());
  graphics.print(value);
  graphics.setPrintPos(2, 22);
  graphics.print(lorenz_generator.cv_freq2.value());
  graphics.print(" ");
  value = SCALE8_16(lorenz_generator.get_freq2());
  graphics.print(value);
//  graphics.print(" ");
//  graphics.print(lorenz_generator.cv_shape.value() * 16);
//  value += lorenz_generator.cv_shape.value() * 16;
//  graphics.setPrintPos(2, 22);
//  graphics.print(value); graphics.print(" ");
//  value = USAT16(value);
//  graphics.print(value);
}
