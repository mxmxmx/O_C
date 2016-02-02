#include "util_app.h"

App available_apps[] = {
  {"ASR", ASR_init, ASR_save, ASR_restore, NULL, ASR_resume,
    _loop, ASR_menu, screensaver, topButton, lowerButton, rightButton, leftButton, NULL, update_ENC, NULL},
  {"Harrington 1200", H1200_init, H1200_save, H1200_restore, NULL, H1200_resume,
    H1200_loop, H1200_menu, H1200_screensaver, H1200_topButton, H1200_lowerButton, H1200_rightButton, H1200_leftButton, H1200_leftButtonLong, H1200_encoders, NULL},
  {"Automatonnetz", Automatonnetz_init, Automatonnetz_save, Automatonnetz_restore, NULL, Automatonnetz_resume,
    Automatonnetz_loop, Automatonnetz_menu, Automatonnetz_screensaver, Automatonnetz_topButton, Automatonnetz_lowerButton, Automatonnetz_rightButton, Automatonnetz_leftButton, Automatonnetz_leftButtonLong, Automatonnetz_encoders, NULL},
  {"VierfStSpQuaMo", QQ_init, QQ_save, QQ_restore, NULL, QQ_resume,
    QQ_loop, QQ_menu, screensaver, QQ_topButton, QQ_lowerButton, QQ_rightButton, QQ_leftButton, QQ_leftButtonLong, QQ_encoders, QQ_isr},
  {"frames::poly_lfo", POLYLFO_init, POLYLFO_save, POLYLFO_restore, NULL, POLYLFO_resume,
    POLYLFO_loop, POLYLFO_menu, POLYLFO_screensaver, POLYLFO_topButton, POLYLFO_lowerButton, POLYLFO_rightButton, POLYLFO_leftButton, POLYLFO_leftButtonLong, POLYLFO_encoders, POLYLFO_isr},
};

namespace OC {

struct Settings {

  static const size_t kAppSettingsSize =
    ASR_SETTINGS_SIZE +
    H1200_SETTINGS_SIZE +
    AUTOMATONNETZ_SETTINGS_SIZE +
    QQ_SETTINGS_SIZE +
    POLYLFO_SETTINGS_SIZE;

  static const uint32_t FOURCC = FOURCC<'O', 'C', 0, 97>::value;

  uint8_t current_app_index;
  char app_settings[kAppSettingsSize];
  size_t used;
};

typedef PageStorage<EEPROMStorage, EEPROM_CALIBRATIONDATA_LENGTH, EEPROMStorage::LENGTH, Settings> SettingsStorage;
}

OC::Settings global_settings;
OC::SettingsStorage settings_storage;

static const int APP_COUNT = sizeof(available_apps) / sizeof(available_apps[0]);
App *current_app = &available_apps[0];
bool SELECT_APP = false;
static const uint32_t SELECT_APP_TIMEOUT = 15000;
static const int DEFAULT_APP_INDEX = 1;


void save_app_settings() {
  char *storage = global_settings.app_settings;
  global_settings.used = 0;
  for (int i = 0; i < APP_COUNT; ++i) {
    if (available_apps[i].save) {
      size_t used = available_apps[i].save(storage);
      storage += used;
      global_settings.used += used;
    }
  }
  Serial.print("App settings saved: "); Serial.println(global_settings.used);
}

void restore_app_settings() {
  const char *storage = global_settings.app_settings;
  size_t restored_bytes = 0;
  for (int i = 0; i < APP_COUNT; ++i) {
    if (available_apps[i].restore) {
      size_t used = available_apps[i].restore(storage);
      storage += used;
      restored_bytes += used;
    }
  }
  Serial.print("App settings restored: "); Serial.print(restored_bytes);
  Serial.print(", expected: "); Serial.println(global_settings.used);
}

void draw_app_menu(int selected) {
  GRAPHICS_BEGIN_FRAME(true);

  graphics.setFont(UI_DEFAULT_FONT);
  int first = selected - 4;
  if (first < 0) first = 0;

  uint8_t y = 0;
  const weegfx::coord_t xstart = 0;
  for (int i = 0, current = first; i < 5 && current < APP_COUNT; ++i, ++current, y += kUiLineH) {
    UI_SETUP_ITEM(current == selected);
    if (global_settings.current_app_index == current)
      graphics.print('>');
    else
      graphics.print(' ');
    graphics.print(available_apps[current].name);
    UI_END_ITEM();
  }

  GRAPHICS_END_FRAME();
}

