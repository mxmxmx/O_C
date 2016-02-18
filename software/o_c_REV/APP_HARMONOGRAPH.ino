/*
#include "OC_apps.h"
#include "OC_digital_inputs.h"

#include "util/util_math.h"
#include "util/util_settings.h"
#include "util_ui.h"
// #include "frames_harmono_graph.h"

enum HARMONOGRAPH_SETTINGS {
  HARMONOGRAPH_SETTING_COARSE,
  HARMONOGRAPH_SETTING_FINE,
  HARMONOGRAPH_SETTING_SHAPE,
  HARMONOGRAPH_SETTING_SHAPE_SPREAD,
  HARMONOGRAPH_SETTING_SPREAD,
  HARMONOGRAPH_SETTING_COUPLING,
  HARMONOGRAPH_SETTING_LAST
};

class HarmonoGraph : public settings::SettingsBase<HarmonoGraph, HARMONOGRAPH_SETTING_LAST> {
public:

  uint16_t get_coarse() const {
    return values_[HARMONOGRAPH_SETTING_COARSE];
  }

  int16_t get_fine() const {
    return values_[HARMONOGRAPH_SETTING_FINE];
  }

  uint16_t get_shape() const {
    return values_[HARMONOGRAPH_SETTING_SHAPE];
  }

  uint16_t get_shape_spread() const {
    return values_[HARMONOGRAPH_SETTING_SHAPE_SPREAD];
  }

  uint16_t get_spread() const {
    return values_[HARMONOGRAPH_SETTING_SPREAD];
  }

  uint16_t get_coupling() const {
    return values_[HARMONOGRAPH_SETTING_COUPLING];
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

  frames::HarmonoGraph harmongraph;
  bool frozen_;

  // ISR update is at 16.666kHz, we don't need it that fast so smooth the values to ~1Khz
  static constexpr int32_t kSmoothing = 16;

  SmoothedValue<int32_t, kSmoothing> cv_freq;
  SmoothedValue<int32_t, kSmoothing> cv_shape;
  SmoothedValue<int32_t, kSmoothing> cv_spread;
  SmoothedValue<int32_t, kSmoothing> cv_coupling;
};

void HarmonoGraph::Init() {
  InitDefaults();
  harmonograph.Init();
  frozen_= false;
}

SETTINGS_DECLARE(HarmonoGraph, HARMONOGRAPH_SETTING_LAST) {
  { 64, 0, 255, "COARSE", NULL, settings::STORAGE_TYPE_U8 },
  { 0, -128, 127, "FINE", NULL, settings::STORAGE_TYPE_I16 },
  { 0, 0, 255, "SHAPE", NULL, settings::STORAGE_TYPE_U8 },
  { 0, -128, 127, "SHAPE SPREAD", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -128, 127, "SPREAD", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -128, 127, "COUPLING", NULL, settings::STORAGE_TYPE_I8 },
};

HarmonoGraph harmono_graph;
struct {
  int selected_param;
  HARMONOGRAPH_SETTINGS left_edit_mode;
} harmono_graph_state;


#define SCALE8_16(x) ((((x + 1) << 16) >> 8) - 1)

void FASTRUN HARMONOGRAPH_isr() {

  bool reset_phase = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>();
  bool freeze = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_2>();

  harmono_graph.cv_freq.push(OC::ADC::value<ADC_CHANNEL_1>());
  harmono_graph.cv_shape.push(OC::ADC::value<ADC_CHANNEL_2>());
  harmono_graph.cv_spread.push(OC::ADC::value<ADC_CHANNEL_3>());
  harmono_graph.cv_coupling.push(OC::ADC::value<ADC_CHANNEL_4>());

  // Range in settings is (0-256] so this gets scaled to (0,65535]
  // CV value is 12 bit so also needs scaling

  int32_t freq = SCALE8_16(harmono_graph.get_coarse()) + (harmono_graph.cv_freq.value() * 16) + harmono_graph.get_fine() * 2;
  freq = USAT16(freq);

  int32_t shape = SCALE8_16(harmono_graph.get_shape()) + (harmono_graph.cv_shape.value() * 16);
  harmono_graph.harmonograph.set_shape(USAT16(shape));

  int32_t spread = SCALE8_16(harmono_graph.get_spread() + 128) + (harmono_graph.cv_spread.value() * 16);
  harmono_graph.harmonograph.set_spread(USAT16(spread));

  int32_t coupling = SCALE8_16(harmono_graph.get_coupling() + 128) + (harmono_graph.cv_coupling.value() * 16);
  harmono_graph.harmonograph.set_coupling(USAT16(coupling));

  harmono_graph.harmonograph.set_shape_spread(SCALE8_16(harmono_graph.get_shape_spread() + 128));

  if (!freeze && !harmono_graph.frozen())
    harmono_graph.harmonograph.Render(freq, reset_phase);

  DAC::set<DAC_CHANNEL_A>(harmono_graph.harmonograph.dac_code(0));
  DAC::set<DAC_CHANNEL_B>(harmono_graph.harmonograph.dac_code(1));
  DAC::set<DAC_CHANNEL_C>(harmono_graph.harmonograph.dac_code(2));
  DAC::set<DAC_CHANNEL_D>(harmono_graph.harmonograph.dac_code(3));
}

void HARMONOGRAPH_init() {
  harmono_graph_state.selected_param = HARMONOGRAPH_SETTING_SHAPE;
  harmono_graph_state.left_edit_mode = HARMONOGRAPH_SETTING_COARSE;
  harmono_graph.Init();
}

size_t HARMONOGRAPH_storageSize() {
  return HarmonoGraph::storageSize();
}

size_t HARMONOGRAPH_save(void *storage) {
  return harmono_graph.Save(storage);
}

size_t HARMONOGRAPH_restore(const void *storage) {
  return harmono_graph.Restore(storage);
}

void HARMONOGRAPH_loop() {
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  buttons(BUTTON_TOP);
  buttons(BUTTON_BOTTOM);
  buttons(BUTTON_LEFT);
  buttons(BUTTON_RIGHT);
}

static const size_t kSmallPreviewBufferSize = 32;
uint16_t preview_buffer[kSmallPreviewBufferSize];

void HARMONOGRAPH_menu() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem

  graphics.setFont(UI_DEFAULT_FONT);

  static const weegfx::coord_t kStartX = 0;
  UI_DRAW_TITLE(kStartX);
  graphics.setPrintPos(2, 2);
  graphics.print(HarmonoGraph::value_attr(poly_lfo_state.left_edit_mode).name);
  graphics.print(harmono_graph.get_value(poly_lfo_state.left_edit_mode), 5);

  int first_visible_param = HARMONOGRAPH_SETTING_SHAPE; /*poly_lfo_state.selected_param - 1;
  if (first_visible_param < HARMONOGRAPH_SETTING_SHAPE)
    first_visible_param = HARMONOGRAPH_SETTING_SHAPE;
*/

