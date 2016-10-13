/*
*   (C) Copyright 2013, Andrew Kroll (xxxajk)
*   (C) Copyright 2015, 2016, Patrick Dowling, Max Stadler
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of version 3 of the GNU General Public License as
*   published by the Free Software Foundation at http://www.gnu.org/licenses,
*   with Additional Permissions under term 7(b) that the original copyright
*   notice and author attibution must be preserved and under term 7(c) that
*   modified versions be marked as different from the original.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*
*   DAC8565 basics.
*
*   chip select/CS : 10
*   reset/RST : 9
*
*   DB23 = 0 :: always 0
*   DB22 = 0 :: always 0
*   DB21 = 0 && DB20 = 1 :: single-channel update
*   DB19 = 0 :: always 0
*   DB18/17  :: DAC # select
*   DB16 = 0 :: power down mode
*   DB15-DB0 :: data
*
*   jacks map to channels A-D as follows:
*   top left (B) - > top right (A) - > bottom left (D) - > bottom right (C) 
*
*/

#include <SPIFIFO.h>
#include "OC_DAC.h"
#include "OC_gpio.h"

#define SPICLOCK_30MHz   (SPI_CTAR_PBR(0) | SPI_CTAR_BR(0) | SPI_CTAR_DBR) //(60 / 2) * ((1+1)/2) = 30 MHz (= 24MHz, when F_BUS == 48000000)
//#define DAC8564 // <-- uncomment, if using DAC8564

namespace OC {

/*static*/
void DAC::Init(CalibrationData *calibration_data) {

  calibration_data_ = calibration_data;

  // set up DAC pins 
  pinMode(DAC_CS, OUTPUT);
  pinMode(DAC_RST,OUTPUT);
  
  #ifdef DAC8564 // A0 = 0, A1 = 0
    digitalWrite(DAC_RST, LOW); 
  #else  // default to DAC8565 - pull RST high 
    digitalWrite(DAC_RST, HIGH);
  #endif

  history_tail_ = 0;
  memset(history_, 0, sizeof(uint16_t) * kHistoryDepth * DAC_CHANNEL_LAST);

  if (F_BUS == 60000000 || F_BUS == 48000000) 
    SPIFIFO.begin(DAC_CS, SPICLOCK_30MHz, SPI_MODE0);  

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

}; // namespace OC

void set8565_CHA(uint32_t data) {
  uint32_t _data = OC::DAC::MAX_VALUE - data;

  SPIFIFO.write(0b00010000, SPI_CONTINUE);
  SPIFIFO.write16(_data);
  SPIFIFO.read();
  SPIFIFO.read();
}

void set8565_CHB(uint32_t data) {
  uint32_t _data = OC::DAC::MAX_VALUE - data;

  SPIFIFO.write(0b00010010, SPI_CONTINUE);
  SPIFIFO.write16(_data);
  SPIFIFO.read();
  SPIFIFO.read();
}

void set8565_CHC(uint32_t data) {
  uint32_t _data = OC::DAC::MAX_VALUE - data;

  SPIFIFO.write(0b00010100, SPI_CONTINUE);
  SPIFIFO.write16(_data);
  SPIFIFO.read();
  SPIFIFO.read(); 
}

void set8565_CHD(uint32_t data) {
  uint32_t _data = OC::DAC::MAX_VALUE - data;

  SPIFIFO.write(0b00010110, SPI_CONTINUE);
  SPIFIFO.write16(_data);
  SPIFIFO.read();
  SPIFIFO.read();
}

// adapted from https://github.com/xxxajk/spi4teensy3 (MISO disabled) : 

void SPI_init() {

  uint32_t ctar0, ctar1;

  SIM_SCGC6 |= SIM_SCGC6_SPI0;
  CORE_PIN11_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2);
  CORE_PIN13_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2);
  
  ctar0 = SPI_CTAR_DBR; // default
  #if   F_BUS == 60000000
      ctar0 = (SPI_CTAR_PBR(0) | SPI_CTAR_BR(0) | SPI_CTAR_DBR); //(60 / 2) * ((1+1)/2) = 30 MHz
  #elif F_BUS == 48000000
      ctar0 = (SPI_CTAR_PBR(0) | SPI_CTAR_BR(0) | SPI_CTAR_DBR); //(48 / 2) * ((1+1)/2) = 24 MHz          
  #endif
  ctar1 = ctar0;
  ctar0 |= SPI_CTAR_FMSZ(7);
  ctar1 |= SPI_CTAR_FMSZ(15);
  SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_PCSIS(0x1F);
  SPI0_MCR |= SPI_MCR_CLR_RXF | SPI_MCR_CLR_TXF;

  // update ctars
  uint32_t mcr = SPI0_MCR;
  if (mcr & SPI_MCR_MDIS) {
    SPI0_CTAR0 = ctar0;
    SPI0_CTAR1 = ctar1;
  } else {
    SPI0_MCR = mcr | SPI_MCR_MDIS | SPI_MCR_HALT;
    SPI0_CTAR0 = ctar0;
    SPI0_CTAR1 = ctar1;
    SPI0_MCR = mcr;
  }
}


