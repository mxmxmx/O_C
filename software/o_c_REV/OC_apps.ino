#include "OC_apps.h"

extern int LAST_ENCODER_VALUE[2];


OC::App available_apps[] = {
  {"ASR", ASR_init, ASR_save, ASR_restore, NULL, ASR_resume,
    ASR_loop, ASR_menu, screensaver, ASR_topButton, ASR_lowerButton, ASR_rightButton, ASR_leftButton, ASR_leftButtonLong, ASR_encoders, ASR_isr},
  {"Harrington 1200", H1200_init, H1200_save, H1200_restore, NULL, H1200_resume,
    H1200_loop, H1200_menu, H1200_screensaver, H1200_topButton, H1200_lowerButton, H1200_rightButton, H1200_leftButton, H1200_leftButtonLong, H1200_encoders, NULL},
  {"Automatonnetz", Automatonnetz_init, Automatonnetz_save, Automatonnetz_restore, NULL, Automatonnetz_resume,
    Automatonnetz_loop, Automatonnetz_menu, Automatonnetz_screensaver, Automatonnetz_topButton, Automatonnetz_lowerButton, Automatonnetz_rightButton, Automatonnetz_leftButton, Automatonnetz_leftButtonLong, Automatonnetz_encoders, NULL},
  {"VierfStSpQuaMo", QQ_init, QQ_save, QQ_restore, NULL, QQ_resume,
    QQ_loop, QQ_menu, screensaver, QQ_topButton, QQ_lowerButton, QQ_rightButton, QQ_leftButton, QQ_leftButtonLong, QQ_encoders, QQ_isr},
  {"frames::poly_lfo", POLYLFO_init, POLYLFO_save, POLYLFO_restore, NULL, POLYLFO_resume,
    POLYLFO_loop, POLYLFO_menu, POLYLFO_screensaver, POLYLFO_topButton, POLYLFO_lowerButton, POLYLFO_rightButton, POLYLFO_leftButton, POLYLFO_leftButtonLong, POLYLFO_encoders, POLYLFO_isr},
};
static constexpr int APP_COUNT = sizeof(available_apps) / sizeof(available_apps[0]);

namespace OC {

extern void debug_menu();

struct GlobalSettings {
  static constexpr uint32_t FOURCC = FOURCC<'O','C','S',0>::value;

  uint8_t current_app_index;
  OC::Scale user_scales[OC::Scales::SCALE_USER_LAST];
};

struct AppData {
  static constexpr uint32_t FOURCC = FOURCC<'O','C','A',0>::value;

  static const size_t kAppDataSize =
    ASR_SETTINGS_SIZE +
    H1200_SETTINGS_SIZE +
    AUTOMATONNETZ_SETTINGS_SIZE +
    QQ_SETTINGS_SIZE +
    POLYLFO_SETTINGS_SIZE;

  char data[kAppDataSize];
  size_t used;
};

typedef PageStorage<EEPROMStorage, EEPROM_GLOBALSETTINGS_START, EEPROM_GLOBALSETTINGS_END, GlobalSettings> GlobalSettingsStorage;
typedef PageStorage<EEPROMStorage, EEPROM_APPDATA_START, EEPROM_APPDATA_END, AppData> AppDataStorage;
};

OC::GlobalSettings global_settings;
OC::GlobalSettingsStorage global_settings_storage;

OC::AppData app_settings;
OC::AppDataStorage app_data_storage;

OC::App *OC::current_app = &available_apps[0];
bool SELECT_APP = false;
static const uint32_t SELECT_APP_TIMEOUT = 15000;
static const int DEFAULT_APP_INDEX = 1;


void save_global_settings() {
  Serial.println("Saving global settings...");

  memcpy(global_settings.user_scales, OC::user_scales, sizeof(OC::user_scales));

  global_settings_storage.save(global_settings);
  Serial.print("page_index       : "); Serial.println(global_settings_storage.page_index());
}

