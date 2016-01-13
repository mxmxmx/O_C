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
  graphics.setDefaultForegroundColor(); \
  graphics.setPrintPos(xstart + 2, kUiTitleTextY); \
  graphics.drawHLine(xstart, kUiDefaultFontH, kUiDisplayWidth); \
  } while (0)

#define UI_SETUP_ITEM(xstart, sel) \
  do { \
    graphics.setDefaultForegroundColor(); \
    if (sel) { \
      graphics.drawBox(xstart, y, kUiDisplayWidth - xstart, kUiLineH - 1); \
      graphics.setDefaultBackgroundColor(); \
    } \
    graphics.setPrintPos(xstart + 2, y + 1); \
  } while (0)

#define UI_START_MENU(xstart) \
  graphics.setDefaultForegroundColor(); \
  weegfx::coord_t y = kUiItemsStartY; \
  graphics.setPrintPos(xstart + 2, y + 1); \
  do {} while (0)


#define UI_BEGIN_ITEMS_LOOP(xstart, first, last, selected, startline) \
  y = kUiItemsStartY + startline * kUiLineH; \
  for (int i = 0, current_item = first; i < kUiVisibleItems - startline && current_item < last; ++i, ++current_item, y += kUiLineH) { \
    UI_SETUP_ITEM(xstart, selected == current_item);

#define UI_END_ITEMS_LOOP() } do {} while (0)

#define UI_DRAW_SETTING(attr, value, xoffset) \
  do { \
    const int val = value; \
    graphics.print(attr.name); \
    if (xoffset) graphics.setPrintPos(xoffset, y + 1); \
    if(attr.value_names) \
      graphics.print(attr.value_names[val]); \
    else \
      graphics.pretty_print(val); \
  } while (0)

#endif // UTIL_UI_H_
