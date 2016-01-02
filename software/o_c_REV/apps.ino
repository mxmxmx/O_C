#include "util_app.h"

App available_apps[] = {
  {"ASR", ASR_init, ASR_save, ASR_restore, NULL, ASR_resume,
    _loop, ASR_menu, screensaver, topButton, lowerButton, rightButton, leftButton, NULL, update_ENC},
  {"Harrington 1200", H1200_init, H1200_save, H1200_restore, NULL, H1200_resume,
    H1200_loop, H1200_menu, H1200_screensaver, H1200_topButton, H1200_lowerButton, H1200_rightButton, H1200_leftButton, H1200_leftButtonLong, H1200_encoders},
  {"Automatonnetz", Automatonnetz_init, Automatonnetz_save, Automatonnetz_restore, NULL, Automatonnetz_resume,
    Automatonnetz_loop, Automatonnetz_menu, Automatonnetz_screensaver, Automatonnetz_topButton, Automatonnetz_lowerButton, Automatonnetz_rightButton, Automatonnetz_leftButton, Automatonnetz_leftButtonLong, Automatonnetz_encoders},
  {"VierfStSpQuaMo", QQ_init, QQ_save, QQ_restore, NULL, QQ_resume,
    QQ_loop, QQ_menu, screensaver, QQ_topButton, QQ_lowerButton, QQ_rightButton, QQ_leftButton, QQ_leftButtonLong, QQ_encoders}
};

struct O_CSettings {

  static const size_t kAppSettingsSize =
    ASR_SETTINGS_SIZE +
    H1200_SETTINGS_SIZE +
    AUTOMATONNETZ_SETTINGS_SIZE +
    QQ_SETTINGS_SIZE;

  static const uint32_t FOURCC = FOURCC<'O', 'C', 0, 94>::value;

  uint8_t current_app_index;
  char app_settings[kAppSettingsSize];
  size_t used;
};

#define EEPROM_CALIBRATIONDATA_START 0
#define EEPROM_CALIBRATIONDATA_LENGTH 64 // calibrate.ino: OCTAVES*uint16_t + numADC*unit16_t = 14 * 2 = 28 -> leaves space

O_CSettings global_settings;
typedef PageStorage<EEPROMStorage, EEPROM_CALIBRATIONDATA_LENGTH, EEPROMStorage::LENGTH, O_CSettings> O_CSettingsStorage;
O_CSettingsStorage settings_storage;

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
  u8g.firstPage();  
  u8g.setFont(UI_DEFAULT_FONT);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  do {
    int first = selected - 4;
    if (first < 0) first = 0;

    uint8_t y = 0;
    for (int i = 0, current = first; i < 5 && current < APP_COUNT; ++i, ++current, y += kUiLineH) {
      UI_SETUP_ITEM(0, current == selected);
      if (global_settings.current_app_index == current)
        u8g.print('>');
      else
        u8g.print(' ');
      u8g.print(available_apps[current].name);
    }
  } while (u8g.nextPage());
}

void set_current_app(int index) {
  global_settings.current_app_index = index;
  current_app = &available_apps[index];
}

void init_apps() {
  for (int i = 0; i < APP_COUNT; ++i)
    available_apps[i].init();

  Serial.print("sizeof(settings) : "); Serial.println(sizeof(O_CSettings));
  Serial.print("PAGESIZE         : "); Serial.println(O_CSettingsStorage::PAGESIZE);
  Serial.print("PAGES            : "); Serial.println(O_CSettingsStorage::PAGES);

  if (!settings_storage.load(global_settings) || global_settings.current_app_index >= APP_COUNT) {
    Serial.print("Settings not loaded, using defaults...");
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

  SELECT_APP = false;
  MENU_REDRAW = 1;
  _UI_TIMESTAMP = millis();
}
