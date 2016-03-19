#include <Arduino.h>
#include <algorithm>

#include "OC_apps.h"
#include "OC_bitmaps.h"
#include "OC_config.h"
#include "OC_gpio.h"
#include "OC_menus.h"
#include "OC_ui.h"

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

Ui ui;

void Ui::Init() {
  ticks_ = 0;

  static const int button_pins[] = { but_top, but_bot, butL, butR };
  for (size_t i = 0; i < CONTROL_BUTTON_LAST; ++i) {
    buttons_[i].Init(button_pins[i], OC_GPIO_BUTTON_PINMODE);
  }
  std::fill(button_press_time_, button_press_time_ + 4, 0);
  button_state_ = 0;
  button_ignore_mask_ = 0;
  screensaver_ = false;

  encoder_right_.Init(OC_GPIO_ENC_PINMODE);
  encoder_left_.Init(OC_GPIO_ENC_PINMODE);

  event_queue_.Init();
}

void FASTRUN Ui::Poll() {

  uint32_t now = ++ticks_;
  uint16_t button_state = 0;

  for (size_t i = 0; i < CONTROL_BUTTON_LAST; ++i) {
    buttons_[i].Poll();
    if (buttons_[i].pressed())
      button_state |= control_mask(i);
  }

  for (size_t i = 0; i < CONTROL_BUTTON_LAST; ++i) {
    auto &button = buttons_[i];
    if (button.just_pressed()) {
      button_press_time_[i] = now;
    } else if (button.released()) {
      if (now - button_press_time_[i] < kLongPressTicks)
        PushEvent(UI::EVENT_BUTTON_PRESS, control_mask(i), 0, button_state);
      else
        PushEvent(UI::EVENT_BUTTON_LONG_PRESS, control_mask(i), 0, button_state);
    }
  }

  encoder_right_.Poll();
  encoder_left_.Poll();

  int32_t increment;
  increment = encoder_right_.Read();
  if (increment)
    PushEvent(UI::EVENT_ENCODER, CONTROL_ENCODER_R, increment, button_state);

  increment = encoder_left_.Read();
  if (increment)
    PushEvent(UI::EVENT_ENCODER, CONTROL_ENCODER_L, increment, button_state);

  button_state_ = button_state;
}

UiMode Ui::DispatchEvents(App *app) {

  while (event_queue_.available()) {
    const UI::Event event = event_queue_.PullEvent();
    if (IgnoreEvent(event))
      continue;

    if (event.type == UI::EVENT_BUTTON_LONG_PRESS && event.control == OC::CONTROL_BUTTON_R)
      return UI_MODE_SELECT_APP;

    switch (event.type) {
      case UI::EVENT_BUTTON_PRESS:
        switch (event.control) {
          case OC::CONTROL_BUTTON_UP: app->top_button(); break;
          case OC::CONTROL_BUTTON_DOWN: app->lower_button(); break;
          case OC::CONTROL_BUTTON_L: app->left_button(); break;
          case OC::CONTROL_BUTTON_R: app->right_button(); break;
          default: break;
        }
        break;
      case UI::EVENT_BUTTON_LONG_PRESS:
        app->left_button_long();
        break;

      case UI::EVENT_ENCODER:
        app->HandleEncoderEvent(event);
        break;
      default:
        break;
    }
    MENU_REDRAW = 1;
  }

  if (idle_time() > SCREENSAVER_TIMEOUT_MS) {
    if (!screensaver_)
      screensaver_ = true;
    return UI_MODE_SCREENSAVER;
  } else {
    return UI_MODE_MENU;
  }
}

UiMode Ui::Splashscreen(bool &use_defaults) {

  UiMode mode = UI_MODE_MENU;

  unsigned long start = millis();
  unsigned long now = start;
  do {

    if (read_immediate(CONTROL_BUTTON_L))
      mode = UI_MODE_CALIBRATE;
    if (read_immediate(CONTROL_BUTTON_R))
      mode = UI_MODE_SELECT_APP;

    use_defaults = 
      read_immediate(CONTROL_BUTTON_UP) && read_immediate(CONTROL_BUTTON_DOWN);

    now = millis();

    GRAPHICS_BEGIN_FRAME(true);

    UI_DRAW_TITLE(0);
    graphics.print("Ornaments & Crimes");
 
    UI_START_MENU(0);
    y += kUiLineH / 2;

    graphics.setPrintPos(xstart, y + 2);
    graphics.print("[L] => calibration");
    if (UI_MODE_CALIBRATE == mode)
      graphics.invertRect(xstart, y, 128, kUiLineH);

    y += kUiLineH;
    graphics.setPrintPos(xstart, y + 2);
    graphics.print("[R] => select app");
    if (UI_MODE_SELECT_APP == mode)
      graphics.invertRect(xstart, y, 128, kUiLineH);

    y += kUiLineH;
    graphics.setPrintPos(xstart, y + 1);
    if (use_defaults)
      graphics.print("Reset EEPROM!");

    weegfx::coord_t w;
    if (now - start < SPLASHSCREEN_DELAY_MS)
      w = 128;
    else
      w = ((start + SPLASHSCREEN_DELAY_MS + SPLASHSCREEN_TIMEOUT_MS - now) << 7) / SPLASHSCREEN_TIMEOUT_MS;
    graphics.drawRect(0, 62, w, 2);

    GRAPHICS_END_FRAME();

  } while (now - start < SPLASHSCREEN_TIMEOUT_MS + SPLASHSCREEN_DELAY_MS);

  SetButtonIgnoreMask();
  return mode;
}

} // namespace OC
