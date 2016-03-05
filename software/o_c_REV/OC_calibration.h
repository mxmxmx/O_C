#ifndef OC_CALIBRATION_H_
#define OC_CALIBRATION_H_

#include "OC_ADC.h"
#include "OC_config.h"
#include "OC_storage.h"
#include "DAC.h"
#include "util/util_pagestorage.h"
#include "util/EEPROMStorage.h"

//#define VERBOSE_LUT
#ifdef VERBOSE_LUT
#define LUT_PRINTF(fmt, ...) serial_printf(fmt, ##__VA_ARGS__)
#else
#define LUT_PRINTF(x, ...) do {} while (0)
#endif

namespace OC {

struct CalibrationData {
  static constexpr uint32_t FOURCC = FOURCC<'C', 'A', 'L', 1>::value;

  DAC::CalibrationData dac;
  ADC::CalibrationData adc;

  uint32_t flags;
  uint8_t display_offset;
};

typedef PageStorage<EEPROMStorage, EEPROM_CALIBRATIONDATA_START, EEPROM_CALIBRATIONDATA_END, CalibrationData> CalibrationStorage;

extern CalibrationData calibration_data;
};

// Forward declarations for screwy build system
struct CalibrationStep;
struct CalibrationState;

#endif // OC_CALIBRATION_H_
