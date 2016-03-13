#ifndef UTIL_UI_H_
#define UTIL_UI_H_

#include "weegfx.h"
#include "util_framebuffer.h"
#include "SH1106_128x64_Driver.h"
#include "util/util_button.h"
#include "util/util_debugpins.h"

extern TimerDebouncedButton<butL, 50, 2000> button_left;
extern TimerDebouncedButton<butR, 50, 2000> button_right;

extern weegfx::Graphics graphics;
extern FrameBuffer<SH1106_128x64_Driver::kFrameSize, 2> frame_buffer;

extern uint_fast8_t MENU_REDRAW;
extern unsigned long LAST_REDRAW_TIME;


#define UI_DEFAULT_FONT nullptr

static const uint8_t kUiDefaultFontH = 12;
static const uint8_t kUiTitleTextY = 2;
static const uint8_t kUiItemsStartY = kUiDefaultFontH + 2 + 1;
static const uint8_t kUiLineH = 12;

static const uint8_t kUiVisibleItems = 4;
static const uint8_t kUiDisplayWidth = 128;

static const uint8_t kUiWideMenuCol1X = 96;

#define UI_DRAW_TITLE(xstart) \
  do { \
  graphics.setPrintPos(xstart + 2, kUiTitleTextY); \
  graphics.drawHLine(xstart, kUiDefaultFontH, kUiDisplayWidth); \
  } while (0)

#define UI_SETUP_ITEM(sel) \
  const bool __selected = sel; \
  do { \
    graphics.setPrintPos(xstart + 2, y + 1); \
  } while (0)

#define UI_END_ITEM() \
  do { \
    if (__selected) \
      graphics.invertRect(xstart, y, kUiDisplayWidth - xstart, kUiLineH - 1); \
  } while (0)


#define UI_START_MENU(x) \
  const weegfx::coord_t xstart = x; \
  weegfx::coord_t y = kUiItemsStartY; \
  graphics.setPrintPos(xstart + 2, y + 1); \
  do {} while (0)


#define UI_BEGIN_ITEMS_LOOP(xstart, first, last, selected, startline) \
  y = kUiItemsStartY + startline * kUiLineH; \
  for (int i = 0, current_item = first; i < kUiVisibleItems - startline && current_item < last; ++i, ++current_item, y += kUiLineH) { \
    UI_SETUP_ITEM(selected == current_item);

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
    UI_END_ITEM(); \
  } while (0)

#define UI_DRAW_EDITABLE(b) \
  do { \
     if (__selected && (b)) { \
      graphics.print(' '); \
      graphics.drawBitmap8(1, y + 1, 6, OC::bitmap_edit_indicator_6x8); \
    } \
 } while (0)

#define GRAPHICS_BEGIN_FRAME(wait) \
do { \
  DEBUG_PIN_SCOPE(DEBUG_PIN_1); \
  uint8_t *frame = NULL; \
  do { \
    if (frame_buffer.writeable()) \
      frame = frame_buffer.writeable_frame(); \
  } while (!frame && wait); \
  if (frame) { \
    graphics.Begin(frame, true); \
    do {} while(0)

#define GRAPHICS_END_FRAME() \
    graphics.End(); \
    frame_buffer.written(); \
    MENU_REDRAW = 0; \
    LAST_REDRAW_TIME = millis(); \
  } \
} while (0)

#endif // UTIL_UI_H_
