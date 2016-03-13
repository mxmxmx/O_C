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

// Global settings are stored separately to actual app setings.
// The theory is that they might not change as often.
struct GlobalSettings {
  static constexpr uint32_t FOURCC = FOURCC<'O','C','S',2>::value;

  uint16_t current_app_id;
  OC::Scale user_scales[OC::Scales::SCALE_USER_LAST];
};

// App settings are packed into a single blob of binary data; each app's chunk
// gets its own header with id and the length of the entire chunk. This makes
// this a bit more flexible during development.
// Chunks are aligned on 2-byte boundaries for arbitrary reasons (thankfully M4
// allows unaligned access...)
struct AppChunkHeader {
  uint16_t id;
  uint16_t length;
} __attribute__((packed));

struct AppData {
  static constexpr uint32_t FOURCC = FOURCC<'O','C','A',3>::value;

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
  SERIAL_PRINTLN("Saving global settings...");

  memcpy(global_settings.user_scales, OC::user_scales, sizeof(OC::user_scales));

  global_settings_storage.Save(global_settings);
  SERIAL_PRINTLN("Saved global settings in page_index %d", global_settings_storage.page_index());
}

void save_app_data() {
  SERIAL_PRINTLN("Saving app data... (%u bytes available)", OC::AppData::kAppDataSize);

  app_settings.used = 0;
  char *data = app_settings.data;
  char *data_end = data + OC::AppData::kAppDataSize;

  size_t start_app = random(NUM_AVAILABLE_APPS);
  for (size_t i = 0; i < NUM_AVAILABLE_APPS; ++i) {
    const auto &app = available_apps[(start_app + i) % NUM_AVAILABLE_APPS];
    size_t storage_size = app.storageSize() + sizeof(OC::AppChunkHeader);
    if (storage_size & 1) ++storage_size; // Align chunks on 2-byte boundaries
    if (app.Save) {
      if (data + storage_size > data_end) {
        SERIAL_PRINTLN("*********************");
        SERIAL_PRINTLN("%s: CANNOT BE SAVED, NOT ENOUGH SPACE FOR %u BYTES, %u AVAILABLE", app.name, data_end - data, OC::AppData::kAppDataSize);
        SERIAL_PRINTLN("*********************");
        continue;
      }

      OC::AppChunkHeader *chunk = reinterpret_cast<OC::AppChunkHeader *>(data);
      chunk->id = app.id;
      chunk->length = storage_size;
      size_t used = app.Save(chunk + 1);
      SERIAL_PRINTLN("* %s (%02x) : Saved %u bytes... (%u)", app.name, app.id, used, storage_size);

      app_settings.used += chunk->length;
      data += chunk->length;
    }
  }
  SERIAL_PRINTLN("App settings used: %u/%u", app_settings.used, EEPROM_APPDATA_BINARY_SIZE);
  app_data_storage.Save(app_settings);
  SERIAL_PRINTLN("Saved app settings in page_index %d", app_data_storage.page_index());
}

void restore_app_data() {
  SERIAL_PRINTLN("Restoring app data from page_index %d, used=%u", app_data_storage.page_index(), app_settings.used);

  const char *data = app_settings.data;
  const char *data_end = data + app_settings.used;
  size_t restored_bytes = 0;

  while (data < data_end) {
    const OC::AppChunkHeader *chunk = reinterpret_cast<const OC::AppChunkHeader *>(data);
    if (data + chunk->length > data_end) {
      SERIAL_PRINTLN("Uh oh, app chunk length %u exceeds available data left (%u)", chunk->length, data_end - data);
      break;
    }

    OC::App *app = OC::APPS::find(chunk->id);
    if (!app) {
      SERIAL_PRINTLN("App %02x not found, ignoring chunk...", app->id);
      if (!chunk->length)
        break;
      data += chunk->length;
      continue;
    }
    size_t expected_length = app->storageSize() + sizeof(OC::AppChunkHeader);
    if (expected_length & 0x1) ++expected_length;
    if (chunk->length != expected_length) {
      SERIAL_PRINTLN("* %s (%02x): chunk length %u != %u (storageSize=%u), skipping...", app->name, chunk->id, chunk->length, expected_length, app->storageSize());
      data += chunk->length;
      continue;
    }

    size_t used = 0;
    if (app->Restore) {
      const size_t len = chunk->length - sizeof(OC::AppChunkHeader);
      used = app->Restore(chunk + 1);
      SERIAL_PRINTLN("* %s (%02x): Restored %u from %u (chunk length %u)...", app->name, chunk->id, used, len, chunk->length);
    }
    restored_bytes += chunk->length;
    data += chunk->length;
  }

  SERIAL_PRINTLN("App data restored: %u, expected %u", restored_bytes, app_settings.used);
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

  if (use_defaults) {
    SERIAL_PRINTLN("Skipping loading of global/app settings");
    global_settings_storage.Init();
    app_data_storage.Init();
  } else {
    SERIAL_PRINTLN("Loading global settings: struct size is %u, PAGESIZE=%u, PAGES=%u, LENGTH=%u",
                  sizeof(OC::GlobalSettings),
                  OC::GlobalSettingsStorage::PAGESIZE,
                  OC::GlobalSettingsStorage::PAGES,
                  OC::GlobalSettingsStorage::LENGTH);

    if (!global_settings_storage.Load(global_settings)) {
      SERIAL_PRINTLN("Settings not loaded or invalid, using defaults...");
    } else {
      SERIAL_PRINTLN("Loaded settings from page_index %d, current_app_id is %02x",
                    global_settings_storage.page_index(),global_settings.current_app_id);
      memcpy(OC::user_scales, global_settings.user_scales, sizeof(OC::user_scales));
    }

    SERIAL_PRINTLN("Loading app data: struct size is %u, PAGESIZE=%u, PAGES=%u, LENGTH=%u",
                  sizeof(OC::AppData),
                  OC::AppDataStorage::PAGESIZE,
                  OC::AppDataStorage::PAGES,
                  OC::AppDataStorage::LENGTH);

    if (!app_data_storage.Load(app_settings)) {
      SERIAL_PRINTLN("App data not loaded, using defaults...");
    } else {
      restore_app_data();
    }
  }

  int current_app_index = APPS::index_of(global_settings.current_app_id);
  if (current_app_index < 0 || current_app_index >= NUM_AVAILABLE_APPS) {
    SERIAL_PRINTLN("App id %02x not found, using default...", global_settings.current_app_id);
    global_settings.current_app_id = DEFAULT_APP_INDEX;
    current_app_index = DEFAULT_APP_INDEX;
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
      SERIAL_PRINTLN("DEBUG MENU");
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
