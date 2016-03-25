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

#ifndef UI_ENCODER_H_
#define UI_ENCODER_H_

#include "../util/util_macros.h"

namespace UI {

template <uint8_t PINA, uint8_t PINB, bool acceleration_enabled = false>
class Encoder {
public:

  static const int32_t kAccelerationDec = 16;
  static const int32_t kAccelerationInc = 208;
  static const int32_t kAccelerationMax = 16 << 8;

  // For debouncing of pins, use 0x0f (b00001111) and 0x0c (b00001100) etc.
  static const uint8_t kPinMask = 0x03;
  static const uint8_t kPinEdge = 0x02;

  Encoder() { }
  ~Encoder() { }

  void Init(uint8_t pin_mode) {
    pinMode(PINA, pin_mode);
    pinMode(PINB, pin_mode);

    acceleration_enabled_ = acceleration_enabled;
    last_dir_ = 0;
    acceleration_ = 0;
    pin_state_[0] = pin_state_[1] = 0xff;
  }

  void Poll() {
    pin_state_[0] = (pin_state_[0] << 1) | digitalReadFast(PINA);
    pin_state_[1] = (pin_state_[1] << 1) | digitalReadFast(PINB);
  }

  void enable_acceleration(bool b) {
    if (b != acceleration_enabled_) {
      acceleration_enabled_ = b;
      acceleration_ = 0;
    }
  }

  inline int32_t Read() {

    int32_t acceleration = acceleration_;
    if (acceleration_enabled_ && acceleration) {
      acceleration -= kAccelerationDec;
      if (acceleration < 0)
        acceleration = 0;
    }

    // Find direction by detecting state change and evaluating the other pin.
    // 0x02 == b10 == rising edge on pin
    // Should also work just checking for falling edge on PINA and checking
    // PINB state.
    int32_t i = 0;
    const uint8_t a = pin_state_[0] & kPinMask;
    const uint8_t b = pin_state_[1] & kPinMask;
    if (a == kPinEdge && b == 0x00) {
      i = 1;
    } else if (b == kPinEdge && a == 0x00) {
      i = -1;
    }

    if (i) {
      if (acceleration_enabled_) {
        if (i != last_dir_) {
          // We've stored the pre-acceleration value so don't need to actually check the signs.
          // 1001 ways to check if sign bit is different are left as an exercise for the reader ;)
          acceleration = 0;
        } else {
          acceleration += kAccelerationInc;
          if (acceleration > kAccelerationMax)
            acceleration = kAccelerationMax;
        }
      } else {
        acceleration = 0;
      }

      last_dir_ = i;
      i += i * (acceleration >> 8);
    }

    acceleration_ = acceleration;
    return i;
  }

private:
  bool acceleration_enabled_;
  int32_t last_dir_;
  int32_t acceleration_;
  uint8_t pin_state_[2];

  DISALLOW_COPY_AND_ASSIGN(Encoder);
};

}; // namespace UI

#endif // UI_ENCODER_H_
