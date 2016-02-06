#include "frames_poly_lfo.h"

enum POLYLFO_SETTINGS {
  POLYLFO_SETTING_FREQ,
  POLYLFO_SETTING_SHAPE,
  POLYLFO_SETTING_SHAPE_SPREAD,
  POLYLFO_SETTING_SPREAD,
  POLYLFO_SETTING_COUPLING,
  POLYLFO_SETTING_LAST
};

class PolyLfo : public settings::SettingsBase<PolyLfo, POLYLFO_SETTING_LAST> {
public:

  int get_freq() const {
    return values_[POLYLFO_SETTING_FREQ];
  }

  int get_shape() const {
    return values_[POLYLFO_SETTING_SHAPE];
  }

  int get_shape_spread() const {
    return values_[POLYLFO_SETTING_SHAPE_SPREAD];
  }

  int get_spread() const {
    return values_[POLYLFO_SETTING_SPREAD];
  }

  int get_coupling() const {
    return values_[POLYLFO_SETTING_COUPLING];
  }

  void Init();

  frames::PolyLfo lfo;
};


void PolyLfo::Init() {
  InitDefaults();
  lfo.Init();
}

/*static*/ template <>
const settings::value_attr settings::SettingsBase<PolyLfo, POLYLFO_SETTING_LAST>::value_attr_[] = {
  { 64, 0, 255, "FREQ", NULL },
  { 0, 0, 255, "SHAPE", NULL },
  { 128, 0, 255, "SHAPE SPREAD", NULL },
  { 128, 0, 255, "SPREAD", NULL },
  { 128, 0, 255, "COUPLING", NULL },
};

PolyLfo poly_lfo;
struct {
  int selected_param;
} poly_lfo_state;

void FASTRUN POLYLFO_isr() {

  poly_lfo.lfo.set_shape(poly_lfo.get_shape() << 8);
  poly_lfo.lfo.set_shape_spread(poly_lfo.get_shape_spread() << 8);
  poly_lfo.lfo.set_spread(poly_lfo.get_spread() << 8);
  poly_lfo.lfo.set_coupling(poly_lfo.get_coupling() << 8);

  poly_lfo.lfo.Render(poly_lfo.get_freq() << 8);
  DAC::set<DAC_CHANNEL_A>(poly_lfo.lfo.dac_code(0));
  DAC::set<DAC_CHANNEL_B>(poly_lfo.lfo.dac_code(1));
  DAC::set<DAC_CHANNEL_C>(poly_lfo.lfo.dac_code(2));
  DAC::set<DAC_CHANNEL_D>(poly_lfo.lfo.dac_code(3));
}

void POLYLFO_init() {
  poly_lfo_state.selected_param = POLYLFO_SETTING_SHAPE;
  poly_lfo.Init();
}

static const size_t POLYLFO_SETTINGS_SIZE = sizeof(uint8_t) * POLYLFO_SETTING_LAST;

size_t POLYLFO_save(char *storage) {
  return poly_lfo.save<uint8_t>(storage);
}

size_t POLYLFO_restore(const char *storage) {
  return poly_lfo.restore<uint8_t>(storage);
}

void POLYLFO_loop() {
  UI();
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  buttons(BUTTON_TOP);
  buttons(BUTTON_BOTTOM);
  buttons(BUTTON_LEFT);
  buttons(BUTTON_RIGHT);
}

static const size_t kSmallPreviewBufferSize = 32;
uint16_t preview_buffer[kSmallPreviewBufferSize];

void POLYLFO_menu() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem

  graphics.setFont(UI_DEFAULT_FONT);

  static const weegfx::coord_t kStartX = 0;
  UI_DRAW_TITLE(kStartX);
  graphics.setPrintPos(2, 2);
  graphics.print("FREQ ");
  graphics.print(poly_lfo.get_value(POLYLFO_SETTING_FREQ));

  int first_visible_param = POLYLFO_SETTING_SHAPE; /*poly_lfo_state.selected_param - 1;
  if (first_visible_param < POLYLFO_SETTING_SHAPE)
    first_visible_param = POLYLFO_SETTING_SHAPE;*/

  UI_START_MENU(kStartX);
  UI_BEGIN_ITEMS_LOOP(kStartX, first_visible_param, POLYLFO_SETTING_LAST, poly_lfo_state.selected_param, 0)
    const settings::value_attr &attr = PolyLfo::value_attr(current_item);
    if (current_item != POLYLFO_SETTING_SHAPE) {
      UI_DRAW_SETTING(attr, poly_lfo.get_value(current_item), kUiWideMenuCol1X);
    } else {
      uint16_t shape = poly_lfo.get_value(current_item);
      poly_lfo.lfo.RenderPreview(shape << 8, preview_buffer, kSmallPreviewBufferSize);
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

void POLYLFO_screensaver() {
  GRAPHICS_BEGIN_FRAME(false);

  uint16_t shape = poly_lfo.get_value(POLYLFO_SETTING_SHAPE) << 8;
  poly_lfo.lfo.RenderPreview(shape, large_preview_buffer, kLargePreviewBufferSize);
  for (weegfx::coord_t x = 0; x < 128; ++x) {
    graphics.setPixel(x, 32 - (large_preview_buffer[(x + scanner) & 63] >> 11));
  }
  if (millis() - scanner_millis > SCANNER_RATE) {
    ++scanner;
    scanner_millis = millis();
  }

  scope_render();

  GRAPHICS_END_FRAME();
}

void POLYLFO_resume() {
  encoder[LEFT].setPos(poly_lfo.get_value(POLYLFO_SETTING_FREQ));
  encoder[RIGHT].setPos(poly_lfo.get_value(poly_lfo_state.selected_param));
}

void POLYLFO_topButton() {
  poly_lfo.change_value(POLYLFO_SETTING_FREQ, 32);
  encoder[LEFT].setPos(poly_lfo.get_value(POLYLFO_SETTING_FREQ));
}

void POLYLFO_lowerButton() {
  poly_lfo.change_value(POLYLFO_SETTING_FREQ, -32);
  encoder[LEFT].setPos(poly_lfo.get_value(POLYLFO_SETTING_FREQ));
}

void POLYLFO_rightButton() {
  ++poly_lfo_state.selected_param;
  if (poly_lfo_state.selected_param >= POLYLFO_SETTING_LAST)
    poly_lfo_state.selected_param = POLYLFO_SETTING_SHAPE;
  encoder[RIGHT].setPos(poly_lfo.get_value(poly_lfo_state.selected_param));
}

void POLYLFO_leftButton() {
}

void POLYLFO_leftButtonLong() {
}

bool POLYLFO_encoders() {
  bool changed = false;
  int value = encoder[LEFT].pos();
  if (value != poly_lfo.get_value(POLYLFO_SETTING_FREQ)) {
    poly_lfo.apply_value(POLYLFO_SETTING_FREQ, value);
    encoder[LEFT].setPos(poly_lfo.get_value(POLYLFO_SETTING_FREQ));
    changed = true;
  }

  value = encoder[RIGHT].pos();
  if (value != poly_lfo.get_value(poly_lfo_state.selected_param)) {
    poly_lfo.apply_value(poly_lfo_state.selected_param, value);
    encoder[RIGHT].setPos(poly_lfo.get_value(poly_lfo_state.selected_param));
    changed = true;
  }

  return changed;
}


