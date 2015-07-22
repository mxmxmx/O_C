/*
*
* encoders + buttons + ADC stuff
*
*/

extern uint8_t MENU_CURRENT; 
extern const int16_t param_limits[][2];
extern const int8_t MAXSCALES;
extern int16_t SCALE_SEL;
extern const int8_t MENU_ITEMS;

uint8_t LAST_SCALE = 0;
uint8_t SCALE_CHANGE = 0;
uint32_t _UI_TIMESTAMP;

// CV 
extern uint16_t _ADC_OFFSET_0;
extern uint16_t _ADC_OFFSET_1;
extern uint16_t _ADC_OFFSET_2;
extern uint16_t _ADC_OFFSET_3;

uint16_t ADC_CNT  = 0;

enum encoders 
{  
  LEFT, 
  RIGHT
};

enum DISPLAY_PAGE 
{  
 SCREENSAVER,
 MENU,
 CALIBRATE 
};  

/* --------------- encoders ---------------------------------------  */

/* encoders variables  */
uint8_t SLOWDOWN = 1; 

/* encoders isr */
void left_encoder_ISR() 
{
  encoder[LEFT].process();
}

void right_encoder_ISR() 
{
  encoder[RIGHT].process();
}

/* ---------------  update params ---------------------------------  */  

void update_ENC()  {
    
       _ENC = false;
      int16_t tmp = encoder[RIGHT].pos();
       
      if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); }  
     
      /* parameters: */
      if (tmp != asr_params[MENU_CURRENT]) {    // params changed?
          int16_t tmp2, tmp3; 
          tmp2 = param_limits[MENU_CURRENT][0]; // lower limit
          tmp3 = param_limits[MENU_CURRENT][1]; // upper limit
           
          if (tmp < tmp2)      { asr_display_params[MENU_CURRENT] = asr_params[MENU_CURRENT] = tmp2; encoder[RIGHT].setPos(tmp2);}
          else if (tmp > tmp3) { asr_display_params[MENU_CURRENT] = asr_params[MENU_CURRENT] = tmp3; encoder[RIGHT].setPos(tmp3);}
          else                 { asr_display_params[MENU_CURRENT] = asr_params[MENU_CURRENT] = tmp;  }
          MENU_REDRAW = 1;
      }
  
     if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); }   
     
     if (UI_MODE) {
     /* scale select: */   
         tmp = encoder[LEFT].pos()>>SLOWDOWN;
         if (tmp != LAST_SCALE) {
             if (tmp >= MAXSCALES)  { SCALE_SEL = 0; encoder[LEFT].setPos(0);}
             else if (tmp < 0)      { SCALE_SEL = MAXSCALES-1; encoder[LEFT].setPos((MAXSCALES-1)<<SLOWDOWN);}
             else SCALE_SEL = tmp;
             if (asr_params[0] != SCALE_SEL) SCALE_CHANGE = 1; 
             else SCALE_CHANGE = 0; 
             MENU_REDRAW = 1;
             LAST_SCALE = SCALE_SEL;
             _UI_TIMESTAMP = millis();
         }
     }
}

/* --- read  ADC ------ */

void CV() {
  
      cvval[0] = _ADC_OFFSET_0 - analogRead(CV1); // sample in
      
      ADC_CNT = ADC_CNT++ > 0x2 ? 0x0 : ADC_CNT;
      
      switch(ADC_CNT) {
        
        case 0: cvval[1] = 0x1+((_ADC_OFFSET_1 - analogRead(CV2)) >> 5); break;            // index -64/64 
        case 1: cvval[2] = 0x1+((_ADC_OFFSET_2 - analogRead(CV3)) >> 8); break;            // # notes -8/8
        case 2: cvval[3] = 0x1+((_ADC_OFFSET_3 - analogRead(CV4)) >> 9); break;            // octave offset
      }
      _ADC = false;
}


/* --- check buttons --- */

void buttons(uint8_t _button) {
  
  switch(_button) {
    
    case 0: { 
      
         if (!digitalReadFast(but_top) && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) { 
           topButton();
           _BUTTONS_TIMESTAMP = millis();   
         } 
    }
    
    case 1: { 
      
         if (!digitalReadFast(but_bot) && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) {
           lowerButton();
           _BUTTONS_TIMESTAMP = millis();
         }
    }
    
    case 2: {
      
         if (!digitalReadFast(butL) && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) {
           leftButton(); //
           _BUTTONS_TIMESTAMP = millis();   
         }
    } 
   
    case 3: {
     
         if (!digitalReadFast(butR) && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) {
           rightButton(); //
           _BUTTONS_TIMESTAMP = millis();
         }      
    }  
  } 
}
 
 
/* --------------- button misc ------------------- */

void rightButton() { 
  
  if (UI_MODE) update_menu();
  _UI_TIMESTAMP = millis();
  UI_MODE = MENU;

} 

/* -----------------------------------------------  */

void leftButton() {
  
  if (UI_MODE) update_scale();
  _UI_TIMESTAMP = millis();
  UI_MODE = MENU;
} 

/* -----------------------------------------------  */

void topButton() {
 
   if (UI_MODE) MENU_REDRAW = 1; 
         int8_t tmp = asr_params[1];
         tmp++;
         if (tmp > 4) tmp = 4; 
         asr_params[1] = tmp; 
   //   }
   //_UI_TIMESTAMP = millis();
   //UI_MODE = MENU;
}

/* -----------------------------------------------  */

void lowerButton() {

  if (UI_MODE) MENU_REDRAW = 1; 
         int8_t tmp = asr_params[1];
         tmp--;
         if (tmp < -3) tmp = -3; 
         asr_params[1] = tmp; 
   //    }
   //_UI_TIMESTAMP = millis();
   //UI_MODE = MENU;  
}

/* -----------------------------------------------  */

void update_menu(void) {
 
      MENU_CURRENT++;
      if ( MENU_CURRENT >= MENU_ITEMS) MENU_CURRENT = 2;
      encoder[RIGHT].setPos(asr_params[MENU_CURRENT]); 
      MENU_REDRAW = 1;  
}

/* -----------------------------------------------  */

void update_scale() {
  
     asr_params[0] = SCALE_SEL;
     encoder[LEFT].setPos(SCALE_SEL<<SLOWDOWN); 
     SCALE_CHANGE = 0;
     MENU_REDRAW = 1;
      
}  


