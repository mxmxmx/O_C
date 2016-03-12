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

static constexpr int NUM_AVAILABLE_APPS = ARRAY_SIZE(available_apps);

namespace OC {

struct GlobalSettings {
  static constexpr uint32_t FOURCC = FOURCC<'O','C','S',2>::value;

  uint16_t current_app_id;
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

static constexpr int DEFAULT_APP_INDEX = 0;
static const uint16_t DEFAULT_APP_ID = available_apps[DEFAULT_APP_INDEX].id;
OC::App *OC::current_app = &available_apps[DEFAULT_APP_INDEX];

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

  size_t start_app = random(NUM_AVAILABLE_APPS);
  for (size_t i = 0; i < NUM_AVAILABLE_APPS; ++i) {
    const auto &app = available_apps[(start_app + i) % NUM_AVAILABLE_APPS];
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

  const weegfx::coord_t xstart = 0;
  weegfx::coord_t y = (64 - (5 * kUiLineH)) / 2;
  for (int i = 0, current = first; i < 5 && current < NUM_AVAILABLE_APPS; ++i, ++current, y += kUiLineH) {
    UI_SETUP_ITEM(current == selected);
    graphics.print(' ');
    graphics.print(available_apps[current].name);
    if (global_settings.current_app_id == available_apps[current].id)
       graphics.drawBitmap8(2, y + 1, 4, OC::bitmap_indicator_4x8);
    UI_END_ITEM();
  }

  GRAPHICS_END_FRAME();
}

void set_current_app(int index) {
  OC::current_app = &available_apps[index];
  global_settings.current_app_id = OC::current_app->id;
}

OC::App *OC::APPS::find(uint16_t id) {
  for (auto &app : available_apps)
    if (app.id == id) return &app;
  return nullptr;
}

int OC::APPS::index_of(uint16_t id) {
  int i = 0;
  for (const auto &app : available_apps) {
    if (app.id == id) return i;
    ++i;
  }
  return i;
}

void OC::APPS::Init(bool use_defaults) {

  OC::Scales::Init();
  for (auto &app : available_apps)
    app.Init();

  global_settings.current_app_id = DEFAULT_APP_ID;

  int current_app_index = DEFAULT_APP_INDEX;

  if (use_defaults) {
    serial_printf("Skipping loading of global/app settings\n");
    global_settings_storage.Init();
    app_data_storage.Init();
  } else {
    serial_printf("Loading global settings: struct size is %u, PAGESIZE=%u, PAGES=%u\n",
                  sizeof(OC::GlobalSettings),
                  OC::GlobalSettingsStorage::PAGESIZE,
                  OC::GlobalSettingsStorage::PAGES);

    if (!global_settings_storage.Load(global_settings)) {
      serial_printf("Settings not loaded or invalid, using defaults...");
    } else {
      serial_printf("Loaded settings from page_index %d, current_app_id is %x\n",
                    global_settings_storage.page_index(),global_settings.current_app_id);
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

  int index = APPS::index_of(global_settings.current_app_id);
  if (index < 0 || index >= NUM_AVAILABLE_APPS) {
    serial_printf("App id %x not found, using default...", global_settings.current_app_id);
    global_settings.current_app_id = DEFAULT_APP_INDEX;
    index = DEFAULT_APP_INDEX;
  }

  set_current_app(current_app_index);
  OC::current_app->handleEvent(OC::APP_EVENT_RESUME);

  LAST_ENCODER_VALUE[LEFT] = encoder[LEFT].pos();
  LAST_ENCODER_VALUE[RIGHT] = encoder[RIGHT].pos();
}

void OC::APPS::Select() {

  // Save state
  int encoder_values[2] = { encoder[LEFT].pos(), encoder[RIGHT].pos() };
  OC::current_app->handleEvent(OC::APP_EVENT_SUSPEND);

  int selected = APPS::index_of(global_settings.current_app_id);
  encoder[RIGHT].setPos(selected);

  draw_app_menu(selected);
  while (!digitalRead(butR));

  uint32_t time = millis();
  bool redraw = true;
  bool save = false;
  while (!(millis() - time > APP_SELECTION_TIMEOUT_MS)) {
    if (_ENC) {
      _ENC = false;
      int value = encoder[RIGHT].pos();
      if (value < 0) value = 0;
      else if (value >= NUM_AVAILABLE_APPS) value = NUM_AVAILABLE_APPS - 1;
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

  MENU_REDRAW = 1;
  _UI_TIMESTAMP = millis();

  OC::CORE::app_isr_enabled = true;
}
