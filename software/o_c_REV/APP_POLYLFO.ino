#include "OC_apps.h"
#include "OC_digital_inputs.h"
#include "OC_menus.h"

#include "util/util_math.h"
#include "util/util_settings.h"
#include "frames_poly_lfo.h"

enum POLYLFO_SETTINGS {
  POLYLFO_SETTING_COARSE,
  POLYLFO_SETTING_FINE,
  POLYLFO_SETTING_SHAPE,
  POLYLFO_SETTING_SHAPE_SPREAD,
  POLYLFO_SETTING_SPREAD,
  POLYLFO_SETTING_COUPLING,
  POLYLFO_SETTING_LAST
};

class PolyLfo : public settings::SettingsBase<PolyLfo, POLYLFO_SETTING_LAST> {
public:

  uint16_t get_coarse() const {
    return values_[POLYLFO_SETTING_COARSE];
  }

  int16_t get_fine() const {
    return values_[POLYLFO_SETTING_FINE];
  }

  uint16_t get_shape() const {
    return values_[POLYLFO_SETTING_SHAPE];
  }

  uint16_t get_shape_spread() const {
    return values_[POLYLFO_SETTING_SHAPE_SPREAD];
  }

  uint16_t get_spread() const {
    return values_[POLYLFO_SETTING_SPREAD];
  }

  uint16_t get_coupling() const {
    return values_[POLYLFO_SETTING_COUPLING];
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

  frames::PolyLfo lfo;
  bool frozen_;

  // ISR update is at 16.666kHz, we don't need it that fast so smooth the values to ~1Khz
  static constexpr int32_t kSmoothing = 16;

  SmoothedValue<int32_t, kSmoothing> cv_freq;
  SmoothedValue<int32_t, kSmoothing> cv_shape;
  SmoothedValue<int32_t, kSmoothing> cv_spread;
  SmoothedValue<int32_t, kSmoothing> cv_coupling;
};

void PolyLfo::Init() {
  InitDefaults();
  lfo.Init();
  frozen_= false;
}

SETTINGS_DECLARE(PolyLfo, POLYLFO_SETTING_LAST) {
  { 64, 0, 255, "COARSE", NULL, settings::STORAGE_TYPE_U8 },
  { 0, -128, 127, "FINE", NULL, settings::STORAGE_TYPE_I16 },
  { 0, 0, 255, "SHAPE", NULL, settings::STORAGE_TYPE_U8 },
  { 0, -128, 127, "SHAPE SPREAD", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -128, 127, "SPREAD", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -128, 127, "COUPLING", NULL, settings::STORAGE_TYPE_I8 },
};

PolyLfo poly_lfo;
struct {

