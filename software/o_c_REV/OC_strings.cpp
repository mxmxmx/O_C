#include "OC_strings.h"

namespace OC {

  namespace Strings {
  const char * const note_names[12] = { "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B " };
  const char * const note_names_unpadded[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

  const char * const trigger_input_names[4] = { "TR1", "TR2", "TR3", "TR4" };

  const char * const cv_input_names[4] = { "CV1", "CV2", "CV3", "CV4" };

  const char * const no_yes[] = { "No", "Yes" };

  const char * const encoder_config_strings[] = { "normal", "R reversed", "L reversed", "LR reversed" };

  const char * const trigger_delay_times[kNumDelayTimes] = {
      "off", "120us", "240us", "360us", "480us", "1ms", "2ms", "4ms"
  };

  };

  // \sa OC_config.h -> kMaxTriggerDelayTicks
  const uint8_t trigger_delay_ticks[kNumDelayTimes] = {
    0, 2, 4, 6, 8, 16, 33, 66
  };
};
