/*
 * Quad quantizer mode
 */
#include "util_settings.h"
#include "braids_quantizer.h"
#include "braids_quantizer_scales.h"

// TODO Extend calibration to get exact octave spacing for inputs?

enum ECustomScaleIndex {
  CUSTOM_SCALE_0,
  CUSTOM_SCALE_1,
  CUSTOM_SCALE_2,
  CUSTOM_SCALE_3,
  CUSTOM_SCALE_LAST
};

enum EChannelSettings {
  CHANNEL_SETTING_SCALE,
  CHANNEL_SETTING_ROOT,
  CHANNEL_SETTING_MASK,
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

  uint16_t get_mask() const {
    return values_[CHANNEL_SETTING_MASK];
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
    last_mask_ = 0;
    last_output_ = 0;
    quantizer_.Init();
    update_scale();
  }

  bool update_scale() {
    const int scale = get_scale();
    const uint16_t mask = get_mask();
    if ((last_scale_ != scale || last_mask_ != mask) | force_update_) {
      last_scale_ = scale;
      last_mask_ = mask;
      quantizer_.Configure(get_scale_def(scale), mask);
      return true;
    } else {
      return false;
    }
  }

  void force_update() {
    force_update_ = true;
  }

  template <size_t index, DAC_CHANNEL dac_channel>
  inline void update() {

    bool update = get_update_mode() == CHANNEL_UPDATE_CONTINUOUS;
    if (CLK_STATE[index]) {
      CLK_STATE[index] = false;
      update |= true;
    }

    update |= update_scale();

    if (force_update_) {
      force_update_ = false;
      update |= true;
    }

    if (update) {
      int32_t transpose = get_transpose();
      ADC_CHANNEL source = get_source();
      int32_t pitch = OC::ADC::value(source);
      if (index != source) {
        transpose += (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) * 12) >> 12;
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

      int32_t sample = OC::calibration_data.octaves[octave];
      if (fractional)
        sample += (fractional * (OC::calibration_data.octaves[octave + 1] - OC::calibration_data.octaves[octave])) / (12 << 7);

      if (last_output_ != sample) {
        MENU_REDRAW = 1;
        last_output_ = sample;
      }
    }

    DAC::set<dac_channel>(last_output_ + get_fine());
  }

  int max_mask() const {
    int num_notes = braids::scales[get_scale()].num_notes;
    uint16_t mask = ~(0xffffU << num_notes);
    return mask;
  }

  static const size_t kBinarySize =
    sizeof(uint8_t) + // CHANNEL_SETTING_SCALE
    sizeof(uint8_t) + // CHANNEL_SETTING_ROOT
    sizeof(uint16_t) + // CHANNEL_SETTING_MASK
    sizeof(uint8_t) + // CHANNEL_SETTING_UPDATEMODE
    sizeof(int8_t) + // CHANNEL_SETTING_TRANSPOSE
    sizeof(int8_t) + // CHANNEL_SETTING_OCTAVE
    sizeof(uint8_t) + // CHANNEL_SETTING_SOURCE
    sizeof(int16_t); // CHANNEL_SETTING_FINE

  size_t save_settings(char *storage) {
    char *ptr = storage;
    ptr = write_setting<uint8_t>(ptr, CHANNEL_SETTING_SCALE);
    ptr = write_setting<uint8_t>(ptr, CHANNEL_SETTING_ROOT);
    ptr = write_setting<uint16_t>(ptr, CHANNEL_SETTING_MASK);
    ptr = write_setting<uint8_t>(ptr, CHANNEL_SETTING_UPDATEMODE);
    ptr = write_setting<int8_t>(ptr, CHANNEL_SETTING_TRANSPOSE);
    ptr = write_setting<int8_t>(ptr, CHANNEL_SETTING_OCTAVE);
    ptr = write_setting<uint8_t>(ptr, CHANNEL_SETTING_SOURCE);
    ptr = write_setting<int16_t>(ptr, CHANNEL_SETTING_FINE);

    return (ptr - storage);
  }

  size_t restore_settings(const char *storage) {
    const char *ptr = storage;
    ptr = read_setting<uint8_t>(ptr, CHANNEL_SETTING_SCALE);
    ptr = read_setting<uint8_t>(ptr, CHANNEL_SETTING_ROOT);
    ptr = read_setting<uint16_t>(ptr, CHANNEL_SETTING_MASK);
    ptr = read_setting<uint8_t>(ptr, CHANNEL_SETTING_UPDATEMODE);
    ptr = read_setting<int8_t>(ptr, CHANNEL_SETTING_TRANSPOSE);
    ptr = read_setting<int8_t>(ptr, CHANNEL_SETTING_OCTAVE);
    ptr = read_setting<uint8_t>(ptr, CHANNEL_SETTING_SOURCE);
    ptr = read_setting<int16_t>(ptr, CHANNEL_SETTING_FINE);

    return (ptr - storage);
  }


