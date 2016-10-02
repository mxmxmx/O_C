#ifndef OC_STRINGS_H_
#define OC_STRINGS_H_

#include <stdint.h>

namespace OC {

  static const int kNumDelayTimes = 8;

  namespace Strings {
    extern const char * const note_names[];
    extern const char * const note_names_unpadded[];
    extern const char * const trigger_input_names[];
    extern const char * const cv_input_names[];
    extern const char * const no_yes[];
    extern const char * const encoder_config_strings[];
    extern const char * const bytebeat_equation_names[];
    extern const char * const integer_sequence_names[];
    extern const char * const integer_sequence_dirs[];
    extern const char * const trigger_delay_times[kNumDelayTimes];
  // Not strings but are constant integer sequences
  extern const uint8_t pi_digits[256];
  extern const uint8_t phi_digits[256];
  extern const uint8_t tau_digits[256];
  extern const uint8_t eul_digits[256];
  extern const uint8_t rt2_digits[256];
  extern const uint8_t rt3_digits[256];
  extern const uint8_t rt5_digits[256];
 };

  // Not a string, but needs to be closer to trigger_delay_times
  extern const uint8_t trigger_delay_ticks[];

};

inline
const char *note_name(int note) {
  return OC::Strings::note_names[(note + 120) % 12];
}

#endif // OC_STRINGS_H_
