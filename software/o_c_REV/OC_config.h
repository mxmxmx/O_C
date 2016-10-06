#ifndef OC_CONFIG_H_
#define OC_CONFIG_H_

#if F_CPU != 120000000
#error "Please compile O&C firmware with CPU speed 120MHz"
#endif

// 60us = 16.666...kHz : Works, SPI transfer ends 2uS before next ISR
// 66us = 15.1515...kHz
// 72us = 13.888...kHz
// 100us = 10Khz
static constexpr uint32_t OC_CORE_ISR_FREQ = 16666U;
static constexpr uint32_t OC_CORE_TIMER_RATE = (1000000UL / OC_CORE_ISR_FREQ);
static constexpr uint32_t OC_UI_TIMER_RATE   = 1000UL;

// From kinetis.h
// Cortex-M4: 0,16,32,48,64,80,96,112,128,144,160,176,192,208,224,240
static constexpr int OC_CORE_TIMER_PRIO = 112; // higher
static constexpr int OC_GPIO_ISR_PRIO   = 128; // default
static constexpr int OC_UI_TIMER_PRIO   = 144; // lower

static constexpr unsigned long REDRAW_TIMEOUT_MS = 1;
static constexpr uint32_t SCREENSAVER_TIMEOUT_S = 25; // default time out menu (in s)
static constexpr uint32_t SCREENSAVER_TIMEOUT_MAX_S = 120;

namespace OC {
static constexpr size_t kMaxTriggerDelayTicks = 96;
};

#define OCTAVES 10      // # octaves
#define SEMITONES (OCTAVES * 12)

static constexpr unsigned long SPLASHSCREEN_DELAY_MS = 1000;
static constexpr unsigned long SPLASHSCREEN_TIMEOUT_MS = 2048;

static constexpr unsigned long APP_SELECTION_TIMEOUT_MS = 25000;

#define EEPROM_CALIBRATIONDATA_START 0
#define EEPROM_CALIBRATIONDATA_END 128

#define EEPROM_GLOBALSETTINGS_START EEPROM_CALIBRATIONDATA_END
#define EEPROM_GLOBALSETTINGS_END 512

#define EEPROM_APPDATA_START EEPROM_GLOBALSETTINGS_END
#define EEPROM_APPDATA_END EEPROMStorage::LENGTH

// This is the available space for all apps' settings (\sa OC_apps.ino)
#define EEPROM_APPDATA_BINARY_SIZE (500 - 4)

#define OC_UI_DEBUG
#define OC_UI_SEPARATE_ISR

#define OC_ENCODERS_ENABLE_ACCELERATION_DEFAULT true

#define OC_CALIBRATION_DEFAULT_FLAGS (0)

//#define QQ_DEBUG_SCREENSAVER
//#define ENVGEN_DEBUG_SCREENSAVER

#endif // OC_CONFIG_H_
