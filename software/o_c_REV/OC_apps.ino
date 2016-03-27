// Copyright (c) 2016 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "OC_apps.h"

#define DECLARE_APP(a, b, name, prefix, isr) \
{ TWOCC<a,b>::value, name, \
  prefix ## _init, prefix ## _storageSize, prefix ## _save, prefix ## _restore, \
  prefix ## _handleAppEvent, \
  prefix ## _loop, prefix ## _menu, prefix ## _screensaver, \
  prefix ## _handleButtonEvent, \
  prefix ## _handleEncoderEvent, \
  isr \
}

#define ASR_screensaver OC::screensaver
#define QQ_screensaver OC::screensaver
// #define ENVGEN_screensaver screensaver

OC::App available_apps[] = {
  DECLARE_APP('A','S', "CopierMaschine", ASR, ASR_isr),
  DECLARE_APP('H','A', "Harrington 1200", H1200, H1200_isr),
  DECLARE_APP('A','T', "Automatonnetz", Automatonnetz, Automatonnetz_isr),
  DECLARE_APP('Q','Q', "Quantermain", QQ, QQ_isr),
  DECLARE_APP('P','L', "Quadraturia", POLYLFO, POLYLFO_isr),
  DECLARE_APP('L','R', "Low-rents", LORENZ, LORENZ_isr),
  DECLARE_APP('E','G', "Piqued", ENVGEN, ENVGEN_isr),
  DECLARE_APP('B','B', "Dialectic Ping Pong", BBGEN, BBGEN_isr),
  DECLARE_APP('B','Y', "Viznutcracker sweet", BYTEBEATGEN, BYTEBEATGEN_isr),
};

static constexpr int NUM_AVAILABLE_APPS = ARRAY_SIZE(available_apps);

namespace OC {

// Global settings are stored separately to actual app setings.
// The theory is that they might not change as often.
struct GlobalSettings {
  static constexpr uint32_t FOURCC = FOURCC<'O','C','S',2>::value;

  bool encoders_enable_acceleration;
  bool reverse_encoders;
  bool switch_encoders;

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

GlobalSettings global_settings;
GlobalSettingsStorage global_settings_storage;

AppData app_settings;
AppDataStorage app_data_storage;

static constexpr int DEFAULT_APP_INDEX = 0;
static const uint16_t DEFAULT_APP_ID = available_apps[DEFAULT_APP_INDEX].id;

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
    size_t storage_size = app.storageSize() + sizeof(AppChunkHeader);
    if (storage_size & 1) ++storage_size; // Align chunks on 2-byte boundaries
    if (app.Save) {
      if (data + storage_size > data_end) {
        SERIAL_PRINTLN("*********************");
        SERIAL_PRINTLN("%s: CANNOT BE SAVED, NOT ENOUGH SPACE FOR %u BYTES, %u AVAILABLE", app.name, data_end - data, AppData::kAppDataSize);
        SERIAL_PRINTLN("*********************");
        continue;
      }

      AppChunkHeader *chunk = reinterpret_cast<AppChunkHeader *>(data);
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
    const AppChunkHeader *chunk = reinterpret_cast<const AppChunkHeader *>(data);
    if (data + chunk->length > data_end) {
      SERIAL_PRINTLN("Uh oh, app chunk length %u exceeds available data left (%u)", chunk->length, data_end - data);
      break;
    }

    App *app = apps::find(chunk->id);
    if (!app) {
      SERIAL_PRINTLN("App %02x not found, ignoring chunk...", app->id);
      if (!chunk->length)
        break;
      data += chunk->length;
      continue;
    }
    size_t expected_length = app->storageSize() + sizeof(AppChunkHeader);
    if (expected_length & 0x1) ++expected_length;
    if (chunk->length != expected_length) {
      SERIAL_PRINTLN("* %s (%02x): chunk length %u != %u (storageSize=%u), skipping...", app->name, chunk->id, chunk->length, expected_length, app->storageSize());
      data += chunk->length;
      continue;
    }

    size_t used = 0;
    if (app->Restore) {
      const size_t len = chunk->length - sizeof(AppChunkHeader);
      used = app->Restore(chunk + 1);
      SERIAL_PRINTLN("* %s (%02x): Restored %u from %u (chunk length %u)...", app->name, chunk->id, used, len, chunk->length);
    }
    restored_bytes += chunk->length;
    data += chunk->length;
  }

