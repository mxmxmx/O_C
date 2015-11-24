#include "util_app.h"

App available_apps[] = {
  {"ASR", ASR_init, _loop, ASR_menu, screensaver, topButton, lowerButton, rightButton, leftButton, update_ENC},
  {"Harrington 1200", H1200_init, H1200_loop, H1200_menu, H1200_screensaver, H1200_topButton, H1200_lowerButton, H1200_rightButton, H1200_leftButton, H1200_encoders},
  {"QuaQua", QQ_init, QQ_loop, QQ_menu, screensaver, QQ_topButton, QQ_lowerButton, QQ_rightButton, QQ_leftButton, QQ_encoders}
};

static const size_t APP_COUNT = sizeof(available_apps) / sizeof(available_apps[0]);
size_t current_app_index = 2;
App *current_app = &available_apps[current_app_index];
bool SELECT_APP = false;

void draw_app_menu() {
  u8g.setFont(u8g_font_6x12);
  u8g.firstPage();
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  u8g.firstPage();  
  do {
    u8g.setDefaultForegroundColor();
    uint8_t x = 4, y = 16;
    for (size_t i = 0; i < APP_COUNT; ++i, y += 16) {
      u8g.setPrintPos(x, y);
      if (current_app_index == i)
        u8g.print('>');
      else
        u8g.print(' ');
      u8g.print(available_apps[i].name);
    }
  } while (u8g.nextPage());
}

void set_current_app(size_t index) {
  current_app_index = index;
  current_app = &available_apps[current_app_index];
  draw_app_menu();
}

void init_apps() {
  for (size_t i = 0; i < APP_COUNT; ++i)
    available_apps[i].init();

  if (!digitalRead(but_top)) {
    set_current_app(0);
    while (!digitalRead(but_top));
  } else if (!digitalRead(but_bot)) {
    set_current_app(2);
    while (!digitalRead(but_bot));
  } else {
    set_current_app(1);
    delay(500);
  }
}

void select_app() {
  SELECT_APP = false;
}
