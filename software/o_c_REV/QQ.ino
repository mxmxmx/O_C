/*
 * Quad quantizer mode
 */
#include "util_settings.h"

extern uint16_t _ADC_OFFSET_0;
extern uint16_t _ADC_OFFSET_1;
extern uint16_t _ADC_OFFSET_2;
extern uint16_t _ADC_OFFSET_3;

extern const char *abc[];
extern const int8_t MAXSCALES;

struct quantizer_channel {

  enum ESettings {
    SETTING_SCALE,
    SETTING_OCTAVE,
    SETTING_TRANSPOSE,
    SETTING_ATTENUATION,
    SETTING_MAXNOTES,
    SETTING_LAST
  };

  int get_scale() const {
    return values_[SETTING_SCALE];
  }

  int get_octave() const {
    return values_[SETTING_OCTAVE];
  }

  int get_transpose() const {
    return values_[SETTING_TRANSPOSE];
  }

  int get_attenuation() const {
    return values_[SETTING_ATTENUATION];
  }

  int get_maxnotes() const {
    return values_[SETTING_MAXNOTES];
  }

  int get_value(size_t index) const {
    return values_[index];
  }

  static int clamp_value(size_t index, int value) {
    return value_attr_[index].clamp(value);
  }

  bool apply_value(size_t index, int value) {
    if (index < SETTING_LAST) {
      const int clamped = value_attr_[index].clamp(value);
      if (values_[index] != clamped) {
        values_[index] = clamped;
        return true;
      }
    }
    return false;
  }

  bool change_value(size_t index, int delta) {
    return apply_value(index, values_[index] + delta);
  }

  static const settings::value_attr value_attr(size_t i) {
    return value_attr_[i];
  }

//private:
  int values_[SETTING_LAST];
  static const settings::value_attr value_attr_[SETTING_LAST];
};

/*static*/
const settings::value_attr quantizer_channel::value_attr_[] = {
  { 0, 0, MAXSCALES, "scale", abc },
  { 0, -4, 4, "octave", NULL },
  { 0, -12, 12, "offset", NULL },
  { 0, 2, MAXNOTES, "#/scale", NULL },
  { 16, 0, 16, "att/mult", NULL }
};

quantizer_channel quantizer_channels[4] = {
  { 0, 1, 0, MAXNOTES, 16 },
  { 0, 2, 0, MAXNOTES, 16 },
  { 0, 3, 0, MAXNOTES, 16 },
  { 0, 0, 0, MAXNOTES, 16 }
};

enum EMenuMode {
  MODE_SELECT_CHANNEL,
  MODE_EDIT_CHANNEL
};

struct quad_quantizer_state {
  int selected_channel;
  EMenuMode left_encoder_mode;
  int left_encoder_value;
  int selected_param;
};

quad_quantizer_state qq_state;

#define CLOCK_CHANNEL(i, sample, dac_set) \
do { \
  if (CLK_STATE[i]) { \
    CLK_STATE[i] = false; \
    uint16_t s = quant_sc(sample, quantizer_channels[i].get_scale(), quantizer_channels[i].get_transpose(), quantizer_channels[i].get_maxnotes()); \
    asr_outputs[i] = s; \
    dac_set(s); \
  } \
} while (0)

void QQ_init() {
  qq_state.selected_channel = 0;
  qq_state.left_encoder_mode = MODE_SELECT_CHANNEL;
  qq_state.left_encoder_value = 0;
  qq_state.selected_param = quantizer_channel::SETTING_TRANSPOSE;
}

void QQ_loop() {

  UI();
  CLOCK_CHANNEL(0, _ADC_OFFSET_0 - analogRead(CV1), set8565_CHA);
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  CLOCK_CHANNEL(1, _ADC_OFFSET_1 - analogRead(CV2), set8565_CHB);
   buttons(BUTTON_TOP);
  CLOCK_CHANNEL(2, _ADC_OFFSET_2 - analogRead(CV3), set8565_CHC);
   buttons(BUTTON_BOTTOM);
  CLOCK_CHANNEL(3, _ADC_OFFSET_3 - analogRead(CV4), set8565_CHD);
   buttons(BUTTON_LEFT);
  CLOCK_CHANNEL(0, _ADC_OFFSET_0 - analogRead(CV1), set8565_CHA);
   buttons(BUTTON_RIGHT);
  CLOCK_CHANNEL(1, _ADC_OFFSET_1 - analogRead(CV2), set8565_CHB);
  CLOCK_CHANNEL(2, _ADC_OFFSET_2 - analogRead(CV3), set8565_CHC);
  CLOCK_CHANNEL(3, _ADC_OFFSET_3 - analogRead(CV4), set8565_CHD);
}

