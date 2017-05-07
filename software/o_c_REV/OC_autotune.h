#ifndef OC_AUTOTUNE_H_
#define OC_AUTOTUNE_H_

#include "OC_autotune_presets.h"
#include "OC_DAC.h"

namespace OC {

typedef OC::Autotune_data Autotune_data;

class AUTOTUNE {
public:
  static const int NUM_DAC_CHANNELS;
  static void Init();
  static const Autotune_data &GetAutotune_data(int channel);
};

extern Autotune_data auto_calibration_data[DAC_CHANNEL_LAST];
};

#endif // OC_AUTOTUNE_H_