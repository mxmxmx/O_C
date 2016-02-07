#ifndef OC_CALIBRATION_H_
#define OC_CALIBRATION_H_

#include "OC_ADC.h"
#include "OC_storage.h"
#include "DAC.h"
#include "util_pagestorage.h"
#include "EEPROMStorage.h"

#define OCTAVES 10      // # octaves
static constexpr uint16_t _ZERO = 0x3;                                     // "zero" code < > octave 4
#define RANGE 119                     // [0-119] = 120 semitones < > 10 octaves 
#define S_RANGE 119<<5                // same thing, spread over 12 bit (ish)

//#define VERBOSE_LUT
#ifdef VERBOSE_LUT
#define VERBOSE_PRINT(x) Serial.print(x)
#define VERBOSE_PRINTLN(x) Serial.println(x)
#else
#define VERBOSE_PRINT(x) do {} while (0)
#define VERBOSE_PRINTLN(x) do {} while (0)
#endif

namespace OC {

struct CalibrationData {
  static constexpr uint32_t FOURCC = FOURCC<'C', 'A', 'L', 1>::value;

  uint16_t octaves[OCTAVES + 1];

  DAC::CalibrationData dac;
  ADC::CalibrationData adc;
};

typedef PageStorage<EEPROMStorage, EEPROM_CALIBRATIONDATA_START, EEPROM_CALIBRATIONDATA_END, CalibrationData> CalibrationStorage;

extern CalibrationData calibration_data;
};

extern uint16_t semitones[RANGE+1];          // DAC output LUT


// Forward declarations for screwy build system
struct CalibrationStep;
struct CalibrationState;

#endif // OC_CALIBRATION_H_
