#include "streams_lorenz_generator.h"
#include "util/util_math.h"
#include "OC_digital_inputs.h"

enum LORENZ_SETTINGS {
  LORENZ_SETTING_FREQ,
  LORENZ_SETTING_SIGMA,
  LORENZ_SETTING_RHO,
  LORENZ_SETTING_BETA,
  LORENZ_SETTING_LAST
};

class LorenzGenerator : public settings::SettingsBase<LorenzGenerator, LORENZ_SETTING_LAST> {
public:

  uint16_t get_freq() const {
    return values_[LORENZ_SETTING_FREQ];
  }

  uint16_t get_sigma() const {
    return values_[LORENZ_SETTING_SIGMA];
  }

  uint16_t get_rho() const {
    return values_[LORENZ_SETTING_RHO];
  }

  uint16_t get_beta() const {
    return values_[LORENZ_SETTING_BETA];
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

  SmoothedValue<int32_t, kSmoothing> cv_freq;
  SmoothedValue<int32_t, kSmoothing> cv_sigma;
  SmoothedValue<int32_t, kSmoothing> cv_rho;
  SmoothedValue<int32_t, kSmoothing> cv_beta;
};

void LorenzGenerator::Init() {
  InitDefaults();
  lorenz.Init();
  frozen_= false;
}

SETTINGS_DECLARE(LorenzGenerator, LORENZ_SETTING_LAST) {
  { 0, 0, 255, "FREQ", NULL, settings::STORAGE_TYPE_U8 },
  { 10, 7, 20, "SIGMA", NULL, settings::STORAGE_TYPE_U8 }, // 10 is sweet spot
  { 28, 24, 35, "RHO", NULL, settings::STORAGE_TYPE_U8 }, // 28 is sweet spot
  { 8, 4, 11, "BETA", NULL, settings::STORAGE_TYPE_U8 }, // 8 (/3) is sweet spot
};

LorenzGenerator lorenz_generator;
struct {
  int selected_param;
} lorenz_generator_state;


#define SCALE8_16(x) ((((x + 1) << 16) >> 8) - 1)

void FASTRUN LORENZ_isr() {

  bool reset_phase = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>();
  bool freeze = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_2>();

  lorenz_generator.cv_freq.push(OC::ADC::value<ADC_CHANNEL_1>());
  lorenz_generator.cv_sigma.push(OC::ADC::value<ADC_CHANNEL_2>());
  lorenz_generator.cv_rho.push(OC::ADC::value<ADC_CHANNEL_3>());
  lorenz_generator.cv_beta.push(OC::ADC::value<ADC_CHANNEL_4>());

  // Range in settings is (0-256] so this gets scaled to (0,65535]
  // CV value is 12 bit so also needs scaling

  int32_t freq = SCALE8_16(lorenz_generator.get_freq()) + (lorenz_generator.cv_freq.value() * 16);
  freq = USAT16(freq);

  // int32_t sigma = lorenz_generator.get_sigma() + (lorenz_generator.cv_sigma.value() << 7);
  int32_t sigma = lorenz_generator.get_sigma() ;
  lorenz_generator.lorenz.set_sigma(USAT16(sigma));

  // int32_t rho = lorenz_generator.get_rho() + (lorenz_generator.cv_rho.value() << 6);
  int32_t rho = lorenz_generator.get_rho() ;
  lorenz_generator.lorenz.set_rho(USAT16(rho));

  // int32_t beta = lorenz_generator.get_beta() + (lorenz_generator.cv_beta.value() << 6);
  int32_t beta = lorenz_generator.get_beta() ;
  lorenz_generator.lorenz.set_beta(USAT16(beta));

  if (!freeze && !lorenz_generator.frozen())
    lorenz_generator.lorenz.Process(freq, reset_phase);

  DAC::set<DAC_CHANNEL_A>(lorenz_generator.lorenz.dac_code(0));
  DAC::set<DAC_CHANNEL_B>(lorenz_generator.lorenz.dac_code(1));
  DAC::set<DAC_CHANNEL_C>(lorenz_generator.lorenz.dac_code(2));
  DAC::set<DAC_CHANNEL_D>(lorenz_generator.lorenz.dac_code(3));
}

void LORENZ_init() {
  lorenz_generator_state.selected_param = LORENZ_SETTING_SIGMA;
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

//static const size_t kSmallPreviewBufferSize = 32;
//uint16_t preview_buffer[kSmallPreviewBufferSize];

void LORENZ_menu() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem

  graphics.setFont(UI_DEFAULT_FONT);

  static const weegfx::coord_t kStartX = 0;
  UI_DRAW_TITLE(kStartX);
  graphics.setPrintPos(2, 2);
  graphics.print("FREQ ");
  graphics.print(lorenz_generator.get_value(LORENZ_SETTING_FREQ));

  int first_visible_param = LORENZ_SETTING_SIGMA; 
//  lorenz_generator_state.selected_param - 1;
//  if (first_visible_param < LORENZ_SETTING_SIGMA)
//    first_visible_param = LORENZ_SETTING_SIGMA;

  UI_START_MENU(kStartX);
  UI_BEGIN_ITEMS_LOOP(kStartX, first_visible_param, LORENZ_SETTING_LAST, lorenz_generator_state.selected_param, 0)
    const settings::value_attr &attr = LorenzGenerator::value_attr(current_item);
    UI_DRAW_SETTING(attr, lorenz_generator.get_value(current_item), kUiWideMenuCol1X);
  UI_END_ITEMS_LOOP();

  GRAPHICS_END_FRAME();
}

// static const size_t kLargePreviewBufferSize = 64;
// uint16_t large_preview_buffer[kLargePreviewBufferSize];

// weegfx::coord_t scanner = 0;
// unsigned scanner_millis = 0;
// static const unsigned SCANNER_RATE = 200;

void LORENZ_screensaver() {
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

// lorenz_scope_render();
scope_render();

  GRAPHICS_END_FRAME();
}

void LORENZ_handleEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      encoder[LEFT].setPos(lorenz_generator.get_value(LORENZ_SETTING_FREQ));
      encoder[RIGHT].setPos(lorenz_generator.get_value(lorenz_generator_state.selected_param));
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER:
      break;
  }
}