/*
  UI_START_MENU(kStartX);
  UI_BEGIN_ITEMS_LOOP(kStartX, first_visible_param, HARMONOGRAPH_SETTING_LAST, poly_lfo_state.selected_param, 0)
    const settings::value_attr &attr = HarmonoGraph::value_attr(current_item);
    if (current_item != HARMONOGRAPH_SETTING_SHAPE) {
      UI_DRAW_SETTING(attr, harmono_graph.get_value(current_item), kUiWideMenuCol1X);
    } else {
      uint16_t shape = harmono_graph.get_value(current_item);
      harmono_graph.harmonograph.RenderPreview(shape << 8, preview_buffer, kSmallPreviewBufferSize);
      for (weegfx::coord_t x = 0; x < (weegfx::coord_t)kSmallPreviewBufferSize; ++x)
        graphics.setPixel(96 + x, y + 8 - (preview_buffer[x] >> 13));

      UI_DRAW_SETTING(attr, shape,  96 - 32);
    }
  UI_END_ITEMS_LOOP();

  GRAPHICS_END_FRAME();
}

static const size_t kLargePreviewBufferSize = 64;
uint16_t large_preview_buffer[kLargePreviewBufferSize];

weegfx::coord_t scanner = 0;
unsigned scanner_millis = 0;
static const unsigned SCANNER_RATE = 200;

void HARMONOGRAPH_screensaver() {
  GRAPHICS_BEGIN_FRAME(false);
*/ 