  POLYLFO_SETTINGS left_edit_mode;
  menu::ScreenCursor<menu::kScreenLines> cursor;

} poly_lfo_state;

void FASTRUN POLYLFO_isr() {

  bool reset_phase = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>();
  bool freeze = OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_2>();

  poly_lfo.cv_freq.push(OC::ADC::value<ADC_CHANNEL_1>());
  poly_lfo.cv_shape.push(OC::ADC::value<ADC_CHANNEL_2>());
  poly_lfo.cv_spread.push(OC::ADC::value<ADC_CHANNEL_3>());
  poly_lfo.cv_coupling.push(OC::ADC::value<ADC_CHANNEL_4>());

  // Range in settings is (0-256] so this gets scaled to (0,65535]
  // CV value is 12 bit so also needs scaling

  int32_t freq = SCALE8_16(poly_lfo.get_coarse()) + (poly_lfo.cv_freq.value() * 16) + poly_lfo.get_fine() * 2;
  freq = USAT16(freq);

  int32_t shape = SCALE8_16(poly_lfo.get_shape()) + (poly_lfo.cv_shape.value() * 16);
  poly_lfo.lfo.set_shape(USAT16(shape));

  int32_t spread = SCALE8_16(poly_lfo.get_spread() + 128) + (poly_lfo.cv_spread.value() * 16);
  poly_lfo.lfo.set_spread(USAT16(spread));

  int32_t coupling = SCALE8_16(poly_lfo.get_coupling() + 128) + (poly_lfo.cv_coupling.value() * 16);
  poly_lfo.lfo.set_coupling(USAT16(coupling));

  poly_lfo.lfo.set_shape_spread(SCALE8_16(poly_lfo.get_shape_spread() + 128));

  if (!freeze && !poly_lfo.frozen())
    poly_lfo.lfo.Render(freq, reset_phase);

  DAC::set<DAC_CHANNEL_A>(poly_lfo.lfo.dac_code(0));
  DAC::set<DAC_CHANNEL_B>(poly_lfo.lfo.dac_code(1));
  DAC::set<DAC_CHANNEL_C>(poly_lfo.lfo.dac_code(2));
  DAC::set<DAC_CHANNEL_D>(poly_lfo.lfo.dac_code(3));
}

void POLYLFO_init() {

  poly_lfo_state.left_edit_mode = POLYLFO_SETTING_COARSE;
  poly_lfo_state.cursor.Init(POLYLFO_SETTING_SHAPE, POLYLFO_SETTING_LAST - 1);
  poly_lfo.Init();
}

size_t POLYLFO_storageSize() {
  return PolyLfo::storageSize();
}

size_t POLYLFO_save(void *storage) {
  return poly_lfo.Save(storage);
}

size_t POLYLFO_restore(const void *storage) {
  return poly_lfo.Restore(storage);
}

void POLYLFO_loop() {
}

static const size_t kSmallPreviewBufferSize = 32;
uint16_t preview_buffer[kSmallPreviewBufferSize];

void POLYLFO_menu() {

  menu::DefaultTitleBar::Draw();
  graphics.print(PolyLfo::value_attr(poly_lfo_state.left_edit_mode).name);
  graphics.print(poly_lfo.get_value(poly_lfo_state.left_edit_mode), 5);


  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX - 1> settings_list(poly_lfo_state.cursor);
  menu::SettingsListItem list_item;
  while (settings_list.available()) {
    const int current = settings_list.Next(list_item);
    const int value = poly_lfo.get_value(current);
    if (POLYLFO_SETTING_SHAPE != current) {
      list_item.DrawDefault(value, PolyLfo::value_attr(current));
    } else {

      poly_lfo.lfo.RenderPreview(value << 8, preview_buffer, kSmallPreviewBufferSize);
      const uint16_t *preview = preview_buffer;
      uint16_t count = kSmallPreviewBufferSize;
      weegfx::coord_t x = list_item.valuex;
      while (count--)
        graphics.setPixel(x++, list_item.y + 8 - (*preview++ >> 13));

      list_item.endx = menu::kDefaultMenuEndX - 39;
      list_item.DrawDefault(value, PolyLfo::value_attr(current));
    }
  }
}

void POLYLFO_screensaver() {
  OC::scope_render();
}

void POLYLFO_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      poly_lfo_state.cursor.set_editing(false);
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
      break;
  }
}

void POLYLFO_handleButtonEvent(const UI::Event &event) {
  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        poly_lfo.change_value(POLYLFO_SETTING_COARSE, 32);
        break;
      case OC::CONTROL_BUTTON_DOWN:
        poly_lfo.change_value(POLYLFO_SETTING_COARSE, -32);
        break;
      case OC::CONTROL_BUTTON_L:
      if (POLYLFO_SETTING_COARSE == poly_lfo_state.left_edit_mode)
        poly_lfo_state.left_edit_mode = POLYLFO_SETTING_FINE;
      else
        poly_lfo_state.left_edit_mode = POLYLFO_SETTING_COARSE;
      break;
      case OC::CONTROL_BUTTON_R:
        poly_lfo_state.cursor.toggle_editing();
        break;
    }
  }
}

void POLYLFO_handleEncoderEvent(const UI::Event &event) {
  if (OC::CONTROL_ENCODER_L == event.control) {
    poly_lfo.change_value(poly_lfo_state.left_edit_mode, event.value);
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    if (poly_lfo_state.cursor.editing()) {
      poly_lfo.change_value(poly_lfo_state.cursor.cursor_pos(), event.value);
    } else {
      poly_lfo_state.cursor.Scroll(event.value);
    }
  }
}

void POLYLFO_debug() {
  graphics.setPrintPos(2, 12);
  graphics.print(poly_lfo.cv_shape.value());
  graphics.print(" ");
  int32_t value = SCALE8_16(poly_lfo.get_shape());
  graphics.print(value);
  graphics.print(" ");
  graphics.print(poly_lfo.cv_shape.value() * 16);
  value += poly_lfo.cv_shape.value() * 16;
  graphics.setPrintPos(2, 22);
  graphics.print(value); graphics.print(" ");
  value = USAT16(value);
  graphics.print(value);
}
