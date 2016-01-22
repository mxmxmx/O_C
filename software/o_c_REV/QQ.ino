/*
 * Quad quantizer mode
 */
#include "util_settings.h"
#include "quantizer.h"
#include "quantizer_scales.h"

// TODO Extend calibration to get exact octave spacing for inputs?

enum EChannelSettings {
  CHANNEL_SETTING_SCALE,
  CHANNEL_SETTING_ROOT,
  CHANNEL_SETTING_UPDATEMODE,
  CHANNEL_SETTING_TRANSPOSE,
  CHANNEL_SETTING_OCTAVE,
  CHANNEL_SETTING_SOURCE,
  CHANNEL_SETTING_FINE,
  CHANNEL_SETTING_LAST
};

enum EChannelUpdateMode {
  CHANNEL_UPDATE_TRIGGERED,
  CHANNEL_UPDATE_CONTINUOUS,
  CHANNEL_UPDATE_LAST
};

class QuantizerChannel : public settings::SettingsBase<QuantizerChannel, CHANNEL_SETTING_LAST> {
public:

  int get_scale() const {
    return values_[CHANNEL_SETTING_SCALE];
  }

  int get_root() const {
    return values_[CHANNEL_SETTING_ROOT];
  }

  EChannelUpdateMode get_update_mode() const {
    return static_cast<EChannelUpdateMode>(values_[CHANNEL_SETTING_UPDATEMODE]);
  }

  int get_transpose() const {
    return values_[CHANNEL_SETTING_TRANSPOSE];
  }

  int get_octave() const {
    return values_[CHANNEL_SETTING_OCTAVE];
  }

  ADC_CHANNEL get_source() const {
    return static_cast<ADC_CHANNEL>(values_[CHANNEL_SETTING_SOURCE]);
  }

  int get_fine() const {
    return values_[CHANNEL_SETTING_FINE];
  }

  void init() {
    force_update_ = false;
    last_scale_ = -1;
    last_output_ = 0;
    quantizer_.Init();
    update_scale();
  }

  void update_scale() {
    int scale = get_scale();
    if (last_scale_ != scale) {
      quantizer_.Configure(braids::scales[scale]);
      last_scale_ = scale;
      force_update_ = true;
    }
  }

  void force_update() {
    force_update_ = true;
  }

  template <size_t index, DAC_CHANNEL dac_channel>
  void update() {

    bool update = get_update_mode() == CHANNEL_UPDATE_CONTINUOUS;
    if (CLK_STATE[index]) {
      CLK_STATE[index] = false;
      update |= true;
    }

    update_scale();

    if (force_update_) {
      force_update_ = false;
      update |= true;
    }

    if (update) {
      int32_t transpose = get_transpose();
      ADC_CHANNEL source = get_source();
      int32_t pitch = ADC::value(source);
      if (index != source) {
        transpose += (ADC::value(static_cast<ADC_CHANNEL>(index)) * 12) >> 12;
      }
      if (transpose > 12) transpose = 12;
      else if (transpose < -12) transpose = -12;

      pitch = (pitch * 120 << 7) >> 12; // Convert to range with 128 steps per semitone
      pitch += 3 * 12 << 7; // offset for LUT range

      int32_t quantized = quantizer_.Process(pitch, (get_root() + 60) << 7, transpose);
      quantized += get_octave() * 12 << 7;

      if (quantized > (120 << 7))
        quantized = 120 << 7;
      else if (quantized < 0)
        quantized = 0;

      const int32_t octave = quantized / (12 << 7);
      const int32_t fractional = quantized - octave * (12 << 7);

      int32_t sample = octaves[octave];
      if (fractional)
        sample += (fractional * (octaves[octave + 1] - octaves[octave])) / (12 << 7);

      if (last_output_ != sample) {
        MENU_REDRAW = 1;
        last_output_ = sample;
      }
    }

    DAC::set<dac_channel>(last_output_ + get_fine());
  }

private:
  bool force_update_;
  int last_scale_;
  int32_t last_output_;
  braids::Quantizer quantizer_;
};

static const size_t QUANTIZER_NUM_SCALES = sizeof(braids::scales) / sizeof(braids::scales[0]);
const char* const quantization_values[QUANTIZER_NUM_SCALES] = {
    "OFF ",
    "SEMI",
    "IONI",
    "DORI",
    "PHRY",
    "LYDI",
    "MIXO",
    "AEOL",
    "LOCR",
    "BLU+",
    "BLU-",
    "PEN+",
    "PEN-",
    "FOLK",
    "JAPA",
    "GAME",
    "GYPS",
    "ARAB",
    "FLAM",
    "WHOL",
    "PYTH",
    "EB/4",
    "E /4",
    "EA/4",
    "BHAI",
    "GUNA",
    "MARW",
    "SHRI",
    "PURV",
    "BILA",
    "YAMA",
    "KAFI",
    "BHIM",
    "DARB",
    "RAGE",
    "KHAM",
    "MIMA",
    "PARA",
    "RANG",
    "GANG",
    "KAME",
    "PAKA",
    "NATB",
    "KAUN",
    "BAIR",
    "BTOD",
    "CHAN",
    "KTOD",
    "JOGE" };