/*
//  uint16_t shape = harmono_graph.get_value(HARMONOGRAPH_SETTING_SHAPE) << 8;
//  harmono_graph.harmonograph.RenderPreview(shape, large_preview_buffer, kLargePreviewBufferSize);
//  for (weegfx::coord_t x = 0; x < 128; ++x) {
//    graphics.setPixel(x, 32 - (large_preview_buffer[(x + scanner) & 63] >> 11));
//  }
//  if (millis() - scanner_millis > SCANNER_RATE) {
//    ++scanner;
//    scanner_millis = millis();
//  }

  scope_render();

  GRAPHICS_END_FRAME();
}

void HARMONOGRAPH_handleEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      encoder[LEFT].setPos(harmono_graph.get_value(poly_lfo_state.left_edit_mode));
      encoder[RIGHT].setPos(harmono_graph.get_value(poly_lfo_state.selected_param));
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER:
      break;
  }
}

void HARMONOGRAPH_topButton() {
  harmono_graph.change_value(HARMONOGRAPH_SETTING_COARSE, 32);
  encoder[LEFT].setPos(harmono_graph.get_value(HARMONOGRAPH_SETTING_COARSE));
}

void HARMONOGRAPH_lowerButton() {
  harmono_graph.change_value(HARMONOGRAPH_SETTING_COARSE, -32);
  encoder[LEFT].setPos(harmono_graph.get_value(HARMONOGRAPH_SETTING_COARSE));
}

void HARMONOGRAPH_rightButton() {
  ++poly_lfo_state.selected_param;
  if (poly_lfo_state.selected_param >= HARMONOGRAPH_SETTING_LAST)
    poly_lfo_state.selected_param = HARMONOGRAPH_SETTING_SHAPE;
  encoder[RIGHT].setPos(harmono_graph.get_value(poly_lfo_state.selected_param));
}

void HARMONOGRAPH_leftButton() {
  if (HARMONOGRAPH_SETTING_COARSE == poly_lfo_state.left_edit_mode) {
    poly_lfo_state.left_edit_mode = HARMONOGRAPH_SETTING_FINE;
  } else {
    poly_lfo_state.left_edit_mode = HARMONOGRAPH_SETTING_COARSE;
  }
  encoder[LEFT].setPos(harmono_graph.get_value(poly_lfo_state.left_edit_mode));
}

void HARMONOGRAPH_leftButtonLong() {
}

bool HARMONOGRAPH_encoders() {
  bool changed = false;
  int value = encoder[LEFT].pos();
  if (value != harmono_graph.get_value(poly_lfo_state.left_edit_mode)) {
    harmono_graph.apply_value(poly_lfo_state.left_edit_mode, value);
    encoder[LEFT].setPos(harmono_graph.get_value(poly_lfo_state.left_edit_mode));
    changed = true;
  }

  value = encoder[RIGHT].pos();
  if (value != harmono_graph.get_value(poly_lfo_state.selected_param)) {
    harmono_graph.apply_value(poly_lfo_state.selected_param, value);
    encoder[RIGHT].setPos(harmono_graph.get_value(poly_lfo_state.selected_param));
    changed = true;
  }

  return changed;
}

void HARMONOGRAPH_debug() {
  graphics.setPrintPos(2, 12);
  graphics.print(harmono_graph.cv_shape.value());
  graphics.print(" ");
  int32_t value = SCALE8_16(harmono_graph.get_shape());
  graphics.print(value);
  graphics.print(" ");
  graphics.print(harmono_graph.cv_shape.value() * 16);
  value += harmono_graph.cv_shape.value() * 16;
  graphics.setPrintPos(2, 22);
  graphics.print(value); graphics.print(" ");
  value = USAT16(value);
  graphics.print(value);
}
*/