void QQ_menu() {

  uint8_t col_x = 96;
  uint8_t y = 2;
  uint8_t h = 11;

  uint8_t x = 0;
  u8g.setDefaultForegroundColor();
  for (int i = 0; i < 4; ++i, x += 31) {
    if (i == qq_state.selected_channel) {
      u8g.drawBox(x, 0, 31, 11);
      u8g.setDefaultBackgroundColor();  
    } else {
      u8g.setDefaultForegroundColor();  
    }
    u8g.setPrintPos(x + 4, y);
    u8g.print((char)('A' + i));
    u8g.setPrintPos(x + 14, y);
    int octave = quantizer_channels[i].get_octave();
    if (octave)
      print_int(octave);
  }

  u8g.setDefaultForegroundColor();  
  u8g.drawLine(0, 13, 128, 13);

  y = 2 * h - 4;
  const quantizer_channel &channel = quantizer_channels[qq_state.selected_channel];
  int scale;
  if (MODE_EDIT_CHANNEL == qq_state.left_encoder_mode) {
    scale = qq_state.left_encoder_value;
    u8g.setPrintPos(0x0, y);
    if (channel.get_scale() == scale)
      u8g.print(">");
    else
      u8g.print('\xb7');
  } else {
    scale = channel.get_scale();
  }
  u8g.setPrintPos(10, y);
  u8g.print(abc[scale]);

  y += h;
  for (int i = quantizer_channel::SETTING_TRANSPOSE; i < quantizer_channel::SETTING_LAST; ++i, y+=h) {
    u8g.setPrintPos(10, y);
    if (i == qq_state.selected_param) {
      u8g.setDefaultForegroundColor();
      u8g.drawBox(0, y, 128, h);
      u8g.setDefaultBackgroundColor();
    } else {
      u8g.setDefaultForegroundColor();
    }
    const settings::value_attr &attr = quantizer_channel::value_attr(i);
    u8g.print(attr.name);
    u8g.setPrintPos(col_x, y);
    if (attr.value_names)
      u8g.print(attr.value_names[channel.get_value(i)]);
    else
      print_int(channel.get_value(i));
  }
}

bool QQ_encoders() {
  bool changed = false;
  int value = encoder[LEFT].pos();
  switch (qq_state.left_encoder_mode) {
    case MODE_EDIT_CHANNEL:
      if (value != qq_state.left_encoder_value) {
        if (value >= MAXSCALES) value = MAXSCALES - 1;
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
        changed = true;
      }
      break;
  }

  quantizer_channel &selected = quantizer_channels[qq_state.selected_channel];
  value = encoder[RIGHT].pos();
  if (value != selected.get_value(qq_state.selected_param)) {
    selected.apply_value(qq_state.selected_param, value);
    encoder[RIGHT].setPos(selected.get_value(qq_state.selected_param));
    changed = true;
  }

  return changed;
}

void QQ_topButton() {
  quantizer_channels[qq_state.selected_channel].change_value(quantizer_channel::SETTING_OCTAVE, 1);
}

void QQ_lowerButton() {
  quantizer_channels[qq_state.selected_channel].change_value(quantizer_channel::SETTING_OCTAVE, -1);
}

void QQ_rightButton() {
  ++qq_state.selected_param;
  if (qq_state.selected_param >= quantizer_channel::SETTING_LAST)
    qq_state.selected_param = quantizer_channel::SETTING_TRANSPOSE;
  encoder[RIGHT].setPos(quantizer_channels[qq_state.selected_channel].get_value(qq_state.selected_param));
}

void QQ_leftButton() {
  if (MODE_EDIT_CHANNEL == qq_state.left_encoder_mode) {
    quantizer_channels[qq_state.selected_channel].apply_value(quantizer_channel::SETTING_SCALE,qq_state.left_encoder_value);
    qq_state.left_encoder_mode = MODE_SELECT_CHANNEL;
    encoder[LEFT].setPos(qq_state.selected_channel);
  } else {
    qq_state.left_encoder_mode = MODE_EDIT_CHANNEL;
    qq_state.left_encoder_value = quantizer_channels[qq_state.selected_channel].get_scale();
    encoder[LEFT].setPos(qq_state.left_encoder_value);
  }
}
