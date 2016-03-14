#include <Arduino.h>
#include "OC_bitmaps.h"
#include "OC_config.h"
#include "OC_gpio.h"
#include "OC_ui.h"
#include "util_ui.h"

static constexpr weegfx::coord_t note_circle_r = 28;

static struct coords {
  weegfx::coord_t x, y;
} circle_pos_lut[12];

void init_circle_lut() {
  static const float pi = 3.14159265358979323846f;
  static const float semitone_radians = (2.f * pi / 12.f);

  for (int i = 0; i < 12; ++i) {
    float rads = ((i + 12 - 3) % 12) * semitone_radians;
    float x = note_circle_r * cos(rads);
    float y = note_circle_r * sin(rads);
    circle_pos_lut[i].x = x;
    circle_pos_lut[i].y = y;
  }
}

void visualize_pitch_classes(uint8_t *normalized, weegfx::coord_t centerx, weegfx::coord_t centery) {
  graphics.drawCircle(centerx, centery, note_circle_r);

  coords last_pos = circle_pos_lut[normalized[0]];
  last_pos.x += centerx;
  last_pos.y += centery;
  for (size_t i = 1; i < 3; ++i) {
    graphics.drawBitmap8(last_pos.x - 3, last_pos.y - 3, 8, OC::circle_disk_bitmap_8x8);
    coords current_pos = circle_pos_lut[normalized[i]];
    current_pos.x += centerx;
    current_pos.y += centery;
    graphics.drawLine(last_pos.x, last_pos.y, current_pos.x, current_pos.y);
    last_pos = current_pos;
  }
  graphics.drawLine(last_pos.x, last_pos.y, circle_pos_lut[normalized[0]].x + centerx, circle_pos_lut[normalized[0]].y + centery);
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
    graphics.print("Depress L: Calibrate");
    if (button_left.read_immediate())
      graphics.invertRect(xstart, y, 128, kUiLineH - 1);

    y += kUiLineH;
    graphics.setPrintPos(xstart, y + 1);
    graphics.print("Depress R: Select app");
    if (button_right.read_immediate())
      graphics.invertRect(xstart, y, 128, kUiLineH - 1);

    y += kUiLineH;
    graphics.setPrintPos(xstart, y + 1);
    if (!digitalRead(but_top) && !digitalRead(but_bot))
      graphics.print("    Using defaults!");

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
