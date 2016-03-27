// Copyright (c)  2015, 2016 Patrick Dowling, Max Stadler, Tim Churches
//
// Author of original O+C firmware: Max Stadler (mxmlnstdlr@gmail.com)
// Author of app scaffolding: Patrick Dowling (pld@gurkenkiste.com)
// Modified for Lorenz and Rössler generators: Tim Churches (tim.churches@gmail.com)
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

// Lorenz and Rössler generator app

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
  "Lx1+Rz2",
  "Lx1xLy1",
  "Lx1xLx2",
  "Lx1xRx1",
  "Lx1xRx2",
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
  { 128, 0, 255, "Freq 1", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "Freq 2", NULL, settings::STORAGE_TYPE_U8 },
  { 63, 4, 127, "Rho/c 1", NULL, settings::STORAGE_TYPE_U8 }, 
  { 63, 4, 127, "Rho/c 2", NULL, settings::STORAGE_TYPE_U8 }, 
  {streams::LORENZ_OUTPUT_X1, streams::LORENZ_OUTPUT_X1, streams::LORENZ_OUTPUT_LAST - 1, "Out A ", lorenz_output_names, settings::STORAGE_TYPE_U8},
  {streams::LORENZ_OUTPUT_Y1, streams::LORENZ_OUTPUT_X1, streams::LORENZ_OUTPUT_LAST - 1, "Out B ", lorenz_output_names, settings::STORAGE_TYPE_U8},
  {streams::LORENZ_OUTPUT_X2, streams::LORENZ_OUTPUT_X1, streams::LORENZ_OUTPUT_LAST - 1, "Out C ", lorenz_output_names, settings::STORAGE_TYPE_U8},
  {streams::LORENZ_OUTPUT_Y2, streams::LORENZ_OUTPUT_X1, streams::LORENZ_OUTPUT_LAST - 1, "Out D ", lorenz_output_names, settings::STORAGE_TYPE_U8},
};

LorenzGenerator lorenz_generator;
struct {
  bool selected_generator;

  menu::ScreenCursor<menu::kScreenLines> cursor;
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
  lorenz_generator_state.selected_generator = 0; 
  lorenz_generator_state.cursor.Init(LORENZ_SETTING_RHO1, LORENZ_SETTING_LAST - 1);
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
}

void LORENZ_menu() {

  menu::DualTitleBar::Draw();
  graphics.print("Freq1 ");
  int32_t freq1 = SCALE8_16(lorenz_generator.get_freq1()) + (lorenz_generator.cv_freq1.value() * 16);
  freq1 = USAT16(freq1);
  graphics.print(freq1 >> 8);

  menu::DualTitleBar::SetColumn(1);
  graphics.print("Freq2 ");
  int32_t freq2 = SCALE8_16(lorenz_generator.get_freq2()) + (lorenz_generator.cv_freq2.value() * 16);
  freq2 = USAT16(freq2);
  graphics.print(freq2 >> 8);

  menu::DualTitleBar::Selected(lorenz_generator_state.selected_generator);

  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX - 12> settings_list(lorenz_generator_state.cursor);
  menu::SettingsListItem list_item;
  while (settings_list.available()) {
    const int current = settings_list.Next(list_item);
    list_item.DrawDefault(lorenz_generator.get_value(current), LorenzGenerator::value_attr(current));
  }
}

void LORENZ_screensaver() {
  OC::scope_render();
}

void LORENZ_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
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
  lorenz_generator_state.cursor.toggle_editing();
}

void LORENZ_leftButton() {
  lorenz_generator_state.selected_generator = 1 - lorenz_generator_state.selected_generator;
}

void LORENZ_handleButtonEvent(const UI::Event &event) {
  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        LORENZ_topButton();
        break;
      case OC::CONTROL_BUTTON_DOWN:
        LORENZ_lowerButton();
        break;
      case OC::CONTROL_BUTTON_L:
        LORENZ_leftButton();
        break;
      case OC::CONTROL_BUTTON_R:
        LORENZ_rightButton();
        break;
    }
  }
}

void LORENZ_handleEncoderEvent(const UI::Event &event) {

  if (OC::CONTROL_ENCODER_L == event.control) {
    if (lorenz_generator_state.selected_generator) {
      lorenz_generator.change_value(LORENZ_SETTING_FREQ2, event.value);
    } else {
      lorenz_generator.change_value(LORENZ_SETTING_FREQ1, event.value);
    }
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    if (lorenz_generator_state.cursor.editing()) {
      lorenz_generator.change_value(lorenz_generator_state.cursor.cursor_pos(), event.value);
    } else {
      lorenz_generator_state.cursor.Scroll(event.value);
    }
  }
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

}
