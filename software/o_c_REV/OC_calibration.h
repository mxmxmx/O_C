#ifndef OC_CALIBRATION_H_
#define OC_CALIBRATION_H_

#include "OC_ADC.h"
#include "OC_config.h"
#include "OC_DAC.h"
#include "util/util_pagestorage.h"
#include "util/EEPROMStorage.h"

//#define VERBOSE_LUT
#ifdef VERBOSE_LUT
#define LUT_PRINTF(fmt, ...) serial_printf(fmt, ##__VA_ARGS__)
#else
#define LUT_PRINTF(x, ...) do {} while (0)
#endif

//#define CALIBRATION_LOAD_LEGACY

namespace OC {

// Originally, this was a single bit that would reverse both encoders.
// With board revisisions >= 2c however, the pins of the right encoder are
// swapped, so additional configurations were added, but existing data
// calibration might have be updated.
enum EncoderConfig : uint32_t {
  ENCODER_CONFIG_NORMAL,
  ENCODER_CONFIG_R_REVERSED,
  ENCODER_CONFIG_L_REVERSED,
  ENCODER_CONFIG_LR_REVERSED,
  ENCODER_CONFIG_LAST
};

enum CalibrationFlags : uint32_t {
  CALIBRATION_FLAG_ENCODER_MASK = 0x3
};

struct CalibrationData {
  static constexpr uint32_t FOURCC = FOURCC<'C', 'A', 'L', 1>::value;

  DAC::CalibrationData dac;
  ADC::CalibrationData adc;

  uint8_t display_offset;
  uint32_t flags;
  uint8_t screensaver_timeout; // 0: default, else seconds
  uint8_t reserved0[3];
  uint32_t reserved1;

  EncoderConfig encoder_config() const {
  	return static_cast<EncoderConfig>(flags & CALIBRATION_FLAG_ENCODER_MASK);
  }

  EncoderConfig next_encoder_config() {
    uint32_t raw_config = ((flags & CALIBRATION_FLAG_ENCODER_MASK) + 1) % ENCODER_CONFIG_LAST;
    flags = (flags & ~CALIBRATION_FLAG_ENCODER_MASK) | raw_config;
    return static_cast<EncoderConfig>(raw_config);
  }
};

static_assert(sizeof(DAC::CalibrationData) == 88, "DAC::CalibrationData size changed!");
static_assert(sizeof(ADC::CalibrationData) == 12, "ADC::CalibrationData size changed!");
static_assert(sizeof(CalibrationData) == 116, "Calibration data size changed!");

typedef PageStorage<EEPROMStorage, EEPROM_CALIBRATIONDATA_START, EEPROM_CALIBRATIONDATA_END, CalibrationData> CalibrationStorage;

extern CalibrationData calibration_data;

}; // namespace OC

// Forward declarations for screwy build system
struct CalibrationStep;
struct CalibrationState;

#endif // OC_CALIBRATION_H_
