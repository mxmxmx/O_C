#include "ADC.h"
#include <Arduino.h>
#include <algorithm>

/*static*/ size_t ADC::scan_channel_;
/*static*/ int32_t ADC::values_[ADC_CHANNEL_LAST];
/*static*/ ADC::CalibrationData *ADC::calibration_data_;

/*static*/ void ADC::Init(CalibrationData *calibration_data) {
  calibration_data_ = calibration_data;
  scan_channel_ = ADC_CHANNEL_2;

  std::fill(values_, values_ + ADC_CHANNEL_LAST, 0);
}

/*static*/ void ADC::Scan() {
  Read<ADC_CHANNEL_1>();

  size_t channel = scan_channel_;
  switch(channel) {
    case ADC_CHANNEL_2: { ADC::Read<ADC_CHANNEL_2>(); ++channel; } break;
    case ADC_CHANNEL_3: { ADC::Read<ADC_CHANNEL_3>(); ++channel; } break;
    case ADC_CHANNEL_4: { ADC::Read<ADC_CHANNEL_4>(); channel = ADC_CHANNEL_2; } break;
  }
  scan_channel_ = channel;
}
