/*
*
* encoders + buttons + ADC stuff
*
*/

/* --------------- encoders ---------------------------------------  */

int LAST_ENCODER_VALUE[2] = {-1, -1};

void encoders() {
  _ENC = false;

  bool changed =
      (LAST_ENCODER_VALUE[LEFT] != encoder[LEFT].pos()) ||
      (LAST_ENCODER_VALUE[RIGHT] != encoder[RIGHT].pos());

  if (UI_MODE) {
    changed |= OC::current_app->update_encoders();
    LAST_ENCODER_VALUE[LEFT] = encoder[LEFT].pos();
    LAST_ENCODER_VALUE[RIGHT] = encoder[RIGHT].pos();
  } else {
    // Eat value
    encoder[LEFT].setPos(LAST_ENCODER_VALUE[LEFT]);
    encoder[RIGHT].setPos(LAST_ENCODER_VALUE[RIGHT]);
  }

  if (changed) {
    UI_MODE = UI::DISPLAY_MENU;
    MENU_REDRAW = 1;
    _UI_TIMESTAMP = millis();
  }
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
        if (UI_MODE) OC::current_app->top_button();
        button_pressed = true;
      }
    }
    break;

    case BUTTON_BOTTOM: {
      if (!digitalReadFast(but_bot) && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) {
        if (UI_MODE) OC::current_app->lower_button();
        button_pressed = true;
      }
    }
    break;

    case BUTTON_LEFT: {
      button_pressed = button_left.read();
      if (button_left.long_event() && UI_MODE) {
        if (OC::current_app->left_button_long)
          OC::current_app->left_button_long();
      } else if (button_left.event() && UI_MODE) {
          OC::current_app->left_button();
      }
    }
    break;

    case BUTTON_RIGHT: {
      button_pressed = button_right.read();
      if (button_right.long_event())
        SELECT_APP = true;
      else if (button_right.event() && UI_MODE)
        OC::current_app->right_button();
    }
    break;
  }

  if (button_pressed) {
    _BUTTONS_TIMESTAMP = millis();
    _UI_TIMESTAMP = millis();
    UI_MODE = UI::DISPLAY_MENU;
  }
}
