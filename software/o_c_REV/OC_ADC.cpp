#include "OC_ADC.h"
#include <algorithm>

/*static*/ ADC OC::ADC::adc_;
/*static*/ size_t OC::ADC::scan_channel_;
/*static*/ OC::ADC::CalibrationData *OC::ADC::calibration_data_;
/*static*/ int32_t OC::ADC::values_[ADC_CHANNEL_LAST];
/*static*/ uint32_t OC::ADC::raw_values_[ADC_CHANNEL_LAST];
#ifdef ENABLE_ADC_DEBUG
/*static*/ volatile uint32_t OC::ADC::busy_waits_;
#endif

/*static*/ void OC::ADC::Init(CalibrationData *calibration_data) {

  pinMode(ChannelDesc<ADC_CHANNEL_1>::PIN, INPUT);
  pinMode(ChannelDesc<ADC_CHANNEL_2>::PIN, INPUT);
  pinMode(ChannelDesc<ADC_CHANNEL_3>::PIN, INPUT);
  pinMode(ChannelDesc<ADC_CHANNEL_4>::PIN, INPUT);

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
  std::fill(values_, values_ + ADC_CHANNEL_LAST, 0);
  std::fill(raw_values_, raw_values_ + ADC_CHANNEL_LAST, 0);
#ifdef ENABLE_ADC_DEBUG
  busy_waits_ = 0;
#endif
}

// As I understand it, only CV4 can be muxed to ADC1, so it's not possible to
// use ADC::startSynchronizedSingleRead, which would allow reading two channels
// simultaneously

/*static*/ void FASTRUN OC::ADC::Scan() {

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
