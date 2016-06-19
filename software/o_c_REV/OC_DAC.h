#ifndef OC_DAC_H_
#define OC_DAC_H_

#include <stdint.h>
#include <string.h>
#include "OC_config.h"
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

namespace OC {

class DAC {
public:
  static constexpr size_t kHistoryDepth = 8;
  static constexpr uint16_t MAX_VALUE = 65535; // DAC fullscale 

  static constexpr int kOctaveZero = 3;

  struct CalibrationData {
    uint16_t calibrated_octaves[DAC_CHANNEL_LAST][OCTAVES + 1];
  };

  static void Init(CalibrationData *calibration_data);

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

  // Set channel to semitone value
  template <DAC_CHANNEL channel>
  static void set_semitone(int32_t semitone, int32_t octave_offset) {
    set<channel>(semitone_to_dac(channel, semitone, octave_offset));
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
};

}; // namespace OC

#endif // OC_DAC_H_
