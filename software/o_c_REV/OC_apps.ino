#include "OC_apps.h"

extern int LAST_ENCODER_VALUE[2];

#define DECLARE_APP(a, b, name, prefix, isr) \
{ TWOCC<a,b>::value, name, \
  prefix ## _init, prefix ## _storageSize, prefix ## _save, prefix ## _restore, \
  prefix ## _handleEvent, \
  prefix ## _loop, prefix ## _menu, prefix ## _screensaver, \
  prefix ## _topButton, prefix ## _lowerButton, \
  prefix ## _rightButton, prefix ## _leftButton, prefix ## _leftButtonLong, \
  prefix ## _encoders, \
  isr \
}

#define ASR_screensaver screensaver
#define QQ_screensaver screensaver
// #define ENVGEN_screensaver screensaver

OC::App available_apps[] = {
  DECLARE_APP('A','S', "CopierMaschine", ASR, ASR_isr),
  DECLARE_APP('H','A', "Harrington 1200", H1200, H1200_isr),
  DECLARE_APP('A','T', "Automatonnetz", Automatonnetz, Automatonnetz_isr),
  DECLARE_APP('Q','Q', "Quantermain", QQ, QQ_isr),
  DECLARE_APP('P','L', "Quadraturia", POLYLFO, POLYLFO_isr),
  DECLARE_APP('L','R', "Low-rents", LORENZ, LORENZ_isr),
  DECLARE_APP('E','G', "Piqued", ENVGEN, ENVGEN_isr),
};

static constexpr int APP_COUNT = sizeof(available_apps) / sizeof(available_apps[0]);

namespace OC {

extern void debug_menu();

struct GlobalSettings {
  static constexpr uint32_t FOURCC = FOURCC<'O','C','S',1>::value;

  // TODO: Save app id instead of index?

  uint8_t current_app_index;
  OC::Scale user_scales[OC::Scales::SCALE_USER_LAST];
};

struct AppChunkHeader {
  uint16_t id;
  uint16_t length;
} __attribute__((packed));

struct AppData {
  static constexpr uint32_t FOURCC = FOURCC<'O','C','A',2>::value;

