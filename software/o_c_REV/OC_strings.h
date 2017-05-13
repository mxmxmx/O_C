#ifndef OC_STRINGS_H_
#define OC_STRINGS_H_

#include <stdint.h>

namespace OC {

  static const int kNumDelayTimes = 8;

  namespace Strings {
    extern const char * const seq_playmodes[];
    extern const char * const channel_trigger_sources[];
    extern const char * const seq_directions[];
    extern const char * const scale_id[];
    extern const char * const channel_id[];
    extern const char * const note_names[];
    extern const char * const note_names_unpadded[];
    extern const char * const trigger_input_names[];
    extern const char * const trigger_input_names_none[];
    extern const char * const cv_input_names[];
    extern const char * const cv_input_names_none[];
    extern const char * const no_yes[];
    extern const char * const off_on[];
    #ifdef PRINT_DEBUG
    extern const char * const encoder_config_strings[];
    #endif
    extern const char * const bytebeat_equation_names[];
    extern const char * const integer_sequence_names[];
    extern const char * const integer_sequence_dirs[];
    extern const char * const trigger_delay_times[kNumDelayTimes];
    extern const char* const chord_property_names[];
    // Not strings but are constant integer sequences
    extern const uint8_t pi_digits[256];
    // extern const uint8_t phi_digits[256];
    // extern const uint8_t tau_digits[256];
    // extern const uint8_t eul_digits[256];
    // extern const uint8_t rt2_digits[256];
    extern const uint8_t van_eck[256];
    extern const uint8_t sum_of_squares_of_digits_of_n[256];
    extern const uint8_t digsum_of_n[256];
    extern const uint8_t digsum_of_n_base4[256];
    extern const uint8_t digsum_of_n_base5[256];
    extern const uint8_t count_down_by_2[256];
    extern const uint8_t interspersion_of_A163253[256];
 };

  // Not a string, but needs to be closer to trigger_delay_times
  extern const uint8_t trigger_delay_ticks[];

};

inline
const char *note_name(int note) {
  return OC::Strings::note_names[(note + 120) % 12];
}

#endif // OC_STRINGS_H_
