/*
*
* calibration menu:
*
* enter by pressing left encoder button during start up; use encoder switches to navigate.
*
*/

const uint16_t CALIB_MENU_ITEMS = 18;                           // menu items
const uint16_t _ZERO = 0x3;                                     // "zero" code < > octave 4
const uint16_t _OFFSET = 4890;                                  // DAC offset, initial approx., ish --> -3.5V to 6V
const uint16_t _ADC_OFFSET = (uint16_t)((float)pow(2,OC::ADC::kAdcResolution)*0.6666667f); // ADC offset

OC::ADC::CalibrationData adc_calibration_data = { _ADC_OFFSET, _ADC_OFFSET, _ADC_OFFSET, _ADC_OFFSET };
uint16_t _AVERAGE = 0x0;
uint16_t _CV = 0x0;
uint16_t _exit = 0x0;

const char *calib_menu_strings[CALIB_MENU_ITEMS] = {

   "--> calibrate:", 
   "trim to 0 volts", 
   "trim to -3 volts", 
   "trim to -2 volts", 
   "trim to -1 volts", 
   "trim to 0 volts",
   "trim to 1 volts",
   "trim to 2 volts",
   "trim to 3 volts",
   "trim to 4 volts",
   "trim to 5 volts",
   "trim to 6 volts",
   "trim CV offset",
   "CV (sample)",
   "CV (index)",
   "CV (notes/scale)",
   "CV (transpose)",
   "done?"
};

enum STEPS 
{  
  HELLO,
  VOLT_0,
  VOLT_3m,
  VOLT_2m,
  VOLT_1m,
  VOLT_0m,
  VOLT_1,
  VOLT_2,
  VOLT_3,
  VOLT_4,
  VOLT_5,
  VOLT_6,
  CV_OFFSET,
  CV_OFFSET_0,
  CV_OFFSET_1,
  CV_OFFSET_2,
  CV_OFFSET_3,
  EXIT
};  


int16_t  _steps;
uint16_t _B_event;
int32_t encoder_data;
uint16_t DAC_output;


/* write to DAC */

void writeAllDAC(uint16_t _data) {
  DAC::set_all(_data);
}


/* make DAC table from calibration values */ 

void init_DACtable() {

  float _diff, _offset, _semitone;
  _offset = octaves[OCTAVES-2];          // = 5v
  semitones[RANGE] = octaves[OCTAVES-1]; // = 6v
  
  // 6v to -3v:
  for (int i = OCTAVES-1; i > 0; i--) {
    
      _diff = (float)(octaves[i] - _offset)/12.0f;
      Serial.print(i);
      Serial.print(" --> ");
      Serial.print(_diff);
      Serial.println("");
  
      for (int j = 0; j <= 11; j++) {
        
           _semitone = j*_diff + _offset; 
           semitones[j+i*12-1] = (uint16_t)(0.5f + _semitone);
           if (j == 11 && i > 1) _offset = octaves[i-2];
           Serial.print(_offset);
           Serial.print(" -> ");
           Serial.print(j+i*12-1);
           Serial.print(" -> ");
           Serial.println(semitones[j+i*12-1]);
      } 
   }
   // fill up remaining semitones (from -3.000v )
   int _fill = 10;
   while (_fill >= 0) {
     
          if (_offset > _diff*2) { 
              semitones[_fill] = _offset - _diff;
              _offset -= _diff;   
          }
          else semitones[_fill] = _diff;
       
          Serial.print(_offset);
          Serial.print(" -> ");
          Serial.print(_fill);
          Serial.print(" -> ");
          Serial.println(semitones[_fill]);
          _fill--;    
   }
}

/*     loop calibration menu until done       */