  static constexpr size_t kAppDataSize = EEPROM_APPDATA_BINARY_SIZE;
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
static const uint32_t SELECT_APP_TIMEOUT = 25000;
static const int DEFAULT_APP_INDEX = 1;


void save_global_settings() {
  serial_printf("Saving global settings...\n");

  memcpy(global_settings.user_scales, OC::user_scales, sizeof(OC::user_scales));

  global_settings_storage.Save(global_settings);
  serial_printf("Saved global settings in page_index %d\n", global_settings_storage.page_index());
}

void save_app_data() {
  serial_printf("Saving app data... (%u bytes available)\n", OC::AppData::kAppDataSize);

  app_settings.used = 0;
  char *data = app_settings.data;
  char *data_end = data + OC::AppData::kAppDataSize;
  for (const auto &app : available_apps) {
    size_t storage_size = app.storageSize() + sizeof(OC::AppChunkHeader);
    if (app.Save) {
      if (data + storage_size > data_end) {
        serial_printf("*********************\n");
        serial_printf("%s: CANNOT BE SAVED, NOT ENOUGH SPACE FOR %u BYTES, %u AVAILABLE\n", app.name, data_end - data, OC::AppData::kAppDataSize);
        serial_printf("*********************\n");
        continue;
      }

      OC::AppChunkHeader *header = reinterpret_cast<OC::AppChunkHeader *>(data);
      header->id = app.id;
      header->length = storage_size;
      size_t used = app.Save(header + 1);
      serial_printf("* %s (%x) : Saved %u bytes... (%u)\n", app.name, app.id, used, storage_size);

      app_settings.used += header->length;
      data += header->length;
    }
  }
  serial_printf("App settings used: %u\n", app_settings.used);
  app_data_storage.Save(app_settings);
  serial_printf("Saved app settings in page_index %d\n", app_data_storage.page_index());
}

void restore_app_data() {
  serial_printf("Restoring app data from page_index %d, used=%u\n", app_data_storage.page_index(), app_settings.used);

  const char *data = app_settings.data;
  const char *data_end = data + app_settings.used;
  size_t restored_bytes = 0;

  while (data < data_end) {
    const OC::AppChunkHeader *header = reinterpret_cast<const OC::AppChunkHeader *>(data);
    if (data + header->length > data_end) {
      serial_printf("Uh oh, app chunk header length %u exceeds available data left (%u)\n", header->length, data_end - data);
      break;
    }

    OC::App *app = OC::APPS::find(header->id);
    if (!app) {
      serial_printf("App %x not found, ignoring chunk...\n", app->id);
      if (!header->length)
        break;
      data += header->length;
      continue;
    }
    const size_t len = header->length - sizeof(OC::AppChunkHeader);
    if (len != app->storageSize()) {
      serial_printf("* %s (%x): header size %u != storage size %u, skipping...\n", app->name, header->id, len, app->storageSize());
      data += header->length;
      continue;
    }

    size_t used = 0;
    if (app->Restore) {
      used = app->Restore(header + 1);
      serial_printf("* %s (%x): Restored %u from %u bytes...\n", app->name, header->id, used, header->length);
    }
    restored_bytes += header->length;
    data += header->length;
  }

  serial_printf("App data restored: %u, expected %u\n", restored_bytes, app_settings.used);
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

OC::App *OC::APPS::find(uint16_t id) {
  for (auto &app : available_apps)
    if (app.id == id) return &app;

  return nullptr;
}

void OC::APPS::Init() {

  OC::Scales::Init();
  for (auto &app : available_apps)
    app.Init();

  if (!digitalRead(but_top) && !digitalRead(but_bot)) {
    serial_printf("Skipping loading of global/app settings\n");
    global_settings_storage.Init();
    app_data_storage.Init();
  } else {
    serial_printf("Loading global settings: struct size is %u, PAGESIZE=%u, PAGES=%u\n",
                  sizeof(OC::GlobalSettings),
                  OC::GlobalSettingsStorage::PAGESIZE,
                  OC::GlobalSettingsStorage::PAGES);

    if (!global_settings_storage.Load(global_settings) || global_settings.current_app_index >= APP_COUNT) {
      serial_printf("Settings not loaded or invalid, using defaults... (index=%u, APP_COUNT=%u)\n",
                    global_settings.current_app_index, APP_COUNT);
      global_settings.current_app_index = DEFAULT_APP_INDEX;
    } else {
      serial_printf("Loaded settings from page_index %d, current_app_index is %d\n",
                    global_settings_storage.page_index(),global_settings.current_app_index);
      memcpy(OC::user_scales, global_settings.user_scales, sizeof(OC::user_scales));
    }

    serial_printf("Loading app data: struct size is %u, PAGESIZE=%u, PAGES=%u\n",
                  sizeof(OC::AppData),
                  OC::AppDataStorage::PAGESIZE,
                  OC::AppDataStorage::PAGES);

    if (!app_data_storage.Load(app_settings)) {
      Serial.println("App data not loaded, using defaults...");
    } else {
      restore_app_data();
    }
  }

  set_current_app(global_settings.current_app_index);
  OC::current_app->handleEvent(OC::APP_EVENT_RESUME);

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
  OC::current_app->handleEvent(OC::APP_EVENT_SUSPEND);

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

  OC::CORE::app_isr_enabled = false;
  delay(1);

  set_current_app(selected);
  if (save) {
    save_global_settings();
    save_app_data();
  }

  // Restore state
  encoder[LEFT].setPos(encoder_values[0]);
  encoder[RIGHT].setPos(encoder_values[1]);

  OC::current_app->handleEvent(OC::APP_EVENT_RESUME);

  LAST_ENCODER_VALUE[LEFT] = encoder[LEFT].pos();
  LAST_ENCODER_VALUE[RIGHT] = encoder[RIGHT].pos();

  SELECT_APP = false;
  MENU_REDRAW = 1;
  _UI_TIMESTAMP = millis();

  OC::CORE::app_isr_enabled = true;
}
