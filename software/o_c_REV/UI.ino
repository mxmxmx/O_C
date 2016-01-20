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

const int8_t OCT_MAX = 4;  // max offset via button (top)
const int8_t OCT_MIN = -3; // min offset via button (bottom)

enum DISPLAY_PAGE 
{  
 SCREENSAVER,
 MENU,
 CALIBRATE 
};  

/* --------------- encoders ---------------------------------------  */

/* encoders variables  */
uint8_t SLOWDOWN = 0; 

/* encoders isr */
void left_encoder_ISR() 
{
  encoder[LEFT].process();
}

void right_encoder_ISR() 
{
  encoder[RIGHT].process();
}

int LAST_ENCODER_VALUE[2] = {-1, -1};

void encoders() {
  _ENC = false;

  bool changed =
      (LAST_ENCODER_VALUE[LEFT] != encoder[LEFT].pos()) ||
      (LAST_ENCODER_VALUE[RIGHT] != encoder[RIGHT].pos());

  if (UI_MODE) {
    changed |= current_app->update_encoders();
    LAST_ENCODER_VALUE[LEFT] = encoder[LEFT].pos();
    LAST_ENCODER_VALUE[RIGHT] = encoder[RIGHT].pos();
  } else {
    // Eat value
    encoder[LEFT].setPos(LAST_ENCODER_VALUE[LEFT]);
    encoder[RIGHT].setPos(LAST_ENCODER_VALUE[RIGHT]);
  }

  if (changed) {
    UI_MODE = MENU;
    MENU_REDRAW = 1;
    _UI_TIMESTAMP = millis();
  }
}


/* Resuming app */

void ASR_resume() {
  encoder[LEFT].setPos(asr_params[0]);
  encoder[RIGHT].setPos(asr_params[MENU_CURRENT]);
  SCALE_SEL = asr_params[0];
  SCALE_CHANGE = 0;
}

/* ---------------  update params ---------------------------------  */  

bool update_ENC()  {
    
      int16_t tmp = encoder[RIGHT].pos();
    
      /* parameters: */
      if (tmp != asr_params[MENU_CURRENT]) {    // params changed?
          int16_t tmp2, tmp3; 
          tmp2 = param_limits[MENU_CURRENT][0]; // lower limit
          tmp3 = param_limits[MENU_CURRENT][1]; // upper limit
           
          if (tmp < tmp2)      { asr_display_params[MENU_CURRENT] = asr_params[MENU_CURRENT] = tmp2; encoder[RIGHT].setPos(tmp2);}
          else if (tmp > tmp3) { asr_display_params[MENU_CURRENT] = asr_params[MENU_CURRENT] = tmp3; encoder[RIGHT].setPos(tmp3);}
          else                 { asr_display_params[MENU_CURRENT] = asr_params[MENU_CURRENT] = tmp;  }
          MENU_REDRAW = 1;
          return true;
      }
  
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
             return true;
         }
     }

     return false;
}

/* --- check buttons --- */

TimerDebouncedButton<butL, 50, 2000> button_left;
TimerDebouncedButton<butR, 50, 2000> button_right;

void buttons_init() {
  button_left.init();
  button_right.init();
}

void buttons(uint8_t _button) {
  bool button_pressed = false;
  switch(_button) {
    case BUTTON_TOP: { 
      if (!digitalReadFast(but_top) && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) {
        if (UI_MODE) current_app->top_button();
        button_pressed = true;
      }
    }
    break;

    case BUTTON_BOTTOM: {
      if (!digitalReadFast(but_bot) && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) {
        if (UI_MODE) current_app->lower_button();
        button_pressed = true;
      }
    }
    break;

    case BUTTON_LEFT: {
      button_pressed = button_left.read();
      if (button_left.long_event() && UI_MODE) {
        if (current_app->left_button_long)
          current_app->left_button_long();
      } else if (button_left.event() && UI_MODE) {
          current_app->left_button();
      }
    }
    break;

    case BUTTON_RIGHT: {
      button_pressed = button_right.read();
      if (button_right.long_event())
        SELECT_APP = true;
      else if (button_right.event() && UI_MODE)
        current_app->right_button();
    }
    break;
  }

  if (button_pressed) {
    _BUTTONS_TIMESTAMP = millis();
    _UI_TIMESTAMP = millis();
    UI_MODE = MENU;
  }
}

 
/* --------------- button misc ------------------- */

void rightButton() { 
  
  update_menu();
} 

/* -----------------------------------------------  */

void leftButton() {
  
  update_scale();
} 

/* -----------------------------------------------  */

void topButton() {
 
   MENU_REDRAW = 1; 
   int8_t tmp = asr_params[1];
   tmp++;
   asr_params[1] = tmp > OCT_MAX ? OCT_MAX : tmp; 
}

/* -----------------------------------------------  */

void lowerButton() {

  MENU_REDRAW = 1; 
  int8_t tmp = asr_params[1];
  tmp--;
  asr_params[1] = tmp < OCT_MIN ? OCT_MIN : tmp; 
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