const char* const update_modes[CHANNEL_UPDATE_LAST] = {
  "trig",
  "cont"
};

const char* const channel_source[ADC_CHANNEL_LAST] = {
  "CV1", "CV2", "CV3", "CV4"
};

/*static*/ template <>
const settings::value_attr settings::SettingsBase<QuantizerChannel, CHANNEL_SETTING_LAST>::value_attr_[] = {
  { 1, 0, QUANTIZER_NUM_SCALES - 1, "scale", quantization_values },
  { 0, 0, 11, "root", note_names },
  { CHANNEL_UPDATE_CONTINUOUS, 0, CHANNEL_UPDATE_LAST - 1, "update", update_modes },
  { 0, -5, 7, "transpose", NULL },
  { 0, -4, 4, "octave", NULL },
  { ADC_CHANNEL_1, ADC_CHANNEL_1, ADC_CHANNEL_LAST - 1, "source", channel_source},
  { 0, -999, 999, "fine", NULL },
};

enum EMenuMode {
  MODE_SELECT_CHANNEL,
  MODE_EDIT_CHANNEL
};

struct QuadQuantizerState {
  int selected_channel;
  EMenuMode left_encoder_mode;
  int left_encoder_value;
  int selected_param;
  int32_t raw_cvs[4];
};

QuadQuantizerState qq_state;
QuantizerChannel quantizer_channels[4];

void QQ_init() {
  for (size_t i = 0; i < 4; ++i) {
    quantizer_channels[i].init_defaults();
    quantizer_channels[i].init();
    quantizer_channels[i].apply_value(CHANNEL_SETTING_SOURCE, (int)i); // override
  }

  qq_state.selected_channel = 0;
  qq_state.left_encoder_mode = MODE_SELECT_CHANNEL;
  qq_state.left_encoder_value = 0;
  qq_state.selected_param = CHANNEL_SETTING_ROOT;
}

static const size_t QQ_SETTINGS_SIZE = sizeof(int16_t) * CHANNEL_SETTING_LAST * 4;

size_t QQ_save(char *storage) {
  size_t used = 0;
  for (size_t i = 0; i < 4; ++i) {
    used += quantizer_channels[i].save<int16_t>(storage + used);
  }
  return used;
}

size_t QQ_restore(const char *storage) {
  size_t used = 0;
  for (size_t i = 0; i < 4; ++i) {
    used += quantizer_channels[i].restore<int16_t>(storage + used);
  }
  return used;
}

void QQ_resume() {
  switch (qq_state.left_encoder_mode) {
    case MODE_EDIT_CHANNEL:
      encoder[LEFT].setPos(qq_state.left_encoder_value);
      break;
    case MODE_SELECT_CHANNEL:
      encoder[LEFT].setPos(qq_state.selected_channel);
      break;
  }
  encoder[RIGHT].setPos(quantizer_channels[qq_state.selected_channel].get_value(qq_state.selected_param));
}

#define CLOCK_CHANNEL(i, adc_channel, dac_channel) \
do { \
  ADC::ReadImmediate<adc_channel>(); \
  quantizer_channels[i].update<i, dac_channel>(); \
} while (0)

void QQ_isr() {
  quantizer_channels[0].update<0, DAC_CHANNEL_A>();
  quantizer_channels[1].update<1, DAC_CHANNEL_B>();
  quantizer_channels[2].update<2, DAC_CHANNEL_C>();
  quantizer_channels[3].update<3, DAC_CHANNEL_D>();
}

void QQ_loop() {
  UI();
  ADC::ReadImmediate<ADC_CHANNEL_1>();
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  ADC::ReadImmediate<ADC_CHANNEL_2>();
  buttons(BUTTON_TOP);
  buttons(BUTTON_BOTTOM);
  ADC::ReadImmediate<ADC_CHANNEL_3>();
  buttons(BUTTON_LEFT);
  buttons(BUTTON_RIGHT);
  ADC::ReadImmediate<ADC_CHANNEL_4>();
}

