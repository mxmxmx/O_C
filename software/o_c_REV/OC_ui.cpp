#include <Arduino.h>
#include "OC_bitmaps.h"
#include "OC_config.h"
#include "OC_gpio.h"
#include "OC_ui.h"
#include "util_ui.h"

static const weegfx::coord_t note_circle_x = 32;
static const weegfx::coord_t note_circle_y = 32;
static const weegfx::coord_t note_circle_r = 28;

struct coords {
  weegfx::coord_t x, y;
} circle_pos_lut[12];

void init_circle_lut() {
  static const float pi = 3.14159265358979323846f;
  static const float semitone_radians = (2.f * pi / 12.f);

  for (int i = 0; i < 12; ++i) {
    float rads = ((i + 12 - 3) % 12) * semitone_radians;
    float x = note_circle_r * cos(rads);
    float y = note_circle_r * sin(rads);
    circle_pos_lut[i].x = note_circle_x + x;
    circle_pos_lut[i].y = note_circle_y + y;
  }
}

void visualize_pitch_classes(uint8_t *normalized) {
  graphics.drawCircle(note_circle_x, note_circle_y, note_circle_r);

  coords last_pos = circle_pos_lut[normalized[0]];
  for (size_t i = 1; i < 3; ++i) {
    graphics.drawBitmap8(last_pos.x - 3, last_pos.y - 3, 8, OC::circle_disk_bitmap_8x8);
    const coords &current_pos = circle_pos_lut[normalized[i]];
    graphics.drawLine(last_pos.x, last_pos.y, current_pos.x, current_pos.y);
    last_pos = current_pos;
  }
  graphics.drawLine(last_pos.x, last_pos.y, circle_pos_lut[normalized[0]].x, circle_pos_lut[normalized[0]].y);
  graphics.drawBitmap8(last_pos.x - 3, last_pos.y - 3, 8, OC::circle_disk_bitmap_8x8);
}

namespace OC {

/* -----------  splash  ------------------- */  
void hello() {

  unsigned long start = millis();
  unsigned long now = start;
  do {
    now = millis();

    GRAPHICS_BEGIN_FRAME(true);

    UI_DRAW_TITLE(0);
    graphics.print("Ornaments & Crimes");

    UI_START_MENU(0);
    y += kUiLineH / 2;

    graphics.setPrintPos(xstart, y + 1);
    graphics.print("[L] Calibrate");
    if (button_left.read_immediate())
      graphics.invertRect(xstart, y, 128, kUiLineH - 1);

    y += kUiLineH;
    graphics.setPrintPos(xstart, y + 1);
    graphics.print("[R] Select app");
    if (button_right.read_immediate())
      graphics.invertRect(xstart, y, 128, kUiLineH - 1);

    weegfx::coord_t w;
    if (now - start < SPLASHSCREEN_DELAY_MS)
      w = 128;
    else
      w = ((start + SPLASHSCREEN_DELAY_MS + SPLASHSCREEN_TIMEOUT_MS - now) << 7) / SPLASHSCREEN_TIMEOUT_MS;
    graphics.drawRect(0, 62, w, 2);

    GRAPHICS_END_FRAME();

  } while (now - start < SPLASHSCREEN_TIMEOUT_MS + SPLASHSCREEN_DELAY_MS);
}

}
