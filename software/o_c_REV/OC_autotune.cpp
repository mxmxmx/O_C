#include "OC_autotune.h"
#include "OC_autotune_presets.h"

namespace OC {

    Autotune_data auto_calibration_data[DAC_CHANNEL_LAST];

    /*static*/
    const int AUTOTUNE::NUM_DAC_CHANNELS = DAC_CHANNEL_LAST;

    /*static*/
    void AUTOTUNE::Init() {
      for (size_t i = 0; i < DAC_CHANNEL_LAST; i++)
        memcpy(&auto_calibration_data[i], &OC::autotune_data_default[0], sizeof(Autotune_data));
    }
    /*static*/
    const Autotune_data &AUTOTUNE::GetAutotune_data(int channel) {
        return auto_calibration_data[channel];
    }
} // namespace OC