void set_current_app(int index) {
  global_settings.current_app_index = index;
  current_app = &available_apps[index];
}

void init_apps() {
  for (int i = 0; i < APP_COUNT; ++i)
    available_apps[i].init();

  Serial.println("Loading app settings...");
  Serial.print("sizeof(settings) : "); Serial.println(sizeof(OC::Settings));
  Serial.print("PAGESIZE         : "); Serial.println(OC::SettingsStorage::PAGESIZE);
  Serial.print("PAGES            : "); Serial.println(OC::SettingsStorage::PAGES);

  if (!settings_storage.load(global_settings) || global_settings.current_app_index >= APP_COUNT) {
    Serial.println("Settings not loaded, using defaults...");
    global_settings.current_app_index = DEFAULT_APP_INDEX;
  } else {
    Serial.println("Loaded settings...");
    Serial.print("page_index       : "); Serial.println(settings_storage.page_index());
    Serial.print("current_app_index: "); Serial.println(global_settings.current_app_index);
    restore_app_settings();
  }
  set_current_app(global_settings.current_app_index);
  if (current_app->resume)
    current_app->resume();

  LAST_ENCODER_VALUE[LEFT] = encoder[LEFT].pos();
  LAST_ENCODER_VALUE[RIGHT] = encoder[RIGHT].pos();

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
    delay(500);
  }
}

void debug_menu() {
  while (true) {

    GRAPHICS_BEGIN_FRAME(false);
      graphics.setPrintPos(2, 2); 
      graphics.print("CV1: "); graphics.pretty_print(OC::ADC::value<ADC_CHANNEL_1>(), 6);

      graphics.setPrintPos(2, 12);
      graphics.print("CV2: "); graphics.pretty_print(OC::ADC::value<ADC_CHANNEL_2>(), 6);

      graphics.setPrintPos(2, 22);
      graphics.print("CV3: "); graphics.pretty_print(OC::ADC::value<ADC_CHANNEL_3>(), 6);

      graphics.setPrintPos(2, 32);
      graphics.print("CV4: "); graphics.pretty_print(OC::ADC::value<ADC_CHANNEL_4>(), 6);

//      graphics.setPrintPos(2, 42);
//      graphics.print((long)OC::ADC::busy_waits());
//      graphics.setPrintPos(2, 42); graphics.print(OC::ADC::fail_flag0());
//      graphics.setPrintPos(2, 52); graphics.print(OC::ADC::fail_flag1());
    GRAPHICS_END_FRAME();

    button_left.read();
    if (button_left.event())
      break;
  }
}

void select_app() {

  // Save state
  int encoder_values[2] = { encoder[LEFT].pos(), encoder[RIGHT].pos() };
  if (current_app->suspend)
    current_app->suspend();

  int selected = global_settings.current_app_index;
  encoder[RIGHT].setPos(selected);

  draw_app_menu(selected);
  while (!digitalRead(butR));

  uint32_t time = millis();
  bool redraw = true;
  bool save = false;
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

    button_left.read();
    if (button_left.event()) {
      debug_menu();
      time = millis();
      redraw = true;
    }
    button_right.read();
    if (button_right.long_event()) {
      save = true;
      break;
    }
    else if (button_right.event())
      break;

    if (redraw) {
      draw_app_menu(selected);
      redraw = false;
    }
  }

  set_current_app(selected);
  if (save) {
    Serial.println("Saving settings...");
    save_app_settings();
    settings_storage.save(global_settings);
    Serial.print("page_index       : "); Serial.println(settings_storage.page_index());
  }

  // Restore state
  encoder[LEFT].setPos(encoder_values[0]);
  encoder[RIGHT].setPos(encoder_values[1]);

  if (current_app->resume)
    current_app->resume();

  LAST_ENCODER_VALUE[LEFT] = encoder[LEFT].pos();
  LAST_ENCODER_VALUE[RIGHT] = encoder[RIGHT].pos();

  SELECT_APP = false;
  MENU_REDRAW = 1;
  _UI_TIMESTAMP = millis();
}