  static braids::Scale custom_scales[CUSTOM_SCALE_LAST];

  static const braids::Scale &get_scale_def(int index) {
    if (index < CUSTOM_SCALE_LAST)
      return custom_scales[index];
    else
      return braids::scales[index - CUSTOM_SCALE_LAST];
  }

private:
  bool force_update_;
  int last_scale_;
  uint16_t last_mask_;
  int32_t last_output_;
  braids::Quantizer quantizer_;
};

/*static*/ braids::Scale QuantizerChannel::custom_scales[CUSTOM_SCALE_LAST];

static const size_t QUANTIZER_NUM_SCALES = CUSTOM_SCALE_LAST + sizeof(braids::scales) / sizeof(braids::scales[0]);
const char* const quantization_values[QUANTIZER_NUM_SCALES] = {
    "USER1",
    "USER2",
    "USER3",
    "USER4",
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
  { CUSTOM_SCALE_LAST + 1, 0, QUANTIZER_NUM_SCALES + CUSTOM_SCALE_LAST - 1, "scale", quantization_values },
  { 0, 0, 11, "root", note_names },
  { 65535, 1, 65535, "active notes", NULL },
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

#include "QQ_scale_edit.h"

struct QuadQuantizerState {
  int selected_channel;
  EMenuMode left_encoder_mode;
  int left_encoder_value;
  int selected_param;

  ScaleEditor scale_editor;
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
  qq_state.scale_editor.Init();

  for (size_t i = 0; i < CUSTOM_SCALE_LAST; ++i)
    memcpy(&QuantizerChannel::custom_scales[i], &braids::scales[1], sizeof(braids::Scale));
}

static const size_t QQ_SETTINGS_SIZE =
  CHANNEL_SETTING_LAST * QuantizerChannel::kBinarySize +
  CUSTOM_SCALE_LAST * sizeof(braids::Scale);

size_t QQ_save(char *storage) {
  size_t used = 0;
  for (size_t i = 0; i < 4; ++i) {
    used += quantizer_channels[i].save_settings(storage + used);
  }

  memcpy(storage + used, QuantizerChannel::custom_scales, CUSTOM_SCALE_LAST * sizeof(braids::Scale));
  used += CUSTOM_SCALE_LAST * sizeof(braids::Scale);
  return used;
}

size_t QQ_restore(const char *storage) {
  size_t used = 0;
  for (size_t i = 0; i < 4; ++i) {
    used += quantizer_channels[i].restore_settings(storage + used);
  }
  memcpy(QuantizerChannel::custom_scales, storage + used, CUSTOM_SCALE_LAST * sizeof(braids::Scale));
  used += CUSTOM_SCALE_LAST * sizeof(braids::Scale);
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
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  buttons(BUTTON_TOP);
  buttons(BUTTON_BOTTOM);
  buttons(BUTTON_LEFT);
  buttons(BUTTON_RIGHT);
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
    if (CHANNEL_SETTING_MASK != current_item) {
      UI_DRAW_SETTING(attr, channel.get_value(current_item), kUiWideMenuCol1X);
    } else {
      graphics.print(attr.name);
      uint16_t mask = channel.get_mask();
      size_t num_notes = QuantizerChannel::get_scale_def(channel.get_scale()).num_notes;
      weegfx::coord_t x = kUiDisplayWidth - num_notes * 3;
      for (size_t i = 0; i < num_notes; ++i, mask >>= 1, x+=3) {
        if (mask & 0x1)
          graphics.drawRect(x, y + 1, 2, 8);
      }
      UI_END_ITEM();
    }
  UI_END_ITEMS_LOOP();

  if (qq_state.scale_editor.active())
    qq_state.scale_editor.Draw();

  GRAPHICS_END_FRAME();
}

bool QQ_encoders() {
  if (qq_state.scale_editor.active())
    return qq_state.scale_editor.handle_encoders();

  bool changed = false;
  int value = encoder[LEFT].pos();

  switch (qq_state.left_encoder_mode) {
    case MODE_EDIT_CHANNEL:
      if (value != qq_state.left_encoder_value) {
        if (value >= (int)QUANTIZER_NUM_SCALES) value = QUANTIZER_NUM_SCALES - 1;
        else if (value < 0) value = 0;
        qq_state.left_encoder_value = value;
        encoder[LEFT].setPos(value);
        changed = true;
      }
      break;
    case MODE_SELECT_CHANNEL:
      if (value != qq_state.selected_channel) {
        if (value > 3) value = 3;
        else if (value < 0) value = 0;
        qq_state.selected_channel = value;
        encoder[LEFT].setPos(value);
        if (CHANNEL_SETTING_MASK != qq_state.selected_param)
          value = quantizer_channels[qq_state.selected_channel].get_value(qq_state.selected_param);
        else
          value = 0;
        encoder[RIGHT].setPos(value);
        changed = true;
      }
      break;
  }

  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  value = encoder[RIGHT].pos();
  if (CHANNEL_SETTING_MASK != qq_state.selected_param) {
    if (value != selected.get_value(qq_state.selected_param)) {
      if (selected.apply_value(qq_state.selected_param, value))
        selected.force_update();
    }
    encoder[RIGHT].setPos(selected.get_value(qq_state.selected_param));
    changed = true;
  } else {
    if (value && CUSTOM_SCALE_LAST != selected.get_scale()) {
      qq_state.scale_editor.EditMask(&selected);
      changed = true;
    }
  }

  return changed;
}

void QQ_topButton() {
  if (qq_state.scale_editor.active()) {
    qq_state.scale_editor.handle_topButton();
    return;
  }

  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  if (selected.change_value(CHANNEL_SETTING_OCTAVE, 1)) {
    if (qq_state.selected_param == CHANNEL_SETTING_OCTAVE)
      encoder[RIGHT].setPos(selected.get_octave());
    selected.force_update();
  }
}

void QQ_lowerButton() {
  if (qq_state.scale_editor.active()) {
    qq_state.scale_editor.handle_bottomButton();
    return;
  }

  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  if (selected.change_value(CHANNEL_SETTING_OCTAVE, -1)) {
    if (qq_state.selected_param == CHANNEL_SETTING_OCTAVE)
      encoder[RIGHT].setPos(selected.get_octave());
    selected.force_update();
  }
}

void QQ_rightButton() {
  if (qq_state.scale_editor.active()) {
    qq_state.scale_editor.handle_rightButton();
    return;
  }

  QuantizerChannel &selected_channel = quantizer_channels[qq_state.selected_channel];
  ++qq_state.selected_param;
  if (qq_state.selected_param >= CHANNEL_SETTING_LAST)
    qq_state.selected_param = CHANNEL_SETTING_ROOT;
  if (CHANNEL_SETTING_MASK != qq_state.selected_param) {
    encoder[RIGHT].setPos(selected_channel.get_value(qq_state.selected_param));
  } else {
    encoder[RIGHT].setPos(0);
  }
}

void QQ_leftButton() {
  if (qq_state.scale_editor.active()) {
    qq_state.scale_editor.handle_leftButton();
    return;
  }

  QuantizerChannel &selected_channel = quantizer_channels[qq_state.selected_channel];
  if (MODE_EDIT_CHANNEL == qq_state.left_encoder_mode) {
    if (selected_channel.apply_value(CHANNEL_SETTING_SCALE, qq_state.left_encoder_value))
      selected_channel.force_update();
    qq_state.left_encoder_mode = MODE_SELECT_CHANNEL;
    encoder[LEFT].setPos(qq_state.selected_channel);
  } else {
    qq_state.left_encoder_mode = MODE_EDIT_CHANNEL;
    qq_state.left_encoder_value = selected_channel.get_scale();
    encoder[LEFT].setPos(qq_state.left_encoder_value);
  }
}

void QQ_leftButtonLong() {
  if (qq_state.scale_editor.active())
    return;

  QuantizerChannel &selected_channel = quantizer_channels[qq_state.selected_channel];

  if (MODE_SELECT_CHANNEL == qq_state.left_encoder_mode) {
    int scale = selected_channel.get_scale();
    int root = selected_channel.get_root();
    for (int i = 0; i < 4; ++i) {
      quantizer_channels[i].apply_value(CHANNEL_SETTING_ROOT, root);
      quantizer_channels[i].apply_value(CHANNEL_SETTING_SCALE, scale);
      quantizer_channels[i].force_update();
    }
  } else {
    int scale = qq_state.left_encoder_value;
    if (scale < CUSTOM_SCALE_LAST) {
      qq_state.scale_editor.EditScale(&QuantizerChannel::custom_scales[scale], quantization_values[scale]);
    }
  }
}
