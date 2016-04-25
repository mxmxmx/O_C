// Copyright (c) 2016 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef UTIL_BUTTON_H_
#define UTIL_BUTTON_H_

#include <Arduino.h>
#include "../util/util_macros.h"

namespace UI {

// Basic button/switch wrapper that has 7 bits of debouncing on press/release.
class Button {
public:
  Button() { }

  void Init(uint8_t pin, uint8_t pin_mode) {
    pin_ = pin;
    pinMode(pin, pin_mode);
    state_ = 0xff;
  }

  // @return True if pressed
  uint8_t Poll() {
    uint8_t state = (state_ << 1) | digitalReadFast(pin_);
    state_ = state;
    return !(state & 0x01);
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
