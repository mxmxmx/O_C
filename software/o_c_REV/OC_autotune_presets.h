#ifndef OC_AUTOTUNE_PRESETS_H_
#define OC_AUTOTUNE_PRESETS_H_

#include <Arduino.h>
#include "OC_config.h"

namespace OC {

  struct Autotune_data {
 
    uint8_t use_auto_calibration_;
    uint16_t auto_calibrated_octaves[OCTAVES + 1];
  };

  const Autotune_data autotune_data_default[] = {
    // default
    { 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }
  };
}
#endif // OC_AUTOTUNE_PRESETS_H_