void calibrate_main() {
  
        UI_MODE = CALIBRATE;
        MENU_REDRAW = 1;
        _steps = 0;
        /* values written? */
        //if (EEPROM.read(0x2) > 0) readDACcode(); 
        //else 
        //delay(1000);
        in_theory();   
        
        while(UI_MODE == CALIBRATE) {
         
             UI();
             delay(20);
             MENU_REDRAW = 0x1;
             
             if (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE && !digitalRead(butR)) {
                         
                            if (_steps < EXIT) _steps++;
                            else if (_steps == EXIT) _exit = 0x1;
                            _BUTTONS_TIMESTAMP = millis();
                            _B_event = 0x1;
              }
              if (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE && !digitalRead(butL)) {
                         
                            if (_steps > VOLT_0) _steps--;
                            _BUTTONS_TIMESTAMP = millis();
                            _B_event = 0x1;
              }
   
              switch(_steps) {
                
                 case EXIT:
                   if (_exit) 
                   {
                     save_settings();
                     encoder[RIGHT].setPos(0x0);
                     UI_MODE = SCREENSAVER; 
                     _exit = 0x0;
                   }
                 break;
                 
                 case VOLT_0:
                   if (_B_event) {
                     encoder[RIGHT].setPos(octaves[_ZERO]); 
                     _B_event = 0x0;
                   }
                 break;
                 
                 case VOLT_3m:
                   if (_B_event) {
                     encoder[RIGHT].setPos(octaves[0]); 
                     _B_event = 0x0;
                   }
                 break;
                 
                 case VOLT_2m:
                   if (_B_event) {
                     encoder[RIGHT].setPos(octaves[1]); 
                     _B_event = 0x0;
                   }
                 break;
                 
                 case VOLT_1m:
                   if (_B_event) {
                     encoder[RIGHT].setPos(octaves[2]); 
                     _B_event = 0x0;
                   }
                 break;
                 
                 case VOLT_0m:
                   if (_B_event) {
                     encoder[RIGHT].setPos(octaves[3]); 
                     _B_event = 0x0;
                   }
                 break;
                 
                 case VOLT_1:
                   if (_B_event) {
                     encoder[RIGHT].setPos(octaves[4]); 
                     _B_event = 0x0;
                   }
                 break;
                 
                 case VOLT_2:
                   if (_B_event) {
                     encoder[RIGHT].setPos(octaves[5]); 
                     _B_event = 0x0;
                   }
                 break;
                 
                 case VOLT_3:
                   if (_B_event) {
                     encoder[RIGHT].setPos(octaves[6]); 
                     _B_event = 0x0;
                   } 
                 break;
                 
                 case VOLT_4:
                   if (_B_event) {
                     encoder[RIGHT].setPos(octaves[7]); 
                     _B_event = 0x0;
                   }
                 break;
                 
                 case VOLT_5:
                   if (_B_event) {
                     encoder[RIGHT].setPos(octaves[8]); 
                     _B_event = 0x0;
                   }
                 break;
                 
                 case VOLT_6:
                   if (_B_event) {
                     encoder[RIGHT].setPos(octaves[9]); 
                     _B_event = 0x0;
                   }
                 break;
               
                 case CV_OFFSET:
                   _AVERAGE = _average();
                 break;
                 
                 case CV_OFFSET_0:
                   if (_B_event) {
                     encoder[RIGHT].setPos(adc_calibration_data.offset[ADC_CHANNEL_1]); 
                     _B_event = 0x0;
                   }
                   _CV = OC::ADC::raw_value(ADC_CHANNEL_1);
                 break;
                 
                 case CV_OFFSET_1:
                   if (_B_event) {
                     encoder[RIGHT].setPos(adc_calibration_data.offset[ADC_CHANNEL_2]); 
                     _B_event = 0x0;
                   }
                   _CV = OC::ADC::raw_value(ADC_CHANNEL_2);
                 break;
                 
                 case CV_OFFSET_2:
                   if (_B_event) {
                     encoder[RIGHT].setPos(adc_calibration_data.offset[ADC_CHANNEL_3]); 
                     _B_event = 0x0;
                   }
                   _CV = OC::ADC::raw_value(ADC_CHANNEL_3);
                 break;
                 
                 case CV_OFFSET_3:
                   if (_B_event) {
                     encoder[RIGHT].setPos(adc_calibration_data.offset[ADC_CHANNEL_4]); 
                     _B_event = 0x0;
                   }
                   _CV = OC::ADC::raw_value(ADC_CHANNEL_4);
                 break;
                 
                 default:
                 break;
              }
              
         encoder_data = encoder[RIGHT].pos();
         if (encoder_data >= MAX_VALUE) encoder[RIGHT].setPos(MAX_VALUE);  
  } 
}

/* DAC output etc */ 

