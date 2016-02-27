#include "halvorsen_generator.h"
#include "util/util_math.h"
#include "OC_digital_inputs.h"

enum HALVORSEN_SETTINGS {
  HALVORSEN_SETTING_FREQ1,
  HALVORSEN_SETTING_FREQ2,
  HALVORSEN_SETTING_ALPHA1,
  HALVORSEN_SETTING_ALPHA2,
  HALVORSEN_SETTING_OUT_C,
  HALVORSEN_SETTING_OUT_D,
  HALVORSEN_SETTING_LAST
};

enum EHalvorsenOutputMap {
  HALVORSEN_OUTPUT_X1,
  HALVORSEN_OUTPUT_Y1,
  HALVORSEN_OUTPUT_Z1,
  HALVORSEN_OUTPUT_X2,
  HALVORSEN_OUTPUT_Y2,
  HALVORSEN_OUTPUT_Z2,
  HALVORSEN_OUTPUT_X1_PLUS_Y1,
  HALVORSEN_OUTPUT_X1_PLUS_Z1,
  HALVORSEN_OUTPUT_Y1_PLUS_Z1,
  HALVORSEN_OUTPUT_X2_PLUS_Y2,
  HALVORSEN_OUTPUT_X2_PLUS_Z2,
  HALVORSEN_OUTPUT_Y2_PLUS_Z2,
  HALVORSEN_OUTPUT_X1_PLUS_X2,
  HALVORSEN_OUTPUT_X1_PLUS_Y2,
  HALVORSEN_OUTPUT_X1_PLUS_Z2,
  HALVORSEN_OUTPUT_Y1_PLUS_X2,
  HALVORSEN_OUTPUT_Y1_PLUS_Y2,
  HALVORSEN_OUTPUT_Y1_PLUS_Z2,
  HALVORSEN_OUTPUT_Z1_PLUS_X2,
  HALVORSEN_OUTPUT_Z1_PLUS_Y2,
  HALVORSEN_OUTPUT_Z1_PLUS_Z2,
  HALVORSEN_OUTPUT_LAST,
};

const char * const halvorsen_output_names[] = {
  "x1",
  "y1",
  "z1",
  "x2",
  "y2",
  "z2",
  "x1+y1",
  "x1+z1",
  "y1+z1",
  "x2+y2",
  "x2+z2",
  "y2+z2",
  "x1+x2",
  "x1+y2",
  "x1+z2",
  "y1+x2",
  "y1+y2",
  "y1+z2",
  "z1+x2",
  "z1+y2",
  "z1+z2",
};

class HalvorsenGenerator : public settings::SettingsBase<HalvorsenGenerator, HALVORSEN_SETTING_LAST> {
public:

  uint16_t get_freq1() const {
    return values_[HALVORSEN_SETTING_FREQ1];
  }

  uint16_t get_freq2() const {
    return values_[HALVORSEN_SETTING_FREQ2];
  }

  uint16_t get_alpha1() const {
    return values_[HALVORSEN_SETTING_ALPHA1];
  }

  uint16_t get_alpha2() const {
    return values_[HALVORSEN_SETTING_ALPHA2];
  }

  uint8_t get_out_c() const {
    return values_[HALVORSEN_SETTING_OUT_C];
  }