void LORENZ_topButton() {
//  lorenz_generator.change_value(LORENZ_SETTING_FREQ, 32);
//  encoder[LEFT].setPos(lorenz_generator.get_value(LORENZ_SETTING_FREQ));
  lorenz_generator.Init();
}

void LORENZ_lowerButton() {
//  lorenz_generator.change_value(LORENZ_SETTING_FREQ, -32);
//  encoder[LEFT].setPos(lorenz_generator.get_value(LORENZ_SETTING_FREQ));
}

void LORENZ_rightButton() {
  ++lorenz_generator_state.selected_param;
  if (lorenz_generator_state.selected_param >= LORENZ_SETTING_LAST)
    lorenz_generator_state.selected_param = LORENZ_SETTING_SIGMA;
  encoder[RIGHT].setPos(lorenz_generator.get_value(lorenz_generator_state.selected_param));
}

void LORENZ_leftButton() {
}

void LORENZ_leftButtonLong() {
}

bool LORENZ_encoders() {
  bool changed = false;
  int value = encoder[LEFT].pos();
  if (value != lorenz_generator.get_value(LORENZ_SETTING_FREQ)) {
    lorenz_generator.apply_value(LORENZ_SETTING_FREQ, value);
    encoder[LEFT].setPos(lorenz_generator.get_value(LORENZ_SETTING_FREQ));
    changed = true;
  }

  value = encoder[RIGHT].pos();
  if (value != lorenz_generator.get_value(lorenz_generator_state.selected_param)) {
    lorenz_generator.apply_value(lorenz_generator_state.selected_param, value);
    encoder[RIGHT].setPos(lorenz_generator.get_value(lorenz_generator_state.selected_param));
    changed = true;
  }

  return changed;
}

void LORENZ_debug() {
  graphics.setPrintPos(2, 12);
  graphics.print(lorenz_generator.cv_freq.value());
  graphics.print(" ");
  int32_t value = SCALE8_16(lorenz_generator.get_freq());
  graphics.print(value);
//  graphics.print(" ");
//  graphics.print(lorenz_generator.cv_shape.value() * 16);
//  value += lorenz_generator.cv_shape.value() * 16;
//  graphics.setPrintPos(2, 22);
//  graphics.print(value); graphics.print(" ");
//  value = USAT16(value);
//  graphics.print(value);
}


