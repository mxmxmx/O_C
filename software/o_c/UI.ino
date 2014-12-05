/*

encoders + buttons + ADC stuff

*/


extern uint8_t MENU_CURRENT; 
extern const int16_t param_limits[][2];
extern const int8_t MAXSCALES;
extern int16_t SCALE_SEL;
extern const int8_t MENU_ITEMS;

uint8_t LAST_SCALE = 0;
uint8_t SCALE_CHANGE = 0;


enum encoders {
  
  LEFT, RIGHT
  
};

enum DISPLAY_PAGE {
  
 SCREENSAVER,
 MENU,
 CALIBRATE 
 
};  

/* --------------- encoders ---------------------------------------  */

/* encoders variables  */
uint8_t ENCODER_SWAP, SLOWDOWN = 1; 

/* encoder data */
int16_t LR_encoderdata[]  = {-999, -999};

/* isr */
void left_encoder_ISR() {
  encoder[LEFT].process();
}

void right_encoder_ISR() {
  encoder[RIGHT].process();
}

/*  read */
void encoders() {
  
   int16_t encoderdata; 
   /* toggle encoder */
   int8_t thisenc = ~ENCODER_SWAP & 1u; 
   if (encoder[thisenc].change()) { 
        
       encoderdata = encoder[thisenc].pos();
       if (!thisenc || UImode) { 
             LAST_UI = millis();
             UImode = MENU;
       }
       LR_encoderdata[thisenc] = encoderdata;  
   }  
   ENCODER_SWAP = thisenc;
}   
  

/* ---------------  update params ---------------------------------  */  

void update_ENC()  {

  encoders(); 
  
  //if (UImode) {
    
      int16_t tmp = LR_encoderdata[RIGHT];
       
      if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); }  
     
      /* parameters: */
      if (encoder[RIGHT].pos() != asr_params[MENU_CURRENT]) {    // params changed?
          int16_t tmp2, tmp3; 
          tmp2 = param_limits[MENU_CURRENT][0]; // lower limit
          tmp3 = param_limits[MENU_CURRENT][1]; // upper limit
           
          if (tmp < tmp2)      { asr_display_params[MENU_CURRENT] = asr_params[MENU_CURRENT] = tmp2; encoder[RIGHT].setPos(tmp2);}
          else if (tmp > tmp3) { asr_display_params[MENU_CURRENT] = asr_params[MENU_CURRENT] = tmp3; encoder[RIGHT].setPos(tmp3);}
          else                 { asr_display_params[MENU_CURRENT] = asr_params[MENU_CURRENT] = tmp;  }
          MENU_REDRAW = 1;
         
      }
  
     if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); }   
     
     /* scale select: */   
     tmp = LR_encoderdata[LEFT]>>SLOWDOWN; 
     if (tmp != asr_params[0]) {
           if (tmp >= MAXSCALES)  { SCALE_SEL = 0; encoder[LEFT].setPos(0);}
           else if (tmp < 0)      { SCALE_SEL = MAXSCALES-1; encoder[LEFT].setPos((MAXSCALES-1)<<SLOWDOWN);}
           else SCALE_SEL = tmp;
           MENU_REDRAW = 1;
           if (LAST_SCALE != SCALE_SEL) SCALE_CHANGE = 1;
           LAST_SCALE = SCALE_SEL;
     }
     else  SCALE_SEL = tmp; 
  //} 
}


/* --- read  ADC ------ */

void CV() {
  
      cvval[0] = (analogRead(CV1));           // sample in
      cvval[1] = (analogRead(CV2) >> 5) - 63; // index -64/64 
      cvval[2] = (analogRead(CV3) >> 8) - 7;  // # notes -8/8
      cvval[3] = (analogRead(CV4) >> 9) - 3;  // octave offset
      _ADC = false;        
}


/* --- check buttons --- */

void buttons(uint8_t _button) {
  
  switch(_button) {
    
    
    case 0: { 
      
         if (!digitalReadFast(but_top) && (millis() - LAST_BUT > DEBOUNCE)) { 
           topButton();
           LAST_BUT = millis();   
         } 
    }
    
    case 1: { 
      
         if (!digitalReadFast(but_bot) && (millis() - LAST_BUT > DEBOUNCE)) {
           lowerButton();
           LAST_BUT = millis();
         }
    }
    
    case 2: {
      
         if (!digitalReadFast(butL) && (millis() - LAST_BUT > DEBOUNCE)) {
           leftButton(); //
           LAST_BUT = millis();   
         }
    } 
   
    case 3: {
     
         if (!digitalReadFast(butR) && (millis() - LAST_BUT > DEBOUNCE)) {
           rightButton(); //
           LAST_BUT = millis();
         }      
    }  
  } 
}
 
 
/* --------------- button misc ------------------- */

void rightButton() { 
  
  if (UImode) update_menu();
  LAST_UI = millis();
  UImode = MENU;

} 

/* -----------------------------------------------  */

void leftButton() {
  
  if (UImode) update_scale();
  LAST_UI = millis();
  UImode = MENU;
} 

/* -----------------------------------------------  */

void topButton() {
 
   //if (UImode) { 
         int8_t tmp = asr_params[1];
         tmp++;
         if (tmp > 3) tmp = 3; 
         asr_params[1] = tmp; 
   //   }
   //LAST_UI = millis();
   //UImode = MENU;
   
}

/* -----------------------------------------------  */

void lowerButton() {

  // if (UImode) { 
         int8_t tmp = asr_params[1];
         tmp--;
         if (tmp < -3) tmp = -3; 
         asr_params[1] = tmp; 
   //    }
   //LAST_UI = millis();
   //UImode = MENU;
   
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

 

