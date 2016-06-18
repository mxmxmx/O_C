#include "OC_ADC.h"
#include "OC_gpio.h"

#include <algorithm>

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

/*static*/ ::ADC ADC::adc_;
/*static*/ size_t ADC::scan_channel_;
/*static*/ ADC::CalibrationData *ADC::calibration_data_;
/*static*/ uint32_t ADC::raw_[ADC_CHANNEL_LAST];
/*static*/ uint32_t ADC::smoothed_[ADC_CHANNEL_LAST];
#ifdef ENABLE_ADC_DEBUG
/*static*/ volatile uint32_t ADC::busy_waits_;
#endif

/*static*/ void ADC::Init(CalibrationData *calibration_data) {

  // According to Paul Stoffregen: You do NOT want to have the pin in digital mode when using it as analog input.
  // https://forum.pjrc.com/threads/34319-Analog-input-impedance-and-pull-up?p=103543&viewfull=1#post103543
  //pinMode(ChannelDesc<ADC_CHANNEL_1>::PIN, INPUT);
  //pinMode(ChannelDesc<ADC_CHANNEL_2>::PIN, INPUT);
  //pinMode(ChannelDesc<ADC_CHANNEL_3>::PIN, INPUT);
  //pinMode(ChannelDesc<ADC_CHANNEL_4>::PIN, INPUT);

  adc_.setReference(ADC_REF_3V3);
  adc_.setResolution(kAdcScanResolution);
  adc_.setConversionSpeed(kAdcConversionSpeed);
  adc_.setSamplingSpeed(kAdcSamplingSpeed);
  adc_.setAveraging(kAdcScanAverages);
  adc_.disableDMA();
  adc_.disableInterrupts();
  adc_.disableCompare();

  scan_channel_ = ADC_CHANNEL_1;
  adc_.startSingleRead(ChannelDesc<ADC_CHANNEL_1>::PIN);

  calibration_data_ = calibration_data;
  std::fill(raw_, raw_ + ADC_CHANNEL_LAST, 0);
  std::fill(smoothed_, smoothed_ + ADC_CHANNEL_LAST, 0);
#ifdef ENABLE_ADC_DEBUG
  busy_waits_ = 0;
#endif
}

// As I understand it, only CV4 can be muxed to ADC1, so it's not possible to
// use ADC::startSynchronizedSingleRead, which would allow reading two channels
// simultaneously

/*static*/ void FASTRUN ADC::Scan() {

#ifdef ENABLE_ADC_DEBUG
  if (!adc_.isComplete(ADC_0)) {
    ++busy_waits_;
    while (!adc_.isComplete(ADC_0));
  }
#endif
  const uint16_t value = adc_.readSingle(ADC_0);

  size_t channel = scan_channel_;
  switch (channel) {
    case ADC_CHANNEL_1:
      adc_.startSingleRead(ChannelDesc<ADC_CHANNEL_2>::PIN, ADC_0);
      update<ADC_CHANNEL_1>(value);
      ++channel; 
      break;

    case ADC_CHANNEL_2:
      adc_.startSingleRead(ChannelDesc<ADC_CHANNEL_3>::PIN, ADC_0);
      update<ADC_CHANNEL_2>(value);
      ++channel; 
      break;

    case ADC_CHANNEL_3:
      adc_.startSingleRead(ChannelDesc<ADC_CHANNEL_4>::PIN, ADC_0);
      update<ADC_CHANNEL_3>(value);
      ++channel; 
      break;

    case ADC_CHANNEL_4:
      adc_.startSingleRead(ChannelDesc<ADC_CHANNEL_1>::PIN, ADC_0);
      update<ADC_CHANNEL_4>(value);
      channel = ADC_CHANNEL_1;
      break;
  }
  scan_channel_ = channel;
}

/*static*/ void ADC::CalibratePitch(int32_t c2, int32_t c4) {
  // This is the method used by the Mutable Instruments calibration and
  // extrapolates from two octaves. I guess an alternative would be to get the
  // lowest (-3v) and highest (+6v) and interpolate between them
  // *vague handwaving*
  if (c2 < c4) {
    int32_t scale = (24 * 128 * 4096L) / (c4 - c2);
    calibration_data_->pitch_cv_scale = scale;
  }
}

}; // namespace OC
