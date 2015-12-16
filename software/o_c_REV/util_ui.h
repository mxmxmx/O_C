#ifndef UTIL_UI_H_
#define UTIL_UI_H_

#define UI_DEFAULT_FONT u8g_font_6x12

static const uint8_t kUiDefaultFontH = 12;
static const uint8_t kUiTitleTextY = 0;
static const uint8_t kUiItemsStartY = kUiDefaultFontH + 2 + 1;
static const uint8_t kUiLineH = 12;

static const uint8_t kUiVisibleItems = 4;
static const uint8_t kUiDisplayWidth = 128;

static const uint8_t kUiWideMenuCol1X = 96;

#define UI_DRAW_TITLE(xstart) \
  do { \
  u8g.setDefaultForegroundColor(); \
  u8g.setPrintPos(xstart + 2, kUiTitleTextY); \
  u8g.drawHLine(xstart, kUiDefaultFontH, kUiDisplayWidth - xstart - 2); \
  } while (0)

#define UI_SETUP_ITEM(xstart, sel) \
  do { \
    u8g.setDefaultForegroundColor(); \
    if (sel) { \
      u8g.drawBox(xstart, y, kUiDisplayWidth - xstart - 2, kUiLineH - 1); \
      u8g.setDefaultBackgroundColor(); \
    } \
    u8g.setPrintPos(xstart + 2, y + 1); \
  } while (0)

#define UI_START_MENU(xstart) \
  u8g.setDefaultForegroundColor(); \
  uint8_t y = kUiItemsStartY; \
  u8g.setPrintPos(xstart + 2, y + 1); \
  do {} while (0)


#define UI_BEGIN_ITEMS_LOOP(xstart, first, last, selected, startline) \
  y = kUiItemsStartY + startline * kUiLineH; \
  for (int i = 0, current_item = first; i < kUiVisibleItems - startline && current_item < last; ++i, ++current_item, y += kUiLineH) { \
    UI_SETUP_ITEM(xstart, selected == current_item);

#define UI_END_ITEMS_LOOP() } do {} while (0)

#define UI_DRAW_SETTING(attr, value, xoffset) \
  do { \
    const int val = value; \
    u8g.print(attr.name); \
    if (xoffset) u8g.setPrintPos(xoffset, y + 1); \
    if(attr.value_names) \
      u8g.print(attr.value_names[val]); \
    else \
      print_int(val); \
  } while (0)

inline void print_int(int value) {
  if (value >= 0) {
    u8g.print('+'); u8g.print(value);
  } else {
    u8g.print('-'); u8g.print(-value);
  }
}

#endif // UTIL_UI_H_