  uint8_t get_out_d() const {
    return values_[HALVORSEN_SETTING_OUT_D];
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

  streams::HalvorsenGenerator halvorsen;
  bool frozen_;

  // ISR update is at 16.666kHz, we don't need it that fast so smooth the values to ~1Khz
  static constexpr int32_t kSmoothing = 16;

  SmoothedValue<int32_t, kSmoothing> cv_freq1;
  SmoothedValue<int32_t, kSmoothing> cv_freq2;
  SmoothedValue<int32_t, kSmoothing> cv_alpha1;
  SmoothedValue<int32_t, kSmoothing> cv_alpha2;
};

void HalvorsenGenerator::Init() {
  InitDefaults();
  halvorsen.Init();
  frozen_= false;
}

SETTINGS_DECLARE(HalvorsenGenerator, HALVORSEN_SETTING_LAST) {
  { 0, 0, 255, "FREQ 1", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 255, "FREQ 2", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 64, 192, "ALPHA 1", NULL, settings::STORAGE_TYPE_U8 }, // 128 is sweet spot
  { 128, 64, 192, "ALPHA 2", NULL, settings::STORAGE_TYPE_U8 }, // 128 is sweet spot
  {HALVORSEN_OUTPUT_X2, HALVORSEN_OUTPUT_X1, HALVORSEN_OUTPUT_LAST - 1, "outC ", halvorsen_output_names, settings::STORAGE_TYPE_U8},
  {HALVORSEN_OUTPUT_Y2, HALVORSEN_OUTPUT_X1, HALVORSEN_OUTPUT_LAST - 1, "outD ", halvorsen_output_names, settings::STORAGE_TYPE_U8},
};

HalvorsenGenerator halvorsen_generator;
struct {
  int selected_param;
  bool selected_generator;
} halvorsen_generator_state;

void FASTRUN HALVORSEN_isr() {

  bool reset_phase = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>();
  bool freeze = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_2>();
  // bool reset_phase2 = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_3>();
  //bool freeze2 = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_4>();

  halvorsen_generator.cv_freq1.push(OC::ADC::value<ADC_CHANNEL_1>());
  halvorsen_generator.cv_alpha1.push(OC::ADC::value<ADC_CHANNEL_2>());
  halvorsen_generator.cv_freq2.push(OC::ADC::value<ADC_CHANNEL_3>());
  halvorsen_generator.cv_alpha2.push(OC::ADC::value<ADC_CHANNEL_4>());

  // Range in settings is (0-256] so this gets scaled to (0,65535]
  // CV value is 12 bit so also needs scaling

  int32_t freq1 = SCALE8_16(halvorsen_generator.get_freq1()) + (halvorsen_generator.cv_freq1.value() * 16);
  freq1 = USAT16(freq1);

  int32_t freq2 = SCALE8_16(halvorsen_generator.get_freq2()) + (halvorsen_generator.cv_freq2.value() * 16);
  freq2 = USAT16(freq2);

  const int32_t alpha_lower_limit = 64 << 8 ;
  const int32_t alpha_upper_limit = 192 << 8 ;

  int32_t alpha1 = SCALE8_16(halvorsen_generator.get_alpha1()) + halvorsen_generator.cv_alpha1.value() ;
  if (alpha1 < alpha_lower_limit) alpha1 = alpha_lower_limit;
  else if (alpha1 > alpha_upper_limit) alpha1 = alpha_upper_limit ;
  halvorsen_generator.halvorsen.set_alpha1(USAT16(alpha1));

  int32_t alpha2 = SCALE8_16(halvorsen_generator.get_alpha2()) + halvorsen_generator.cv_alpha2.value() ;
  if (alpha2 < alpha_lower_limit) alpha2 = alpha_lower_limit;
  else if (alpha2 > alpha_upper_limit) alpha2 = alpha_upper_limit ;
  halvorsen_generator.halvorsen.set_alpha2(USAT16(alpha2));

  uint8_t out_c = halvorsen_generator.get_out_c() ;
  halvorsen_generator.halvorsen.set_out_c(out_c);

  uint8_t out_d = halvorsen_generator.get_out_d() ;
  halvorsen_generator.halvorsen.set_out_d(out_d);

  if (!freeze && !halvorsen_generator.frozen())
    halvorsen_generator.halvorsen.Process(freq1, freq2, reset_phase);

  DAC::set<DAC_CHANNEL_A>(halvorsen_generator.halvorsen.dac_code(0));
  DAC::set<DAC_CHANNEL_B>(halvorsen_generator.halvorsen.dac_code(1));
  DAC::set<DAC_CHANNEL_C>(halvorsen_generator.halvorsen.dac_code(2));
  DAC::set<DAC_CHANNEL_D>(halvorsen_generator.halvorsen.dac_code(3));
}

void HALVORSEN_init() {
  halvorsen_generator_state.selected_param = HALVORSEN_SETTING_ALPHA1;
  halvorsen_generator.Init();
}

size_t HALVORSEN_storageSize() {
  return HalvorsenGenerator::storageSize();
}

size_t HALVORSEN_save(void *storage) {
  return halvorsen_generator.Save(storage);
}

size_t HALVORSEN_restore(const void *storage) {
  return halvorsen_generator.Restore(storage);
}

void HALVORSEN_loop() {
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  buttons(BUTTON_TOP);
  buttons(BUTTON_BOTTOM);
  buttons(BUTTON_LEFT);
  buttons(BUTTON_RIGHT);
}

void HALVORSEN_menu() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem

