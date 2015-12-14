#ifndef UTIL_UI_H_
#define UTIL_UI_H_

#define UI_DEFAULT_FONT u8g_font_6x12

static const uint8_t kUiDefaultFontH = 12;
static const uint8_t kUiParamStartY = kUiDefaultFontH + 2 + 1;
static const uint8_t kUiParamLineH = 12;

static const uint8_t kUiVisibleParams = 4;
static const uint8_t kDisplayWidth = 128;

#define UI_DRAW_TITLE(xstart) \
  u8g.setDefaultForegroundColor(); \
  u8g.setPrintPos(xstart, 0); \
  u8g.drawHLine(xstart, kUiDefaultFontH, kDisplayWidth - xstart - 2);

#define UI_BEGIN_SETTINGS_LOOP(xstart, first, last, selected) \
  uint8_t y = kUiParamStartY; \
  for (size_t i = 0, setting = first; i < kUiVisibleParams && setting < last; ++i, ++setting, y += kUiParamLineH) { \
    u8g.setDefaultForegroundColor(); \
    if (selected == setting) { \
      u8g.drawBox(xstart, y, kDisplayWidth - xstart - 2, kUiParamLineH - 1); \
      u8g.setDefaultBackgroundColor(); \
    } \
    u8g.setPrintPos(xstart + 2, y + 1);

#define UI_END_SETTINGS_LOOP() }

#define UI_DRAW_SETTING(attr, value) \
  do { \
    u8g.print(attr.name); \
    if(attr.value_names) \
      u8g.print(attr.value_names[value]); \
    else \
      print_int(value); \
  } while (0)


inline void print_int(int value) {
  if (value >= 0) {
    u8g.print('+'); u8g.print(value);
  } else {
    u8g.print('-'); u8g.print(-value);
  }
}

#endif // UTIL_UI_H_
