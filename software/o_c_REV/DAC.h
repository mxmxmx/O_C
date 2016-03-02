#ifndef DAC_H_
#define DAC_H_

#include "util/util_math.h"

enum DAC_CHANNEL {
  DAC_CHANNEL_A, DAC_CHANNEL_B, DAC_CHANNEL_C, DAC_CHANNEL_D, DAC_CHANNEL_LAST
};

extern void set8565_CHA(uint32_t data);
extern void set8565_CHB(uint32_t data);
extern void set8565_CHC(uint32_t data);
extern void set8565_CHD(uint32_t data);

class DAC {
public:
  static constexpr size_t kHistoryDepth = 8;
  static constexpr uint16_t MAX_VALUE = 65535; // DAC fullscale 

  struct CalibrationData {
    uint16_t octaves[OCTAVES + 1];
    int8_t fine[DAC_CHANNEL_LAST];
  };

  static void Init(CalibrationData *calibration_data);

  static void set_all(uint32_t value) {
    for (int i = DAC_CHANNEL_A; i < DAC_CHANNEL_LAST; ++i)
      values_[i] = value;
  }

  template <DAC_CHANNEL channel>
  static void set(uint32_t value) {
    values_[channel] = value;
  }

  static void set(DAC_CHANNEL channel, uint32_t value) {
    values_[channel] = value;
  }

  template <DAC_CHANNEL channel>
  static uint32_t get() {
    return values_[channel];
  }

  static uint32_t value(size_t index) {
    return values_[index];
  }

  static int32_t pitch_to_dac(int32_t pitch, int32_t octave_offset) {
    pitch += octave_offset * 12 << 7;
    CONSTRAIN(pitch, 0, (120 << 7));

    const int32_t octave = pitch / (12 << 7);
    const int32_t fractional = pitch - octave * (12 << 7);
    int32_t sample = calibration_data_->octaves[octave];
    if (fractional)
      sample += (fractional * (calibration_data_->octaves[octave + 1] - calibration_data_->octaves[octave])) / (12 << 7);

    return sample;
  }

  static void Update() {
    int32_t value;

    value = static_cast<int32_t>(values_[DAC_CHANNEL_A]) + calibration_data_->fine[DAC_CHANNEL_A];
    set8565_CHA(USAT16(value));

    value = static_cast<int32_t>(values_[DAC_CHANNEL_B]) + calibration_data_->fine[DAC_CHANNEL_B];
    set8565_CHB(USAT16(value));

    value = static_cast<int32_t>(values_[DAC_CHANNEL_C]) + calibration_data_->fine[DAC_CHANNEL_C];
    set8565_CHC(USAT16(value));

    value = static_cast<int32_t>(values_[DAC_CHANNEL_D]) + calibration_data_->fine[DAC_CHANNEL_D];
    set8565_CHD(USAT16(value));

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

#endif // DAC_H_
