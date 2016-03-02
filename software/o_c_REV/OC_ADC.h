#ifndef OC_ADC_H_
#define OC_ADC_H_

#include <Arduino.h>
#include <ADC.h>

#include <stdint.h>
#include <string.h>
#include "OC_gpio.h"

//#define ENABLE_ADC_DEBUG

enum ADC_CHANNEL {
  ADC_CHANNEL_1,
  ADC_CHANNEL_2,
  ADC_CHANNEL_3,
  ADC_CHANNEL_4,
  ADC_CHANNEL_LAST,
};

namespace OC {

template <ADC_CHANNEL> struct ChannelDesc { };
template <> struct ChannelDesc<ADC_CHANNEL_1> {
  static const int PIN = CV1;
};
template <> struct ChannelDesc<ADC_CHANNEL_2> {
  static const int PIN = CV2;
};
template <> struct ChannelDesc<ADC_CHANNEL_3> {
  static const int PIN = CV3;
};
template <> struct ChannelDesc<ADC_CHANNEL_4> {
  static const int PIN = CV4;
};

class ADC {
public:

  static constexpr uint8_t kAdcResolution = 12;
  static constexpr uint32_t kAdcSmoothing = 4;
  static constexpr uint32_t kAdcSmoothBits = 8; // fractional bits for smoothing

  // These values should be tweaked so startSingleRead/readSingle run in main ISR update time
  // 16 bit has best-case 13 bits useable, but we only want 12 so we discard 4 anyway
  static constexpr uint8_t kAdcScanResolution = 16;
  static constexpr uint8_t kAdcScanAverages = 16;
  static constexpr uint8_t kAdcSamplingSpeed = ADC_HIGH_SPEED_16BITS;
  static constexpr uint8_t kAdcConversionSpeed = ADC_HIGH_SPEED;
  static constexpr uint32_t kAdcValueShift = kAdcSmoothBits + (kAdcScanResolution - kAdcResolution);


  struct CalibrationData {
    uint16_t offset[ADC_CHANNEL_LAST];
    uint16_t pitch_cv_scale;
    uint16_t pitch_cv_offset;
  };

  static void Init(CalibrationData *calibration_data);

  // Read the value of the last conversion and update current channel, then
  // start the next conversion. If necessary, some channels could be given
  // priority by scanning them more often. Even better might be some kind of
  // continuous/DMA sampling to make things even more independent of the main
  // ISR timing restrictions.
  static void Scan();

  template <ADC_CHANNEL channel>
  static int32_t value() {
    return values_[channel];
  }

  static int32_t value(ADC_CHANNEL channel) {
    return values_[channel];
  }

  static uint32_t raw_value(ADC_CHANNEL channel) {
    return raw_values_[channel] >> kAdcValueShift;
  }

  static int32_t pitch_value(ADC_CHANNEL channel) {
    return (values_[channel] * 120 << 7) >> 12;
  }

#ifdef ENABLE_ADC_DEBUG
  // DEBUG
  static uint16_t fail_flag0() {
    return adc_.adc0->fail_flag;
  }

  static uint16_t fail_flag1() {
    return adc_.adc1->fail_flag;
  }

  static uint32_t busy_waits() {
    return busy_waits_;
  }
#endif

private:

  template <ADC_CHANNEL channel>
  static void update(uint32_t value) {
    // division should be shift if kAdcSmoothing is power-of-two
    value = (raw_values_[channel] * (kAdcSmoothing - 1) + (value << kAdcSmoothBits)) / kAdcSmoothing;
    values_[channel] = calibration_data_->offset[channel] - (value >> kAdcValueShift);
    raw_values_[channel] = value;
  }

  static ::ADC adc_;
  static size_t scan_channel_;
  static CalibrationData *calibration_data_;

  static uint32_t raw_values_[ADC_CHANNEL_LAST];
  static int32_t values_[ADC_CHANNEL_LAST];

#ifdef ENABLE_ADC_DEBUG
  static volatile uint32_t busy_waits_;
#endif
};

};

#endif // OC_ADC_H_