  graphics.setFont(UI_DEFAULT_FONT);

  static const weegfx::coord_t kStartX = 0;
  UI_DRAW_TITLE(kStartX);
  graphics.setPrintPos(2, 2);
  graphics.print("FREQ1 ");
  int32_t freq1 = SCALE8_16(halvorsen_generator.get_freq1()) + (halvorsen_generator.cv_freq1.value() * 16);
  freq1 = USAT16(freq1);
  // graphics.print(halvorsen_generator.get_value(HALVORSEN_SETTING_FREQ1) + (halvorsen_generator.cv_freq1.value() >> 4));
  graphics.print(freq1 >> 8);
  graphics.setPrintPos(66, 2);
  graphics.print("FREQ2 ");
  int32_t freq2 = SCALE8_16(halvorsen_generator.get_freq2()) + (halvorsen_generator.cv_freq2.value() * 16);
  freq2 = USAT16(freq2);
  // graphics.print(halvorsen_generator.get_value(HALVORSEN_SETTING_FREQ2) + (halvorsen_generator.cv_freq2.value() >> 4));
  graphics.print(freq2 >> 8);
 if (halvorsen_generator_state.selected_generator) {
      graphics.invertRect(66, 0, 127, 10);
  } else {
      graphics.invertRect(2, 0, 64, 10);    
  }
  

  int first_visible_param = HALVORSEN_SETTING_ALPHA1; 

  UI_START_MENU(kStartX);
  UI_BEGIN_ITEMS_LOOP(kStartX, first_visible_param, HALVORSEN_SETTING_LAST, halvorsen_generator_state.selected_param, 0)
    const settings::value_attr &attr = HalvorsenGenerator::value_attr(current_item);
    UI_DRAW_SETTING(attr, halvorsen_generator.get_value(current_item), kUiWideMenuCol1X);
  UI_END_ITEMS_LOOP();

  GRAPHICS_END_FRAME();
}

// static const size_t kLargePreviewBufferSize = 64;
// uint16_t large_preview_buffer[kLargePreviewBufferSize];

// weegfx::coord_t scanner = 0;
// unsigned scanner_millis = 0;
// static const unsigned SCANNER_RATE = 200;

void HALVORSEN_screensaver() {
  GRAPHICS_BEGIN_FRAME(false);

//  uint16_t shape = poly_lfo.get_value(POLYLFO_SETTING_SHAPE) << 8;
//  poly_lfo.lfo.RenderPreview(shape, large_preview_buffer, kLargePreviewBufferSize);
//  for (weegfx::coord_t x = 0; x < 128; ++x) {
//    graphics.setPixel(x, 32 - (large_preview_buffer[(x + scanner) & 63] >> 11));
//  }
//  if (millis() - scanner_millis > SCANNER_RATE) {
//    ++scanner;
//    scanner_millis = millis();
//  }

// halvorsen_scope_render();
scope_render();

  GRAPHICS_END_FRAME();
}

