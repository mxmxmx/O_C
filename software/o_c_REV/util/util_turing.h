// Copyright (c) 2016 Patrick Dowling
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

#ifndef UTIL_TURING_H_
#define UTIL_TURING_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

namespace util {

class TuringShiftRegister {
public:
  static const uint8_t kMinLength = 3;
  static const uint8_t kMaxLength = 32;
  static const uint8_t kDefaultLength = 16;
  static const uint8_t kDefaultProbability = 128;

  void Init() {
    length_ = kDefaultLength;
    probability_ = kDefaultProbability;
    shift_register_ = 0xffffffff;
  }

  uint32_t Clock() {
    uint32_t shift_register = shift_register_;

    // Toggle LSB; there might be better random options
    if (255 == probability_ ||
        static_cast<uint8_t>(random(255) < probability_))
      shift_register ^= 0x1;

    uint32_t lsb_mask = 0x1 << (length_ - 1);
    if (shift_register & 0x1)
      shift_register = (shift_register >> 1) | lsb_mask;
    else
      shift_register = (shift_register >> 1) & ~lsb_mask;

    // hack... don't turn all zero ...
    if (!shift_register)
      shift_register |= (random(0x2) << (length_ - 1));

    shift_register_ = shift_register;

    return shift_register & ~(0xffffffff << length_);
  }

  void set_length(uint8_t length) {
    // hack... don't turn all zero ...
    if (length > length_) 
      shift_register_ |= (random(0x2) << length_);

    length_ = length;
  }

  uint8_t length() const {
     return length_;
  }

  void set_probability(uint8_t probability) {
    probability_ = probability;
  }

  uint32_t get_shift_register() const {
    return shift_register_;
  }

  bool get_LSB() const {
    return (shift_register_ & 1u);
  }

private:

  uint8_t length_;
  uint8_t probability_;
  uint32_t shift_register_;
};

}; // namespace util

#endif // UTIL_TURING_H_
