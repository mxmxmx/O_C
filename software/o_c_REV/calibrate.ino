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
const uint16_t _ADC_OFFSET = (uint16_t)((float)pow(2,_ADC_RES)*0.6666667f); // ADC offset

uint16_t _ADC_OFFSET_0 = _ADC_OFFSET;
uint16_t _ADC_OFFSET_1 = _ADC_OFFSET;
uint16_t _ADC_OFFSET_2 = _ADC_OFFSET;
uint16_t _ADC_OFFSET_3 = _ADC_OFFSET;
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
        
        u8g.setFont(u8g_font_6x12);
        u8g.setColorIndex(1);
        u8g.setFontRefHeightText();
        u8g.setFontPosTop();
      
        
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
                     encoder[RIGHT].setPos(_ADC_OFFSET_0); 
                     _B_event = 0x0;
                   }
                   _CV = analogRead(CV1);
                 break;
                 
                 case CV_OFFSET_1:
                   if (_B_event) {
                     encoder[RIGHT].setPos(_ADC_OFFSET_1); 
                     _B_event = 0x0;
                   }
                   _CV = analogRead(CV2);
                 break;
                 
                 case CV_OFFSET_2:
                   if (_B_event) {
                     encoder[RIGHT].setPos(_ADC_OFFSET_2); 
                     _B_event = 0x0;
                   }
                   _CV = analogRead(CV3);
                 break;
                 
                 case CV_OFFSET_3:
                   if (_B_event) {
                     encoder[RIGHT].setPos(_ADC_OFFSET_3); 
                     _B_event = 0x0;
                   }
                   _CV = analogRead(CV4);
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
  
    u8g.drawStr(10, 10, calib_menu_strings[_steps]); 
    
    switch (_steps) {
      
      case HELLO: {  
        
          u8g.drawStr(10, 30, "...");
          u8g.drawStr(10, 50, "push encoders");
          u8g.setPrintPos(87, 30);
          writeAllDAC(octaves[_ZERO]);
          break;
      }
      
      case VOLT_0: { 
          u8g.drawStr(10, 30, "->  0.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");
          octaves[_ZERO] = encoder_data;
          writeAllDAC(octaves[_ZERO]);
        break;   
      } 
      
      case VOLT_3m: { 
          u8g.drawStr(10, 30, "-> -3.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[0] = encoder_data;
          writeAllDAC(encoder_data);
          break;   
      } 
      
      case VOLT_2m: { 
          u8g.drawStr(10, 30, "-> -2.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[1] = encoder_data;
          writeAllDAC(encoder_data);
          break;   
      } 
      
      case VOLT_1m: { 
          u8g.drawStr(10, 30, "-> -1.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[2] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_0m: { 
          u8g.drawStr(10, 30, "->  0.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[_ZERO] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      }
      
       case VOLT_1: { 
          u8g.drawStr(10, 30, "->  1.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[4] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      }
      
      case VOLT_2: { 
          u8g.drawStr(10, 30, "->  2.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[5] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_3: { 
          u8g.drawStr(10, 30, "->  3.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");
          octaves[6] = encoder_data;  
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_4: { 
          u8g.drawStr(10, 30, "->  4.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[7] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_5: { 
          u8g.drawStr(10, 30, "->  5.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[8] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_6: { 
          u8g.drawStr(10, 30, "->  6.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[9] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case CV_OFFSET: { 
          u8g.setPrintPos(10, 30);
          u8g.print(_ADC_OFFSET);
          u8g.drawStr(45, 30, " ==");
          u8g.setPrintPos(78, 30);
          uint16_t _adc_val = _AVERAGE;
          u8g.print(_adc_val);
          u8g.drawStr(10, 50, "use trimpot!");  
        break;   
      }
      
      case CV_OFFSET_0: { 
          u8g.setPrintPos(10, 30);
          u8g.print(encoder_data - _CV);
          u8g.drawStr(40, 30, "--> 0"); 
          u8g.drawStr(10, 50, "use encoder!");
          _ADC_OFFSET_0 = encoder_data;   
          delay(20);
        break;   
      }
      
      case CV_OFFSET_1: { 
          u8g.setPrintPos(10, 30);
          u8g.print(encoder_data - _CV);
          u8g.drawStr(40, 30, "--> 0"); 
          u8g.drawStr(10, 50, "use encoder!"); 
          _ADC_OFFSET_1 = encoder_data; 
          delay(20);    
        break;   
      }
  
      case CV_OFFSET_2: { 
          u8g.setPrintPos(10, 30);
          u8g.print(encoder_data - _CV);
          u8g.drawStr(40, 30, "--> 0"); 
          u8g.drawStr(10, 50, "use encoder!");  
          _ADC_OFFSET_2 = encoder_data;
          delay(20);
        break;   
      }  
      
      case CV_OFFSET_3: { 
          u8g.setPrintPos(10, 30);
          u8g.print(encoder_data - _CV);
          u8g.drawStr(40, 30, "--> 0"); 
          u8g.drawStr(10, 50, "use encoder!");
          _ADC_OFFSET_3 = encoder_data;
          delay(20);  
        break;   
      }
      
      case EXIT: { 
        
        u8g.drawStr(10, 30, "push (R) to exit");  
        break;   
      } 
      
      default: break;   
 } 
} 

/* misc */ 

uint16_t _average() {
  
  float average = 0.0f;
  
  for (int i = 0; i < 50; i++) {
   
           average +=  analogRead(CV1);
           delay(1);
           average +=  analogRead(CV2);
           delay(1);
           average +=  analogRead(CV3);
           delay(1);
           average +=  analogRead(CV4);
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
  uint16_t _offset[4] = {_ADC_OFFSET_0, _ADC_OFFSET_1, _ADC_OFFSET_2, _ADC_OFFSET_3};
  
  for (int i = 0; i < numADC; i++) {
    
      byte0 = _offset[i] >> 8;
      byte1 = _offset[i];
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
   
   uint16_t _offset[numADC];
   
   for (int i = 0; i < numADC; i++) {  
  
       byte0 = EEPROM.read(adr);
       adr++;
       byte1 = EEPROM.read(adr);
       adr++;
       _offset[i] = (uint16_t)(byte0 << 8) + byte1;
       Serial.print(i);
       Serial.print(" --> ");
       Serial.println(_offset[i]);
   }
   
   _ADC_OFFSET_0 = _offset[0];
   _ADC_OFFSET_1 = _offset[1];
   _ADC_OFFSET_2 = _offset[2];
   _ADC_OFFSET_3 = _offset[3];
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

/* write to DAC */

void writeAllDAC(uint16_t _data) {

  set8565_CHA(_data); // ch A >> 
  set8565_CHB(_data); // ch B >>
  set8565_CHC(_data); // ch C >>
  set8565_CHD(_data); // ch D >> 
}
  


