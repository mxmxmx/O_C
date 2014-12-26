/*

calibration menu:

enter by pressing left encoder button during start up; follow the instructions


*/

#define CALIB_MENU_ITEMS 13

char *calib_menu_strings[CALIB_MENU_ITEMS] = {

   "--> calibrate:", 
   "trim center value:", 
   "trim to 1 volts", 
   "trim to 2 volts", 
   "trim to 3 volts", 
   "trim to 4 volts",
   "trim to 6 volts",
   "trim to 7 volts",
   "trim to 8 volts",
   "trim to 9 volts",
   "trim to 10 volts",
   "trim to 0.08333v",
   "done? / clear?"
};

enum STEPS {
  
  HELLO,
  CENTRE,
  VOLT_1,
  VOLT_2,
  VOLT_3,
  VOLT_4,
  VOLT_6,
  VOLT_7,
  VOLT_8,
  VOLT_9,
  VOLT_10,
  VOLT_0,
  CHECK
};  


uint8_t _steps = 0;
uint16_t DAC_output;
int32_t encoder_data;


/* make DAC table from calibration values */ 

void init_DACtable() {
  
  float diff, prev, out;
  uint16_t _off = octaves[0];
  octaves[0] = 0;
  for (int i = 0; i < OCTAVES; i++) {
       
        diff = ((float)(octaves[i+1] - octaves[i]))/12.0f;
        
        Serial.print(diff);Serial.print(" | ");
        Serial.print(prev);Serial.print(" | ");
        Serial.println(octaves[i]);
        for (int j = 0; j <= 11; j++) {       
            out = j*diff + prev; 
            semitones[j+i*12] = (uint16_t)(0.5f + out);  
            if (j == 11) { prev += diff*12; }
    }
  }
  octaves[0] = _off;
  /* deal w/ offset ? */
  if (octaves[0] != THEORY[0]) {
        diff = (float)(octaves[1] - octaves[0])/11.0f;
        semitones[0] = _off; // 0.08333 mv
        semitones[1] = _off; // 0.08333 mv
        //semitones[2] = _off;
        for (int j = 11; j > 2; j--) { 
            semitones[j] = uint16_t(0.5 + semitones[j+1] - diff);
        } 
  }  
}

/*        loop calibration menu until we're done       */

void calibrate_main() {
  
        UImode = CALIBRATE;
        MENU_REDRAW = 1;
        _steps = 0;
        /* values written? */
        if (EEPROM.read(0x2) > 0) readDACcode();    
        u8g.setFont(u8g_font_6x12);
        u8g.setColorIndex(1);
        u8g.setFontRefHeightText();
        u8g.setFontPosTop();
       
        while(UImode == CALIBRATE) {
         
             UI();
             delay(20);
             MENU_REDRAW = 1;
             
             /* advance in menu or exit */
             
             if (_steps >= CHECK) { 
                      
                       if (millis() - LAST_BUT > DEBOUNCE && !digitalRead(butR)) {
                         
                            UImode = SCREENSAVER;
                            LAST_BUT = millis();
                       }
                       
                       if (millis() -  LAST_BUT > DEBOUNCE && !digitalRead(but_top)) {
                            
                            clear();
                            _steps = HELLO;
                            LAST_BUT = millis(); 
                       } 
             } 
             else if (millis() - LAST_BUT > DEBOUNCE && !digitalRead(butR)) { 
                       
                             _steps++;  
                             if  (_steps == VOLT_0) encoder[RIGHT].setPos(octaves[0]);
                             else if (_steps == CENTRE) encoder[RIGHT].setPos(octaves[5]);
                             else if (_steps > CENTRE && _steps < VOLT_6) encoder[RIGHT].setPos(octaves[_steps-1]);
                             else if (_steps > VOLT_4) encoder[RIGHT].setPos(octaves[_steps]);
                             LAST_BUT = millis(); 
             }
             
             encoder_data = encoder[RIGHT].pos();
             if (encoder_data >= MAX_VALUE) encoder[RIGHT].setPos(MAX_VALUE);
        } 
        writeDACcode();
        encoder[RIGHT].setPos(0);
        UImode = SCREENSAVER;  
}


