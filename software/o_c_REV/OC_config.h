#ifndef OC_CONFIG_H_
#define OC_CONFIG_H_

// 60us = 16.666...kHz : Works, SPI transfer ends 2uS before next ISR
// 66us = 15.1515...kHz
// 72us = 13.888...kHz
// 100us = 10Khz
static constexpr uint32_t OC_CORE_ISR_FREQ = 16666U;
static constexpr uint32_t OC_CORE_TIMER_RATE = (1000000UL / OC_CORE_ISR_FREQ);

static constexpr unsigned long REDRAW_TIMEOUT_MS = 1;
static constexpr unsigned long SCREENSAVER_TIMEOUT_MS = 15000; // time out menu (in ms)

#define OCTAVES 10      // # octaves
static constexpr uint16_t _ZERO = 0x3;                                     // "zero" code < > octave 4
#define SEMITONES (OCTAVES * 12)

static constexpr unsigned long SPLASHSCREEN_DELAY_MS = 500;
static constexpr unsigned long SPLASHSCREEN_TIMEOUT_MS = 2048;

static constexpr unsigned long APP_SELECTION_TIMEOUT_MS = 25000;

#endif // OC_CONFIG_H_
