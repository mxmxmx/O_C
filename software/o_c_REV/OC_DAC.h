#ifndef OC_DAC_H_
#define OC_DAC_H_

#include <stdint.h>
#include <string.h>
#include "OC_config.h"
#include "OC_options.h"
#include "util/util_math.h"
#include "util/util_macros.h"

extern void set8565_CHA(uint32_t data);
extern void set8565_CHB(uint32_t data);
extern void set8565_CHC(uint32_t data);
extern void set8565_CHD(uint32_t data);
extern void SPI_init();

enum DAC_CHANNEL {
  DAC_CHANNEL_A, DAC_CHANNEL_B, DAC_CHANNEL_C, DAC_CHANNEL_D, DAC_CHANNEL_LAST
};

enum OutputVoltageScaling {
  VOLTAGE_SCALING_1V_PER_OCT,    // 0
  VOLTAGE_SCALING_CARLOS_ALPHA,  // 1
  VOLTAGE_SCALING_CARLOS_BETA,   // 2
  VOLTAGE_SCALING_CARLOS_GAMMA,  // 3
  VOLTAGE_SCALING_BOHLEN_PIERCE, // 4
  VOLTAGE_SCALING_QUARTERTONE,   // 5
  #ifdef BUCHLA_SUPPORT
    VOLTAGE_SCALING_1_2V_PER_OCT,  // 6
    VOLTAGE_SCALING_2V_PER_OCT,    // 7
  #endif
  VOLTAGE_SCALING_LAST  
} ;

namespace OC {

class DAC {
public:
  static constexpr size_t kHistoryDepth = 8;
  static constexpr uint16_t MAX_VALUE = 65535; // DAC fullscale 

  #ifdef BUCHLA_4U
    static constexpr int kOctaveZero = 0;
  #else
    static constexpr int kOctaveZero = 3;
  #endif

  struct CalibrationData {
    uint16_t calibrated_octaves[DAC_CHANNEL_LAST][OCTAVES + 1];
  };

  static void Init(CalibrationData *calibration_data);

  static uint8_t calibration_data_used(uint8_t channel_id);
  static void set_auto_channel_calibration_data(uint8_t channel_id);
  static void set_default_channel_calibration_data(uint8_t channel_id);
  static void update_auto_channel_calibration_data(uint8_t channel_id, int8_t octave, uint32_t pitch_data);
  static void reset_auto_channel_calibration_data(uint8_t channel_id);
  static void reset_all_auto_channel_calibration_data();
  static void choose_calibration_data();
  static void set_scaling(uint8_t scaling, uint8_t channel_id);
  static void restore_scaling(uint32_t scaling);
  static uint8_t get_voltage_scaling(uint8_t channel_id);
  static uint32_t store_scaling();
  
  static void set_all(uint32_t value) {
    for (int i = DAC_CHANNEL_A; i < DAC_CHANNEL_LAST; ++i)
      values_[i] = USAT16(value);
  }

  template <DAC_CHANNEL channel>
  static void set(uint32_t value) {
    values_[channel] = USAT16(value);
  }

  static void set(DAC_CHANNEL channel, uint32_t value) {
    values_[channel] = USAT16(value);
  }

  static uint32_t value(size_t index) {
    return values_[index];
  }

  // Calculate DAC value from semitone, where 0 = C1 = 0V, C2 = 12 = 1V
  // Expected semitone resolution is 12 bit.
  //
  // @return DAC output value
  static int32_t semitone_to_dac(DAC_CHANNEL channel, int32_t semi, int32_t octave_offset) {
    return pitch_to_dac(channel, semi << 7, octave_offset);
  }


  // Calculate DAC value from pitch value with 12 * 128 bit per octave.
  // 0 = C1 = 0V, C2 = 24 << 7 = 1V etc. Automatically shifts for LUT range.
  //
  // @return DAC output value
  static int32_t pitch_to_dac(DAC_CHANNEL channel, int32_t pitch, int32_t octave_offset) {
    pitch += (kOctaveZero + octave_offset) * 12 << 7;
    
    CONSTRAIN(pitch, 0, (120 << 7));

    const int32_t octave = pitch / (12 << 7);
    const int32_t fractional = pitch - octave * (12 << 7);

    int32_t sample = calibration_data_->calibrated_octaves[channel][octave];
    if (fractional) {
      int32_t span = calibration_data_->calibrated_octaves[channel][octave + 1] - sample;
      sample += (fractional * span) / (12 << 7);
    }

    return sample;
  }

  // Specialised versions with voltage scaling

  static int32_t semitone_to_scaled_voltage_dac(DAC_CHANNEL channel, int32_t semi, int32_t octave_offset, uint8_t voltage_scaling) {
    return pitch_to_scaled_voltage_dac(channel, semi << 7, octave_offset, voltage_scaling);
  }
  
