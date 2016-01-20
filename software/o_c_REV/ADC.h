#ifndef ADC_H_
#define ADC_H_

#include <Arduino.h>

#include <stdint.h>
#include <string.h>
#include "O_C_gpio.h"

enum ADC_CHANNEL {
  ADC_CHANNEL_1,
  ADC_CHANNEL_2,
  ADC_CHANNEL_3,
  ADC_CHANNEL_4,
  ADC_CHANNEL_LAST,
};

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
  struct CalibrationData {
    uint16_t offset[ADC_CHANNEL_LAST];
  };

  static void Init(CalibrationData *calibration_data);

  static void Scan();

  template <ADC_CHANNEL channel>
  static int32_t ReadImmediate() {
    int32_t value = calibration_data_->offset[channel] - analogRead(ChannelDesc<channel>::PIN);
    values_[channel] = value;
    return value;
  }

  template <ADC_CHANNEL channel>
  static int32_t value() {
    return values_[channel];
  }

  static int32_t value(ADC_CHANNEL channel) {
    return values_[channel];
  }

private:

  template <ADC_CHANNEL channel>
  static void Read() {
    values_[channel] = calibration_data_->offset[channel] - analogRead(ChannelDesc<channel>::PIN);
  }

  static size_t scan_channel_;
  static int32_t values_[ADC_CHANNEL_LAST];
  static CalibrationData *calibration_data_;
};

#endif // ADC_H_
