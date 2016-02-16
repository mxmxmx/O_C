/*
*
* DAC8565
*
* chip select/CS : 10
* reset/RST : 9
*
* DB23 = 0 :: always 0
* DB22 = 0 :: always 0
* DB21 = 0 && DB20 = 1 :: single-channel update
* DB19 = 0 :: always 0
* DB18/17  :: DAC # select
* DB16 = 0 :: power down mode
* DB15-DB0 :: data
*
* jacks map to channels A-D as follows:
* 
* top left (B) - > top right (A) - > bottom left (D) - > bottom right (C) 
*
*/

#include <spififo.h>

#define SPICLOCK_30MHz   (SPI_CTAR_PBR(0) | SPI_CTAR_BR(0) | SPI_CTAR_DBR) //(60 / 2) * ((1+1)/2) = 30 MHz (= 24MHz, when F_BUS == 48000000)

/*static*/
void DAC::Init(CalibrationData *calibration_data) {

  calibration_data_ = calibration_data;

  // set up DAC pins 
  pinMode(DAC_CS, OUTPUT);
  pinMode(DAC_RST,OUTPUT);
  // pull RST high 
  digitalWrite(DAC_RST, HIGH); 

  history_tail_ = 0;
  memset(history_, 0, sizeof(uint16_t) * kHistoryDepth * DAC_CHANNEL_LAST);

  if (F_BUS == 60000000 || F_BUS == 48000000) SPIFIFO.begin(DAC_CS, SPICLOCK_30MHz, SPI_MODE0);  

  set_all(0xffff);
  Update();
}

/*static*/
DAC::CalibrationData *DAC::calibration_data_ = nullptr;
/*static*/
uint32_t DAC::values_[DAC_CHANNEL_LAST];
/*static*/
uint16_t DAC::history_[DAC_CHANNEL_LAST][DAC::kHistoryDepth];
/*static*/ 
volatile size_t DAC::history_tail_;

void set8565_CHA(uint32_t data) {
  uint32_t _data = DAC::MAX_VALUE - data;

  SPIFIFO.write(B00010000, SPI_CONTINUE);
  SPIFIFO.write16(_data);
  SPIFIFO.read();
  SPIFIFO.read();
}

void set8565_CHB(uint32_t data) {
  uint32_t _data = DAC::MAX_VALUE - data;

  SPIFIFO.write(B00010010, SPI_CONTINUE);
  SPIFIFO.write16(_data);
  SPIFIFO.read();
  SPIFIFO.read();
}

void set8565_CHC(uint32_t data) {
  uint32_t _data = DAC::MAX_VALUE - data;

  SPIFIFO.write(B00010100, SPI_CONTINUE);
  SPIFIFO.write16(_data);
  SPIFIFO.read();
  SPIFIFO.read(); 
}

void set8565_CHD(uint32_t data) {
  uint32_t _data = DAC::MAX_VALUE - data;

  SPIFIFO.write(B00010110, SPI_CONTINUE);
  SPIFIFO.write16(_data);
  SPIFIFO.read();
  SPIFIFO.read();
}
