#include "util_app.h"

App available_apps[] = {
  {"ASR", ASR_init, _loop, NULL, NULL, ASR_menu, screensaver, topButton, lowerButton, rightButton, leftButton, update_ENC},
  {"Harrington 1200", H1200_init, H1200_loop, NULL, NULL, H1200_menu, H1200_screensaver, H1200_topButton, H1200_lowerButton, H1200_rightButton, H1200_leftButton, H1200_encoders},
  {"QuaQua", QQ_init, QQ_loop, NULL, NULL, QQ_menu, screensaver, QQ_topButton, QQ_lowerButton, QQ_rightButton, QQ_leftButton, QQ_encoders}
};

static const int APP_COUNT = sizeof(available_apps) / sizeof(available_apps[0]);
int current_app_index = 2;
App *current_app = &available_apps[current_app_index];
bool SELECT_APP = false;
static const uint32_t SELECT_APP_TIMEOUT = 15000;

void draw_app_menu(int selected) {
  u8g.setFont(u8g_font_6x12);
  u8g.firstPage();
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  u8g.firstPage();  
  do {
    uint8_t x = 4, y = 0;
    for (int i = 0; i < APP_COUNT; ++i, y += 16) {
      if (i == selected) {
        u8g.setDefaultForegroundColor();
        u8g.drawBox(0, y, 128, 15);
        u8g.setDefaultBackgroundColor();
      } else {
        u8g.setDefaultForegroundColor();
      }

      u8g.setPrintPos(x, y + 2);
      if (current_app_index == i)
        u8g.print('>');
      else
        u8g.print(' ');
      u8g.print(available_apps[i].name);
    }
  } while (u8g.nextPage());
}

void set_current_app(int index) {
  current_app_index = index;
  current_app = &available_apps[current_app_index];
}

void init_apps() {
  for (int i = 0; i < APP_COUNT; ++i)
    available_apps[i].init();

  if (!digitalRead(but_top)) {
    set_current_app(0);
    draw_app_menu(0);
    while (!digitalRead(but_top));
  } else if (!digitalRead(but_bot)) {
    set_current_app(2);
    draw_app_menu(2);
    while (!digitalRead(but_bot));
  } else if (!digitalRead(butR)) {
    select_app();
  } else {
    set_current_app(1);
    delay(500);
  }
}

void select_app() {

  // Save state
  int encoder_values[2] = { encoder[LEFT].pos(), encoder[RIGHT].pos() };
  if (current_app->suspend)
    current_app->suspend();

  int selected = current_app_index;
  encoder[RIGHT].setPos(selected);

  draw_app_menu(selected);
  while (!digitalRead(butR));

  uint32_t time = millis();
  bool redraw = true;
  while (!(millis() - time > SELECT_APP_TIMEOUT)) {
    if (_ENC) {
      _ENC = false;
      int value = encoder[RIGHT].pos();
      if (value < 0) value = 0;
      else if (value >= APP_COUNT) value = APP_COUNT - 1;
      encoder[RIGHT].setPos(value);
      if (value != selected) {
        selected = value;
        redraw = true;
      }
    }
    button_right.read();
    if (button_right.event())
      break;

    if (redraw) {
      draw_app_menu(selected);
      redraw = false;
    }
  }

  if (selected != current_app_index)
    set_current_app(selected);

  // Restore state
  encoder[LEFT].setPos(encoder_values[0]);
  encoder[RIGHT].setPos(encoder_values[1]);

  if (current_app->resume)
    current_app->resume();

  SELECT_APP = false;
  MENU_REDRAW = 1;
  _UI_TIMESTAMP = millis();
}
