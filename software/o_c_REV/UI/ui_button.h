#ifndef UTIL_BUTTON_H_
#define UTIL_BUTTON_H_

#include <Arduino.h>
#include "../util/util_macros.h"

namespace UI {

// Basic button/switch wrapper that has some debouncing
class Button {
public:
  Button() { }

  void Init(uint8_t pin, uint8_t pin_mode) {
    pin_ = pin;
    pinMode(pin, pin_mode);
    state_ = 0xff;
  }

  void Poll() {
    state_ = (state_ << 1) | digitalReadFast(pin_);
  }

  inline bool pressed() const {
    return state_ == 0x00;
  }

  inline bool just_pressed() const {
    return state_ == 0x80;
  }

  inline bool released() const {
    return state_ == 0x7f;
  }

  bool read_immediate() const {
    return !digitalReadFast(pin_);
  }

private:
  int pin_;
  uint8_t state_;

  DISALLOW_COPY_AND_ASSIGN(Button);
};

}; // namespace UI

#endif // UTIL_BUTTON_H_