void calibrate() {
  
    u8g.drawStr(10, 10, calib_menu_strings[_steps]); 
    
    switch (_steps) {
      
      case HELLO: {  // 0
        
          u8g.drawStr(10, 30, "use encoder (R)");
          u8g.drawStr(10, 50, "switch: left pos.");
          writeAllDAC(THEORY[0]);
          break;
      }
      
      case CENTRE: {  // 1
        
          u8g.drawStr(10, 30, "- > 5.000V /");  
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use trimpots!");  
          octaves[5] = encoder_data;
          writeAllDAC(encoder_data);
          break;   
      }  
      
      case VOLT_1: { // 2
          u8g.drawStr(10, 30, "- > 1.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[_steps-1] = encoder_data;
          writeAllDAC(encoder_data);
          break;   
      } 
      
      case VOLT_2: { // 3
          u8g.drawStr(10, 30, "- > 2.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[_steps-1] = encoder_data;
          writeAllDAC(encoder_data);
          break;   
      } 
      
      case VOLT_3: { // 4
          u8g.drawStr(10, 30, "- > 3.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[_steps-1] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_4: { // 5
          u8g.drawStr(10, 30, "- > 4.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[_steps-1] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      }
      
      case VOLT_6: { // 6
          u8g.drawStr(10, 30, "- > 6.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[_steps] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_7: { // 7
          u8g.drawStr(10, 30, "- > 7.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");
          octaves[_steps] = encoder_data;  
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_8: { // 8
          u8g.drawStr(10, 30, "- > 8.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[_steps] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_9: { // 9
          u8g.drawStr(10, 30, "- > 9.000V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[_steps] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_10: { // 10
          u8g.drawStr(10, 30, "- > 10.00V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[_steps] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case VOLT_0: { // 11
          u8g.drawStr(10, 30, "- > .1667V /");
          u8g.setPrintPos(87, 30);
          u8g.print(encoder_data);
          u8g.drawStr(10, 50, "use encoder!");  
          octaves[0] = encoder_data;
          writeAllDAC(encoder_data);
        break;   
      } 
      
      case CHECK: {
        
        u8g.drawStr(10, 30, "push to exit");  
        break;   
      } 
      
      default: break;   
 } 
} 


void writeDACcode() {
   
  uint8_t byte0, byte1, adr;
  uint16_t _DACcode;
  
  for (int i = 0; i <= OCTAVES; i++) {  
    
       _DACcode = octaves[i];
       byte0 = _DACcode >> 8;
       byte1 = _DACcode;
       EEPROM.write(adr, byte0);
       adr++;
       EEPROM.write(adr, byte1);
       adr++;
  }  
}  


void readDACcode() {
  
   uint8_t byte0, byte1, adr;
   
   for (int i = 0; i <= OCTAVES; i++) {  
  
       byte0 = EEPROM.read(adr);
       adr++;
       byte1 = EEPROM.read(adr);
       adr++;
       octaves[i] = (uint16_t)(byte0 << 8) + byte1;
   }
}  
  
void clear() {
  
  uint8_t adr = 0;  
  
  for (int i = 0; i <= OCTAVES; i++) {  
       EEPROM.write(adr, 0x00);
       adr++;
       EEPROM.write(adr, 0x00);
       adr++;
  }  
}
  
void in_theory() {

   for (int i = 0; i <= OCTAVES; i++) {  
     
     octaves[i] = THEORY[i];
   }
}

void writeAllDAC(uint16_t _data) {

  set8565_CHA(_data); // ch A >> 
  set8565_CHB(_data); // ch B >>
  set8565_CHC(_data); // ch C >>
  set8565_CHD(_data); // ch D >> 
}
  

uint8_t testnote = 12; 

void xxx() {
  
  testnote++;
  if (testnote > 36) testnote = 12;
  int16_t x = getnote(testnote, 3, 7);
  uint16_t y = semitones[testnote];
  Serial.println(y);
  writeAllDAC(y);
}