void save_app_data() {
  Serial.println("Saving app data...");

  char *storage = app_settings.data;
  app_settings.used = 0;
  for (int i = 0; i < APP_COUNT; ++i) {
    if (available_apps[i].save) {
      size_t used = available_apps[i].save(storage);
      storage += used;
      app_settings.used += used;
    }
  }
  Serial.print("App settings used: "); Serial.println(app_settings.used);
  app_data_storage.save(app_settings);
  Serial.print("page_index       : "); Serial.println(app_data_storage.page_index());
}

void restore_app_data() {

  Serial.println("Restoring app data...");

  const char *storage = app_settings.data;
  size_t restored_bytes = 0;
  for (int i = 0; i < APP_COUNT; ++i) {
    if (available_apps[i].restore) {
      size_t used = available_apps[i].restore(storage);
      storage += used;
      restored_bytes += used;
    }
  }
  Serial.print("App data restored: "); Serial.print(restored_bytes);
  Serial.print(", expected: "); Serial.println(app_settings.used);
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
  OC::current_app = &available_apps[index];
}

void OC::APPS::Init() {

  OC::Scales::Init();
  for (auto &app : available_apps)
    app.init();

  if (!digitalRead(but_top) && !digitalRead(but_bot)) {
    Serial.println("Skipping loading of global/app settings");
    global_settings_storage.Init();
    app_data_storage.Init();
  } else {
    Serial.print("Loading global settings: "); Serial.println(sizeof(OC::GlobalSettings));
    Serial.print(" PAGESIZE: "); Serial.println(OC::GlobalSettingsStorage::PAGESIZE);
    Serial.print(" PAGES   : "); Serial.println(OC::GlobalSettingsStorage::PAGES);

    if (!global_settings_storage.load(global_settings) || global_settings.current_app_index >= APP_COUNT) {
      Serial.println("Settings not loaded or invalid, using defaults...");
      global_settings.current_app_index = DEFAULT_APP_INDEX;
    } else {
      Serial.print("Loaded settings, current_app_index is ");
      Serial.println(global_settings.current_app_index);
      memcpy(OC::user_scales, global_settings.user_scales, sizeof(OC::user_scales));
    }

    Serial.print("Loading app data: "); Serial.println(sizeof(OC::AppData));
    Serial.print(" PAGESIZE: "); Serial.println(OC::AppDataStorage::PAGESIZE);
    Serial.print(" PAGES   : "); Serial.println(OC::AppDataStorage::PAGES);

    if (!app_data_storage.load(app_settings)) {
      Serial.println("App data not loaded, using defaults...");
    } else {
      restore_app_data();
    }
  }

  set_current_app(global_settings.current_app_index);
  if (OC::current_app->resume)
    OC::current_app->resume();

  LAST_ENCODER_VALUE[LEFT] = encoder[LEFT].pos();
  LAST_ENCODER_VALUE[RIGHT] = encoder[RIGHT].pos();

  if (!digitalRead(butR))
    SELECT_APP = true;
  else
    delay(500);
}

void OC::APPS::Select() {

  // Save state
  int encoder_values[2] = { encoder[LEFT].pos(), encoder[RIGHT].pos() };
  if (OC::current_app->suspend)
    OC::current_app->suspend();

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
      Serial.println("DEBUG MENU");
      OC::debug_menu();
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
    save_global_settings();
    save_app_data();
  }

  // Restore state
  encoder[LEFT].setPos(encoder_values[0]);
  encoder[RIGHT].setPos(encoder_values[1]);

  if (OC::current_app->resume)
    OC::current_app->resume();

  LAST_ENCODER_VALUE[LEFT] = encoder[LEFT].pos();
  LAST_ENCODER_VALUE[RIGHT] = encoder[RIGHT].pos();

  SELECT_APP = false;
  MENU_REDRAW = 1;
  _UI_TIMESTAMP = millis();
}