void calibrate() {
    GRAPHICS_BEGIN_FRAME(true);

    graphics.drawStr(10, 10, calib_menu_strings[_steps]); 
    
    switch (_steps) {
      
      case HELLO: {  
        
          graphics.drawStr(10, 30, "...");
          graphics.drawStr(10, 50, "push encoders");
          graphics.setPrintPos(87, 30);
          writeAllDAC(octaves[_ZERO]);
          break;
      }
      
      case VOLT_0: { 
          graphics.drawStr(10, 30, "->  0.000V /");
          graphics.setPrintPos(87, 30);
          graphics.print(encoder_data);
          graphics.drawStr(10, 50, "use encoder!");
          octaves[_ZERO] = encoder_data;
          writeAllDAC(octaves[_ZERO]);
        break;   
      } 
      
      case VOLT_3m: { 
          graphics.drawStr(10, 30, "-> -3.000V /");
          graphics.setPrintPos(87, 30);
          graphics.print(encoder_data);
          graphics.drawStr(10, 50, "use encoder!");  
          octaves[0] = encoder_data;
          writeAllDAC(encoder_data);
          break;   
      } 
      
      case VOLT_2m: { 
          graphics.drawStr(10, 30, "-> -2.000V /");
          graphics.setPrintPos(87, 30);
          graphics.print(encoder_data);
          graphics.drawStr(10, 50, "use encoder!");  
          octaves[1] = encoder_data;
          writeAllDAC(encoder_data);
          break;   
      } 
      
      case VOLT_1m: { 
          graphics.drawStr(10, 30, "-> -1.000V /");
          graphics.setPrintPos(87, 30);
          graphics.print(encoder_data);
          graphics.drawStr(10, 50, "use encoder!");  
          octaves[2] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_0m: { 
          graphics.drawStr(10, 30, "->  0.000V /");
          graphics.setPrintPos(87, 30);
          graphics.print(encoder_data);
          graphics.drawStr(10, 50, "use encoder!");  
          octaves[_ZERO] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      }
      
       case VOLT_1: { 
          graphics.drawStr(10, 30, "->  1.000V /");
          graphics.setPrintPos(87, 30);
          graphics.print(encoder_data);
          graphics.drawStr(10, 50, "use encoder!");  
          octaves[4] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      }
      
      case VOLT_2: { 
          graphics.drawStr(10, 30, "->  2.000V /");
          graphics.setPrintPos(87, 30);
          graphics.print(encoder_data);
          graphics.drawStr(10, 50, "use encoder!");  
          octaves[5] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_3: { 
          graphics.drawStr(10, 30, "->  3.000V /");
          graphics.setPrintPos(87, 30);
          graphics.print(encoder_data);
          graphics.drawStr(10, 50, "use encoder!");
          octaves[6] = encoder_data;  
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_4: { 
          graphics.drawStr(10, 30, "->  4.000V /");
          graphics.setPrintPos(87, 30);
          graphics.print(encoder_data);
          graphics.drawStr(10, 50, "use encoder!");  
          octaves[7] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_5: { 
          graphics.drawStr(10, 30, "->  5.000V /");
          graphics.setPrintPos(87, 30);
          graphics.print(encoder_data);
          graphics.drawStr(10, 50, "use encoder!");  
          octaves[8] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_6: { 
          graphics.drawStr(10, 30, "->  6.000V /");
          graphics.setPrintPos(87, 30);
          graphics.print(encoder_data);
          graphics.drawStr(10, 50, "use encoder!");  
          octaves[9] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case CV_OFFSET: { 
          graphics.setPrintPos(10, 30);
          graphics.print(_ADC_OFFSET);
          graphics.drawStr(45, 30, " ==");
          graphics.setPrintPos(78, 30);
          uint16_t _adc_val = _AVERAGE;
          graphics.print(_adc_val);
          graphics.drawStr(10, 50, "use trimpot!");  
        break;   
      }
      
      case CV_OFFSET_0: { 
          graphics.setPrintPos(10, 30);
          graphics.print(encoder_data - _CV);
          graphics.drawStr(40, 30, "--> 0"); 
          graphics.drawStr(10, 50, "use encoder!");
          adc_calibration_data.offset[ADC_CHANNEL_1] = encoder_data;   
          delay(20);
        break;   
      }
      
      case CV_OFFSET_1: { 
          graphics.setPrintPos(10, 30);
          graphics.print(encoder_data - _CV);
          graphics.drawStr(40, 30, "--> 0"); 
          graphics.drawStr(10, 50, "use encoder!"); 
          adc_calibration_data.offset[ADC_CHANNEL_2] = encoder_data; 
          delay(20);    
        break;   
      }
  
      case CV_OFFSET_2: { 
          graphics.setPrintPos(10, 30);
          graphics.print(encoder_data - _CV);
          graphics.drawStr(40, 30, "--> 0"); 
          graphics.drawStr(10, 50, "use encoder!");  
          adc_calibration_data.offset[ADC_CHANNEL_3] = encoder_data;
          delay(20);
        break;   
      }  
      
      case CV_OFFSET_3: { 
          graphics.setPrintPos(10, 30);
          graphics.print(encoder_data - _CV);
          graphics.drawStr(40, 30, "--> 0"); 
          graphics.drawStr(10, 50, "use encoder!");
          adc_calibration_data.offset[ADC_CHANNEL_4] = encoder_data;
          delay(20);  
        break;   
      }
      
      case EXIT: { 
        
        graphics.drawStr(10, 30, "push (R) to exit");  
        break;   
      } 
      
      default: break;   
   }
   GRAPHICS_END_FRAME();
} 

/* misc */ 

uint16_t _average() {
  
  float average = 0.0f;
  
  for (int i = 0; i < 50; i++) {
   
           average +=  OC::ADC::raw_value(ADC_CHANNEL_1);
           delay(1);
           average +=  OC::ADC::raw_value(ADC_CHANNEL_2);
           delay(1);
           average +=  OC::ADC::raw_value(ADC_CHANNEL_3);
           delay(1);
           average +=  OC::ADC::raw_value(ADC_CHANNEL_4);
           delay(1);
      }
      
  return average / 200.0f;
}

/* save settings */ 

void save_settings() {
   
  uint8_t byte0, byte1, adr;
  uint16_t _DACcode;
  
  adr = 0; 
  // write DAC code
  for (int i = 0; i < OCTAVES; i++) {  
    
       _DACcode = octaves[i];
       byte0 = _DACcode >> 8;
       byte1 = _DACcode;
       EEPROM.write(adr, byte0);
       adr++;
       EEPROM.write(adr, byte1);
       adr++;
  }  
  // write CV offset
  for (int i = 0; i < ADC_CHANNEL_LAST; i++) {
    
      byte0 = adc_calibration_data.offset[i] >> 8;
      byte1 = adc_calibration_data.offset[i];
      EEPROM.write(adr, byte0);
      adr++;
      EEPROM.write(adr, byte1);
      adr++;
  }
}  

/* read settings */

void read_settings() {
  
   delay(1000);
   uint8_t byte0, byte1, adr;
   
   adr = 0; 
   Serial.println("from eeprom:");
   
   for (int i = 0; i < OCTAVES; i++) {  
  
       byte0 = EEPROM.read(adr);
       adr++;
       byte1 = EEPROM.read(adr);
       adr++;
       octaves[i] = (uint16_t)(byte0 << 8) + byte1;
       Serial.print(i);
       Serial.print(" --> ");
       Serial.println(octaves[i]);
   }
   
   uint16_t _offset[ADC_CHANNEL_LAST];
   
   for (int i = 0; i < ADC_CHANNEL_LAST; i++) {  
  
       byte0 = EEPROM.read(adr);
       adr++;
       byte1 = EEPROM.read(adr);
       adr++;
       _offset[i] = (uint16_t)(byte0 << 8) + byte1;
       Serial.print(i);
       Serial.print(" --> ");
       Serial.println(_offset[i]);
   }
   
   adc_calibration_data.offset[ADC_CHANNEL_1] = _offset[0];
   adc_calibration_data.offset[ADC_CHANNEL_2] = _offset[1];
   adc_calibration_data.offset[ADC_CHANNEL_3] = _offset[2];
   adc_calibration_data.offset[ADC_CHANNEL_4] = _offset[3];
   Serial.println("......");
   Serial.println("");
}  
  
/* octaves */  
  
void in_theory() {

   for (int i = 0; i <= OCTAVES; i++) 
   {  
     octaves[i] = THEORY[i] + _OFFSET;
     Serial.println(octaves[i]);
   }
}

