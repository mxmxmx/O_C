/*
 * Quad quantizer mode
 */
#include "util_settings.h"

extern uint16_t _ADC_OFFSET_0;
extern uint16_t _ADC_OFFSET_1;
extern uint16_t _ADC_OFFSET_2;
extern uint16_t _ADC_OFFSET_3;

extern const char *abc[];
extern const char *map_param5[];
extern const int8_t MAXSCALES;

enum EChannelSettings {
  CHANNEL_SETTING_SCALE,
  CHANNEL_SETTING_OCTAVE,
  CHANNEL_SETTING_TRANSPOSE,
  CHANNEL_SETTING_ATTENUATION,
  CHANNEL_SETTING_MAXNOTES,
  CHANNEL_SETTING_FINE,
  CHANNEL_SETTING_LAST
};

struct quantizer_channel : public settings::SettingsBase<quantizer_channel, CHANNEL_SETTING_LAST> {

  int get_scale() const {
    return values_[CHANNEL_SETTING_SCALE];
  }

  int get_octave() const {
    return values_[CHANNEL_SETTING_OCTAVE];
  }

  int get_offset() const {
    return values_[CHANNEL_SETTING_TRANSPOSE];
  }

  int get_fine() const {
    return values_[CHANNEL_SETTING_FINE];
  }

  int get_attenuation() const {
    return values_[CHANNEL_SETTING_ATTENUATION] + 7; // see ASR
  }

  int get_maxnotes() const {
    return values_[CHANNEL_SETTING_MAXNOTES];
  }
};

/*static*/ template <>
const settings::value_attr settings::SettingsBase<quantizer_channel, CHANNEL_SETTING_LAST>::value_attr_[] = {
  { 0, 0, MAXSCALES, "scale", abc },
  { 0, -4, 4, "octave", NULL },
  { 0, -11, 11, "transp", NULL },
  { 9, 0, 19, "att/mult", map_param5 }, // ASR: 7-26
  { 0, 2, MAXNOTES, "#/scale", NULL },
  { 0, -999, 999, "fine", NULL },
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
quantizer_channel quantizer_channels[4];

#define CLOCK_CHANNEL(i, sample, dac_set) \
do { \
  if (CLK_STATE[i]) { \
    CLK_STATE[i] = false; \
    const quantizer_channel &channel = quantizer_channels[i]; \
    int32_t s = (sample) + channel.get_fine(); \
    s *= channel.get_attenuation(); \
    s = s >> 4; \
    s += channel.get_offset(); \
    s = quant_sc(s, channel.get_scale(), channel.get_octave(), channel.get_maxnotes()); \
    asr_outputs[i] = s; \
    dac_set(s); \
    MENU_REDRAW = 1; \
  } \
} while (0)

void QQ_init() {
  for (size_t i = 0; i < 4; ++i)
    quantizer_channels[i].init_defaults();

  qq_state.selected_channel = 0;
  qq_state.left_encoder_mode = MODE_SELECT_CHANNEL;
  qq_state.left_encoder_value = 0;
  qq_state.selected_param = CHANNEL_SETTING_TRANSPOSE;
}

void QQ_resume() {
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

  u8g.setFont(u8g_font_6x12);
  u8g.setColorIndex(1);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();

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
  static const int kVisibleParams = 3;
  int first_visible_param = qq_state.selected_param - 2;
  if (first_visible_param < CHANNEL_SETTING_TRANSPOSE) {
    first_visible_param = CHANNEL_SETTING_TRANSPOSE;
  } else if (first_visible_param > CHANNEL_SETTING_TRANSPOSE) {
    u8g.setPrintPos(0, y);
    u8g.print('\x5e');
  }

  for (int i = first_visible_param; i < first_visible_param + kVisibleParams; ++i, y+=h) {
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
        value = quantizer_channels[qq_state.selected_channel].get_value(qq_state.selected_param);
        encoder[RIGHT].setPos(value);
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
  quantizer_channels[qq_state.selected_channel].change_value(CHANNEL_SETTING_OCTAVE, 1);
}

void QQ_lowerButton() {
  quantizer_channels[qq_state.selected_channel].change_value(CHANNEL_SETTING_OCTAVE, -1);
}

void QQ_rightButton() {
  ++qq_state.selected_param;
  if (qq_state.selected_param >= CHANNEL_SETTING_LAST)
    qq_state.selected_param = CHANNEL_SETTING_TRANSPOSE;
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
    for (int i = 0; i < 4; ++i)
      quantizer_channels[i].apply_value(CHANNEL_SETTING_SCALE, scale);

    qq_state.left_encoder_mode = MODE_SELECT_CHANNEL;
    encoder[LEFT].setPos(qq_state.selected_channel);
  }
}