void HALVORSEN_handleEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      if (halvorsen_generator_state.selected_generator) {
        encoder[LEFT].setPos(halvorsen_generator.get_value(HALVORSEN_SETTING_FREQ2));    
      } else {
        encoder[LEFT].setPos(halvorsen_generator.get_value(HALVORSEN_SETTING_FREQ1));
      }
      encoder[RIGHT].setPos(halvorsen_generator.get_value(halvorsen_generator_state.selected_param));
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER:
      break;
  }
}

void HALVORSEN_topButton() {
//  halvorsen_generator.change_value(HALVORSEN_SETTING_FREQ, 32);
//  encoder[LEFT].setPos(halvorsen_generator.get_value(HALVORSEN_SETTING_FREQ));
  halvorsen_generator.halvorsen.Init();
}

void HALVORSEN_lowerButton() {
//  halvorsen_generator.change_value(HALVORSEN_SETTING_FREQ, -32);
//  encoder[LEFT].setPos(halvorsen_generator.get_value(HALVORSEN_SETTING_FREQ));
}

void HALVORSEN_rightButton() {
  ++halvorsen_generator_state.selected_param;
  if (halvorsen_generator_state.selected_param >= HALVORSEN_SETTING_LAST)
    halvorsen_generator_state.selected_param = HALVORSEN_SETTING_ALPHA1;
  encoder[RIGHT].setPos(halvorsen_generator.get_value(halvorsen_generator_state.selected_param));
}

void HALVORSEN_leftButton() {
  halvorsen_generator_state.selected_generator = !halvorsen_generator_state.selected_generator;
  if (halvorsen_generator_state.selected_generator) {
        encoder[LEFT].setPos(halvorsen_generator.get_value(HALVORSEN_SETTING_FREQ2));
  } else {
        encoder[LEFT].setPos(halvorsen_generator.get_value(HALVORSEN_SETTING_FREQ1));
  }

}

void HALVORSEN_leftButtonLong() {
}

bool HALVORSEN_encoders() {
  bool changed = false;
  int value = encoder[LEFT].pos();
  if (halvorsen_generator_state.selected_generator) {
    if (value != halvorsen_generator.get_value(HALVORSEN_SETTING_FREQ2)) {
      halvorsen_generator.apply_value(HALVORSEN_SETTING_FREQ2, value);
      encoder[LEFT].setPos(halvorsen_generator.get_value(HALVORSEN_SETTING_FREQ2));
      changed = true;
    }
  } else {
    if (value != halvorsen_generator.get_value(HALVORSEN_SETTING_FREQ1)) {
      halvorsen_generator.apply_value(HALVORSEN_SETTING_FREQ1, value);
      encoder[LEFT].setPos(halvorsen_generator.get_value(HALVORSEN_SETTING_FREQ1));
      changed = true;
    }
  }

  value = encoder[RIGHT].pos();
  if (value != halvorsen_generator.get_value(halvorsen_generator_state.selected_param)) {
    halvorsen_generator.apply_value(halvorsen_generator_state.selected_param, value);
    encoder[RIGHT].setPos(halvorsen_generator.get_value(halvorsen_generator_state.selected_param));
    changed = true;
  }

  return changed;
}

void HALVORSEN_debug() {
  graphics.setPrintPos(2, 12);
  graphics.print(halvorsen_generator.cv_freq1.value());
  graphics.print(" ");
  int32_t value = SCALE8_16(halvorsen_generator.get_freq1());
  graphics.print(value);
  graphics.setPrintPos(12, 12);
  graphics.print(halvorsen_generator.cv_freq2.value());
  graphics.print(" ");
  value = SCALE8_16(halvorsen_generator.get_freq2());
  graphics.print(value);
//  graphics.print(" ");
//  graphics.print(halvorsen_generator.cv_shape.value() * 16);
//  value += halvorsen_generator.cv_shape.value() * 16;
//  graphics.setPrintPos(2, 22);
//  graphics.print(value); graphics.print(" ");
//  value = USAT16(value);
//  graphics.print(value);
}