  SERIAL_PRINTLN("App data restored: %u, expected %u", restored_bytes, app_settings.used);
}

namespace apps {

void set_current_app(int index) {
  current_app = &available_apps[index];
  global_settings.current_app_id = current_app->id;
}

App *current_app = &available_apps[DEFAULT_APP_INDEX];

App *find(uint16_t id) {
  for (auto &app : available_apps)
    if (app.id == id) return &app;
  return nullptr;
}

int index_of(uint16_t id) {
  int i = 0;
  for (const auto &app : available_apps) {
    if (app.id == id) return i;
    ++i;
  }
  return i;
}

void Init(bool use_defaults) {

  Scales::Init();
  for (auto &app : available_apps)
    app.Init();

  global_settings.current_app_id = DEFAULT_APP_ID;
  global_settings.encoders_enable_acceleration = OC_ENCODERS_ENABLE_ACCELERATION_DEFAULT;
  global_settings.switch_encoders = false;
  global_settings.reverse_encoders = false;

  if (use_defaults) {
    SERIAL_PRINTLN("Skipping loading of global/app settings");
    global_settings_storage.Init();
    app_data_storage.Init();
  } else {
    SERIAL_PRINTLN("Loading global settings: struct size is %u, PAGESIZE=%u, PAGES=%u, LENGTH=%u",
                  sizeof(GlobalSettings),
                  GlobalSettingsStorage::PAGESIZE,
                  GlobalSettingsStorage::PAGES,
                  GlobalSettingsStorage::LENGTH);

    if (!global_settings_storage.Load(global_settings)) {
      SERIAL_PRINTLN("Settings not loaded or invalid, using defaults...");
    } else {
      SERIAL_PRINTLN("Loaded settings from page_index %d, current_app_id is %02x",
                    global_settings_storage.page_index(),global_settings.current_app_id);
      memcpy(user_scales, global_settings.user_scales, sizeof(user_scales));
    }

    SERIAL_PRINTLN("Loading app data: struct size is %u, PAGESIZE=%u, PAGES=%u, LENGTH=%u",
                  sizeof(AppData),
                  AppDataStorage::PAGESIZE,
                  AppDataStorage::PAGES,
                  AppDataStorage::LENGTH);

    if (!app_data_storage.Load(app_settings)) {
      SERIAL_PRINTLN("App data not loaded, using defaults...");
    } else {
      restore_app_data();
    }
  }

  int current_app_index = apps::index_of(global_settings.current_app_id);
  if (current_app_index < 0 || current_app_index >= NUM_AVAILABLE_APPS) {
    SERIAL_PRINTLN("App id %02x not found, using default...", global_settings.current_app_id);
    global_settings.current_app_id = DEFAULT_APP_INDEX;
    current_app_index = DEFAULT_APP_INDEX;
  }

  SERIAL_PRINTLN("Encoder acceleration: %s", global_settings.encoders_enable_acceleration ? "enabled" : "disabled");
  ui.encoders_enable_acceleration(global_settings.encoders_enable_acceleration);

  set_current_app(current_app_index);
  current_app->HandleAppEvent(APP_EVENT_RESUME);

  delay(100);
}

}; // namespace apps

void draw_app_menu(const menu::ScreenCursor<5> &cursor) {
  GRAPHICS_BEGIN_FRAME(true);

  menu::SettingsListItem item;
  item.x = menu::kIndentDx;
  item.y = (64 - (5 * menu::kMenuLineH)) / 2;

  for (int current = cursor.first_visible();
       current <= cursor.last_visible();
       ++current, item.y += menu::kMenuLineH) {
    item.selected = current == cursor.cursor_pos();
    item.SetPrintPos();
    graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);
    graphics.print(available_apps[current].name);
    if (global_settings.current_app_id == available_apps[current].id)
       graphics.drawBitmap8(item.x + 2, item.y + 1, 4, bitmap_indicator_4x8);
     item.DrawCustom();
  }

  GRAPHICS_END_FRAME();
}

void Ui::AppSettings() {

  SetButtonIgnoreMask();

  apps::current_app->HandleAppEvent(APP_EVENT_SUSPEND);

  menu::ScreenCursor<5> cursor;
  cursor.Init(0, NUM_AVAILABLE_APPS - 1);
  cursor.Scroll(apps::index_of(global_settings.current_app_id));

  bool change_app = false;
  bool save = false;
  while (!change_app && idle_time() < APP_SELECTION_TIMEOUT_MS) {

    while (event_queue_.available()) {
      UI::Event event = event_queue_.PullEvent();
      if (IgnoreEvent(event))
        continue;

      if (UI::EVENT_ENCODER == event.type && CONTROL_ENCODER_R == event.control) {
        cursor.Scroll(event.value);
      } else if (CONTROL_BUTTON_R == event.control) {
        save = event.type == UI::EVENT_BUTTON_LONG_PRESS;
        change_app = true;
      } else if (CONTROL_BUTTON_L == event.control) {
        ui.DebugStats();
      } else if (CONTROL_BUTTON_UP == event.control) {
        bool enabled = !global_settings.encoders_enable_acceleration;
        SERIAL_PRINTLN("Encoder acceleration: %s", enabled ? "enabled" : "disabled");
        ui.encoders_enable_acceleration(enabled);
        global_settings.encoders_enable_acceleration = enabled;
      }
    }

    draw_app_menu(cursor);
  }

  event_queue_.Flush();
  event_queue_.Poke();

  CORE::app_isr_enabled = false;
  delay(1);

  if (change_app) {
    apps::set_current_app(cursor.cursor_pos());
    if (save) {
      save_global_settings();
      save_app_data();
    }
  }

  OC::ui.encoders_enable_acceleration(global_settings.encoders_enable_acceleration);

  // Restore state
  apps::current_app->HandleAppEvent(APP_EVENT_RESUME);
  CORE::app_isr_enabled = true;
}

}; // namespace OC
