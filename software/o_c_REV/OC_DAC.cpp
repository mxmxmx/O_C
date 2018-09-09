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

#include "util/util_SPIFIFO.h"
#include "OC_DAC.h"
#include "OC_gpio.h"
#include "OC_options.h"
#include "OC_calibration.h"
#include "OC_autotune_presets.h"
#include "OC_autotune.h"

#define SPICLOCK_30MHz   (SPI_CTAR_PBR(0) | SPI_CTAR_BR(0) | SPI_CTAR_DBR) //(60 / 2) * ((1+1)/2) = 30 MHz (= 24MHz, when F_BUS == 48000000)

namespace OC {

/*static*/
void DAC::Init(CalibrationData *calibration_data) {

  calibration_data_ = calibration_data;
  
  restore_scaling(0x0);

  // set up DAC pins 
  OC::pinMode(DAC_CS, OUTPUT);
  OC::pinMode(DAC_RST,OUTPUT);
  
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
uint8_t DAC::calibration_data_used(uint8_t channel_id) {
  const OC::Autotune_data &autotune_data = OC::AUTOTUNE::GetAutotune_data(channel_id);
  return autotune_data.use_auto_calibration_;
}
/*static*/
void DAC::set_auto_channel_calibration_data(uint8_t channel_id) {
  
  SERIAL_PRINTLN("use auto calibration data ... (channel: %d)", channel_id + 1);
  
  if (channel_id < DAC_CHANNEL_LAST) {
  
    OC::Autotune_data *_autotune_data = &OC::auto_calibration_data[channel_id];
    if (_autotune_data->use_auto_calibration_ == 0xFF)  { // = data available ?
      
        _autotune_data->use_auto_calibration_ = 0x01; // = use auto data 
        // update data:
        const OC::Autotune_data &autotune_data = OC::AUTOTUNE::GetAutotune_data(channel_id);
        for (int i = 0; i < OCTAVES + 1; i++)
          calibration_data_->calibrated_octaves[channel_id][i] = autotune_data.auto_calibrated_octaves[i];
    } 
  }
}
/*static*/
void DAC::set_default_channel_calibration_data(uint8_t channel_id) {
  
  SERIAL_PRINTLN("reset to core/default calibration data ... (channel: %d)", channel_id + 1);
  
  if (channel_id < DAC_CHANNEL_LAST) {

    // reset data
    for (int i = 0; i < OCTAVES + 1; i++) 
      calibration_data_->calibrated_octaves[channel_id][i] = OC::calibration_data.dac.calibrated_octaves[channel_id][i];
    // + update info
    OC::Autotune_data *autotune_data = &OC::auto_calibration_data[channel_id];
    if (autotune_data->use_auto_calibration_ == 0xFF || autotune_data->use_auto_calibration_ == 0x01)
      autotune_data->use_auto_calibration_ = 0xFF; // = data available, but not used
    else 
      autotune_data->use_auto_calibration_ = 0x00;
  }
}
/*static*/
void DAC::update_auto_channel_calibration_data(uint8_t channel_id, int8_t octave, uint32_t pitch_data) {
  
  SERIAL_PRINTLN("update: ch.#%d -> %d (%d)", (int)(channel_id + 1), (int)pitch_data, (int)octave);
  
  if (channel_id < DAC_CHANNEL_LAST) {

      // write data
      OC::Autotune_data *autotune_data = &OC::auto_calibration_data[channel_id];
      autotune_data->auto_calibrated_octaves[octave] = pitch_data;
      // + update info
      if (octave == OCTAVES) {
        autotune_data->use_auto_calibration_ = 0xFF; // = data available, but not used
        SERIAL_PRINTLN("set auto-calibration data, %d", autotune_data->use_auto_calibration_);
      }
   }
}
/*static*/
void DAC::reset_auto_channel_calibration_data(uint8_t channel_id) {
  
  // reset data
  if (channel_id < DAC_CHANNEL_LAST) {
    SERIAL_PRINTLN("reset channel# %d calibration data", (int)(channel_id + 1));
    OC::Autotune_data *autotune_data = &OC::auto_calibration_data[channel_id];
    autotune_data->use_auto_calibration_ = 0x0;
    for (int i = 0; i < OCTAVES + 1; i++)
      autotune_data->auto_calibrated_octaves[i] = OC::calibration_data.dac.calibrated_octaves[channel_id][i];
  }
}
/*static*/
void DAC::reset_all_auto_channel_calibration_data(){
   for (int i = 0; i < DAC_CHANNEL_LAST; i++)
      reset_auto_channel_calibration_data(i);
}
/*static*/
void DAC::choose_calibration_data() {
  
  // at this point, global settings are restored
  for (int i = 0; i < DAC_CHANNEL_LAST; i++) {
    
    const OC::Autotune_data &autotune_data = OC::AUTOTUNE::GetAutotune_data(i);

    if (autotune_data.use_auto_calibration_ == 0x0) { 
    // no autotune_data yet, so we use defaults:
      reset_auto_channel_calibration_data(i);
    }
    else if (autotune_data.use_auto_calibration_ == 0xFF) {
    // use autotune_data
      set_auto_channel_calibration_data(i);
    }
    // ... else use default calibration
  }
}
/*static*/
uint8_t DAC::get_voltage_scaling(uint8_t channel_id) {
  return DAC_scaling[channel_id];
}
/*static*/
void DAC::set_scaling(uint8_t scaling, uint8_t channel_id) {

  if (channel_id < DAC_CHANNEL_LAST)
    DAC_scaling[channel_id] = scaling;
}
/*static*/
void DAC::restore_scaling(uint32_t scaling) {
  
  // restore scaling from global settings
  for (int i = 0; i < DAC_CHANNEL_LAST; i++) {
    uint8_t _scaling = (scaling >> (i * 8)) & 0xFF;
    set_scaling(_scaling, i);
  }
}
uint32_t DAC::store_scaling() {

  uint32_t _scaling = 0;
  // merge values into uint32_t : 
  for (int i = 0; i < DAC_CHANNEL_LAST; i++)
    _scaling |= (DAC_scaling[i] << (i * 8)); 
  return _scaling;
}
/*static*/
DAC::CalibrationData *DAC::calibration_data_ = nullptr;
/*static*/
uint32_t DAC::values_[DAC_CHANNEL_LAST];
/*static*/
uint16_t DAC::history_[DAC_CHANNEL_LAST][DAC::kHistoryDepth];
/*static*/ 
volatile size_t DAC::history_tail_;
/*static*/ 
uint8_t DAC::DAC_scaling[DAC_CHANNEL_LAST];
}; // namespace OC

void set8565_CHA(uint32_t data) {
  #ifdef BUCHLA_cOC
  uint32_t _data = data;
  #else
  uint32_t _data = OC::DAC::MAX_VALUE - data;
  #endif
  #ifdef FLIP_180
  SPIFIFO.write(0b00010110, SPI_CONTINUE);
  #else
  SPIFIFO.write(0b00010000, SPI_CONTINUE);
  #endif
  SPIFIFO.write16(_data);
  SPIFIFO.read();
  SPIFIFO.read();
}

void set8565_CHB(uint32_t data) {
  #ifdef BUCHLA_cOC
  uint32_t _data = data;
  #else
  uint32_t _data = OC::DAC::MAX_VALUE - data;
  #endif
  #ifdef FLIP_180
  SPIFIFO.write(0b00010100, SPI_CONTINUE);
  #else
  SPIFIFO.write(0b00010010, SPI_CONTINUE);
  #endif
  SPIFIFO.write16(_data);
  SPIFIFO.read();
  SPIFIFO.read();
}

void set8565_CHC(uint32_t data) {
  #ifdef BUCHLA_cOC
  uint32_t _data = data;
  #else
  uint32_t _data = OC::DAC::MAX_VALUE - data;
  #endif
  #ifdef FLIP_180
  SPIFIFO.write(0b00010010, SPI_CONTINUE);
  #else
  SPIFIFO.write(0b00010100, SPI_CONTINUE);
  #endif
  SPIFIFO.write16(_data);
  SPIFIFO.read();
  SPIFIFO.read(); 
}

void set8565_CHD(uint32_t data) {
  #ifdef BUCHLA_cOC
  uint32_t _data = data;
  #else
  uint32_t _data = OC::DAC::MAX_VALUE - data;
  #endif
  #ifdef FLIP_180
  SPIFIFO.write(0b00010000, SPI_CONTINUE);
  #else
  SPIFIFO.write(0b00010110, SPI_CONTINUE);
  #endif
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
// OC_DAC