void QQ_menu() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem

  graphics.setFont(UI_DEFAULT_FONT);

  static const weegfx::coord_t kStartX = 0;

  UI_DRAW_TITLE(kStartX);

  for (int i = 0, x = 0; i < 4; ++i, x += 32) {
    graphics.setPrintPos(x + 4, 2);
    graphics.print((char)('A' + i));
    graphics.setPrintPos(x + 14, 2);
    int octave = quantizer_channels[i].get_octave();
    if (octave)
      graphics.pretty_print(octave);

    if (i == qq_state.selected_channel) {
      graphics.invertRect(x, 0, 32, 11);
    }
  }

  const QuantizerChannel &channel = quantizer_channels[qq_state.selected_channel];

  UI_START_MENU(kStartX);

  int scale;
  if (MODE_EDIT_CHANNEL == qq_state.left_encoder_mode) {
    scale = qq_state.left_encoder_value;
    if (channel.get_scale() == scale)
      graphics.print(">");
    else
      graphics.print('-');
  } else {
    scale = channel.get_scale();
  }
  graphics.print(quantization_values[scale]);

  int first_visible_param = qq_state.selected_param - 2;
  if (first_visible_param < CHANNEL_SETTING_ROOT)
    first_visible_param = CHANNEL_SETTING_ROOT;

  UI_BEGIN_ITEMS_LOOP(kStartX, first_visible_param, CHANNEL_SETTING_LAST, qq_state.selected_param, 1)
    const settings::value_attr &attr = QuantizerChannel::value_attr(current_item);
    UI_DRAW_SETTING(attr, channel.get_value(current_item), kUiWideMenuCol1X);
  UI_END_ITEMS_LOOP();

  GRAPHICS_END_FRAME();
}

bool QQ_encoders() {
  bool changed = false;
  int value = encoder[LEFT].pos();
  switch (qq_state.left_encoder_mode) {
    case MODE_EDIT_CHANNEL:
      if (value != qq_state.left_encoder_value) {
        if (value >= (int)QUANTIZER_NUM_SCALES) value = QUANTIZER_NUM_SCALES - 1;
        else if (value < 0) value = 0;
        qq_state.left_encoder_value = value;
        encoder[LEFT].setPos(value);
        encoder[RIGHT].setPos(quantizer_channels[qq_state.selected_channel].get_value(qq_state.selected_param));
        changed = true;
      }
      break;
    case MODE_SELECT_CHANNEL:
      if (value != qq_state.selected_channel) {
        if (value > 3) value = 3;
        else if (value < 0) value = 0;
        qq_state.selected_channel = value;
        encoder[LEFT].setPos(value);
        value = quantizer_channels[qq_state.selected_channel].get_value(qq_state.selected_param);
        encoder[RIGHT].setPos(value);
        changed = true;
      }
      break;
  }

  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  value = encoder[RIGHT].pos();
  if (value != selected.get_value(qq_state.selected_param)) {
    if (selected.apply_value(qq_state.selected_param, value))
      selected.force_update();
    encoder[RIGHT].setPos(selected.get_value(qq_state.selected_param));
    changed = true;
  }

  return changed;
}

void QQ_topButton() {
  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  if (selected.change_value(CHANNEL_SETTING_OCTAVE, 1)) {
    if (qq_state.selected_param == CHANNEL_SETTING_OCTAVE)
      encoder[RIGHT].setPos(selected.get_octave());
    selected.force_update();
  }
}

void QQ_lowerButton() {
  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  if (selected.change_value(CHANNEL_SETTING_OCTAVE, -1)) {
    if (qq_state.selected_param == CHANNEL_SETTING_OCTAVE)
      encoder[RIGHT].setPos(selected.get_octave());
    selected.force_update();
  }
}

void QQ_rightButton() {
  ++qq_state.selected_param;
  if (qq_state.selected_param >= CHANNEL_SETTING_LAST)
    qq_state.selected_param = CHANNEL_SETTING_ROOT;
  encoder[RIGHT].setPos(quantizer_channels[qq_state.selected_channel].get_value(qq_state.selected_param));
}

void QQ_leftButton() {
  if (MODE_EDIT_CHANNEL == qq_state.left_encoder_mode) {
    quantizer_channels[qq_state.selected_channel].apply_value(CHANNEL_SETTING_SCALE, qq_state.left_encoder_value);
    qq_state.left_encoder_mode = MODE_SELECT_CHANNEL;
    encoder[LEFT].setPos(qq_state.selected_channel);
  } else {
    qq_state.left_encoder_mode = MODE_EDIT_CHANNEL;
    qq_state.left_encoder_value = quantizer_channels[qq_state.selected_channel].get_scale();
    encoder[LEFT].setPos(qq_state.left_encoder_value);
  }
}

void QQ_leftButtonLong() {
  if (MODE_EDIT_CHANNEL == qq_state.left_encoder_mode) {
    int scale = qq_state.left_encoder_value;
    int root = quantizer_channels[qq_state.selected_channel].get_root();
    for (int i = 0; i < 4; ++i) {
      quantizer_channels[i].apply_value(CHANNEL_SETTING_SCALE, scale);
      quantizer_channels[i].apply_value(CHANNEL_SETTING_ROOT, root);
      quantizer_channels[i].force_update();
    }

    qq_state.left_encoder_mode = MODE_SELECT_CHANNEL;
    encoder[LEFT].setPos(qq_state.selected_channel);
  }
}