  static int32_t pitch_to_scaled_voltage_dac(DAC_CHANNEL channel, int32_t pitch, int32_t octave_offset, uint8_t voltage_scaling) {
    pitch += (octave_offset * 12) << 7;

 
    switch (voltage_scaling) {
      case VOLTAGE_SCALING_1V_PER_OCT:    // 1V/oct
          // do nothing
          break;
      case VOLTAGE_SCALING_CARLOS_ALPHA:  // Wendy Carlos alpha scale - scale by 0.77995
          pitch = (pitch * 25548) >> 15;  // 2^15 * 0.77995 = 25547.571
          break;
      case VOLTAGE_SCALING_CARLOS_BETA:   // Wendy Carlos beta scale - scale by 0.63833 
          pitch = (pitch * 20917) >> 15;  // 2^15 * 0.63833 = 20916.776
          break;
      case VOLTAGE_SCALING_CARLOS_GAMMA:  // Wendy Carlos gamma scale - scale by 0.35099
          pitch = (pitch * 11501) >> 15;  // 2^15 * 0.35099 = 11501.2403
          break;
      case VOLTAGE_SCALING_BOHLEN_PIERCE: // Bohlen-Pierce macrotonal scale - scale by 1.585
          pitch = (pitch * 25969) >> 14;  // 2^14 * 1.585 = 25968.64
          break;
      case VOLTAGE_SCALING_QUARTERTONE:   // Quartertone scaling (just down-scales to 0.5V/oct)
          pitch = pitch >> 1;
          break;
      #ifdef BUCHLA_SUPPORT
      case VOLTAGE_SCALING_1_2V_PER_OCT:  // 1.2V/oct
          pitch = (pitch * 19661) >> 14;
          break;
      case VOLTAGE_SCALING_2V_PER_OCT:    // 2V/oct
          pitch = pitch << 1;
          break;
      #endif    
      default: 
          break;
    }

    pitch += (kOctaveZero * 12) << 7;
   
    CONSTRAIN(pitch, 0, (120 << 7));

    const int32_t octave = pitch / (12 << 7);
    const int32_t fractional = pitch - octave * (12 << 7);

    int32_t sample = calibration_data_->calibrated_octaves[channel][octave];
    if (fractional) {
      int32_t span = calibration_data_->calibrated_octaves[channel][octave + 1] - sample;
      sample += (fractional * span) / (12 << 7);
    }

    return sample;
  }
    
  // Set channel to semitone value
  template <DAC_CHANNEL channel>
  static void set_semitone(int32_t semitone, int32_t octave_offset) {
    set<channel>(semitone_to_dac(channel, semitone, octave_offset));
  }

  // Set channel to semitone value
  template <DAC_CHANNEL channel>
  static void set_voltage_scaled_semitone(int32_t semitone, int32_t octave_offset, uint8_t voltage_scaling) {
    set<channel>(semitone_to_scaled_voltage_dac(channel, semitone, octave_offset, voltage_scaling));
  }

  // Set channel to pitch value
  static void set_pitch(DAC_CHANNEL channel, int32_t pitch, int32_t octave_offset) {
    set(channel, pitch_to_dac(channel, pitch, octave_offset));
  }

  // Set integer voltage value, where 0 = 0V, 1 = 1V
  static void set_octave(DAC_CHANNEL channel, int v) {
    set(channel, calibration_data_->calibrated_octaves[channel][kOctaveZero + v]);
  }

  // Set all channels to integer voltage value, where 0 = 0V, 1 = 1V
  static void set_all_octave(int v) {
    set_octave(DAC_CHANNEL_A, v);
    set_octave(DAC_CHANNEL_B, v);
    set_octave(DAC_CHANNEL_C, v);
    set_octave(DAC_CHANNEL_D, v);
  }

  static uint32_t get_zero_offset(DAC_CHANNEL channel) {
    return calibration_data_->calibrated_octaves[channel][kOctaveZero];
  }

  static uint32_t get_octave_offset(DAC_CHANNEL channel, int octave) {
    return calibration_data_->calibrated_octaves[channel][kOctaveZero + octave];
  }

  static void Update() {

    set8565_CHA(values_[DAC_CHANNEL_A]);
    set8565_CHB(values_[DAC_CHANNEL_B]);
    set8565_CHC(values_[DAC_CHANNEL_C]);
    set8565_CHD(values_[DAC_CHANNEL_D]);

    size_t tail = history_tail_;
    history_[DAC_CHANNEL_A][tail] = values_[DAC_CHANNEL_A];
    history_[DAC_CHANNEL_B][tail] = values_[DAC_CHANNEL_B];
    history_[DAC_CHANNEL_C][tail] = values_[DAC_CHANNEL_C];
    history_[DAC_CHANNEL_D][tail] = values_[DAC_CHANNEL_D];
    history_tail_ = (tail + 1) % kHistoryDepth;
  }

  template <DAC_CHANNEL channel>
  static void getHistory(uint16_t *dst){
    size_t head = (history_tail_ + 1) % kHistoryDepth;

    size_t count = kHistoryDepth - head;
    const uint16_t *src = history_[channel] + head;
    while (count--)
      *dst++ = *src++;

    count = head;
    src = history_[channel];
    while (count--)
      *dst++ = *src++;
  }

private:
  static CalibrationData *calibration_data_;
  static uint32_t values_[DAC_CHANNEL_LAST];
  static uint16_t history_[DAC_CHANNEL_LAST][kHistoryDepth];
  static volatile size_t history_tail_;
  static uint8_t DAC_scaling[DAC_CHANNEL_LAST];
};

}; // namespace OC

#endif // OC_DAC_H_
