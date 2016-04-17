// Copyright (c) 2016 Tim Churches
// Utilising some code from https://github.com/xaocdevices/batumi/blob/alternate/lfo.cc
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

#ifndef UTIL_LOGISTIC_MAP_H_
#define UTIL_LOGISTIC_MAP_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

namespace util {

class LogisticMap {
public:
  static const int64_t kONE = 1 << 24;
  static const int64_t kDefaultSeed = 1 << 16; // 1.0 = 1 << 24
  static const int64_t kDefaultR = 3.6 * kONE;

  void Init() {
    x_ = seed_ = kDefaultSeed;
    r_ = kDefaultR;
  }

  int64_t Clock() {
    next_x_ = (r_ * ((x_ * (kONE - x_)) >> 24)) >> 24;
    x_ = next_x_;
    return x_ ;
  }

  void set_seed(uint8_t seed) {
    if (seed_ != seed << 16) {
      seed_ = seed << 16 ; // seed range 1 to 255 = 1/256 to 1
      x_ = seed_;
    }
  }

  void set_r(uint8_t r) {
    r_ = ((3 << 8) + r) << 16;  // r needs to be between 3.0 and < 4.0
  }

  uint32_t get_register() const {
    return x_;
  }

private:
  int64_t seed_;
  int64_t r_;
  int64_t x_;
  int64_t next_x_;
};

}; // namespace util

#endif // UTIL_LOGISTIC_MAP_H_
