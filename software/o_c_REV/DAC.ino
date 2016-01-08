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


/*static*/
uint32_t DAC::values_[DAC_CHANNEL_LAST];

void set8565_CHA(uint32_t data) {
  
       		uint8_t _DACout[3];
                uint32_t _data = MAX_VALUE - data;
                 
                _DACout[0] = B00010000;
                _DACout[1] = uint8_t(_data>>8);
                _DACout[2] = uint8_t(_data);
                
                digitalWriteFast(CS, LOW); 
       		spi4teensy3::send(_DACout, 3);
                digitalWriteFast(CS, HIGH); 
}

void set8565_CHB(uint32_t data) {
  
       		uint8_t _DACout[3];
                uint32_t _data = MAX_VALUE - data;
                
                _DACout[0] = B00010010;
                _DACout[1] = uint8_t(_data>>8);
                _DACout[2] = uint8_t(_data);
                
                digitalWriteFast(CS, LOW); 
       		spi4teensy3::send(_DACout, 3);
                digitalWriteFast(CS, HIGH); 
}

void set8565_CHC(uint32_t data) {
  
       		uint8_t _DACout[3];
                uint32_t _data = MAX_VALUE - data;

		            _DACout[0] = B00010100;
                _DACout[1] = uint8_t(_data>>8);
                _DACout[2] = uint8_t(_data);
                
                digitalWriteFast(CS, LOW); 
       		spi4teensy3::send(_DACout, 3);
                digitalWriteFast(CS, HIGH); 
}

void set8565_CHD(uint32_t data) {
  
       		uint8_t _DACout[3];
                uint32_t _data = MAX_VALUE - data;
                
                _DACout[0] = B00010110;
                _DACout[1] = uint8_t(_data>>8);
                _DACout[2] = uint8_t(_data);
                
                digitalWriteFast(CS, LOW); 
       		spi4teensy3::send(_DACout, 3);  
                digitalWriteFast(CS, HIGH); 
}


