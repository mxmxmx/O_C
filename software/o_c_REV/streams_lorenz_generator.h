// Copyright 2014 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Lorenz system.

#ifndef STREAMS_LORENZ_GENERATOR_H_
#define STREAMS_LORENZ_GENERATOR_H_

#include "util/util_macros.h"
// #include "stmlib/stmlib.h"
// #include "streams/meta_parameters.h"

namespace streams {

const size_t kNumChannels = 4;

class LorenzGenerator {
 public:
  LorenzGenerator() { }
  ~LorenzGenerator() { }
  
  void Init();
  
  void Process(int32_t freq1, int32_t freq2, bool reset1, bool reset2);
 
  void set_index(uint8_t index) {
    index_ = index;
  }

  inline void set_sigma(uint16_t sigma) {
    sigma_ = (double)sigma * (1 << 24);
  }

  inline void set_rho(uint16_t rho) {
    rho_ = (double)rho * (1 << 24);
  }

  inline void set_beta(uint16_t beta) {
    beta_ = (double)beta  / 3.0 * (1 << 24);
  }
  
 inline const uint16_t dac_code(uint8_t index) const {
    return dac_code_[index];
  }

 private:
  int32_t x1_, y1_, z1_;
  int32_t rate1_;
  int32_t x2_, y2_, z2_;
  int32_t rate2_;

  int64_t sigma_, rho_, beta_ ;
  
  // O+C
  uint16_t dac_code_[kNumChannels];
 
  uint8_t index_;
  
  DISALLOW_COPY_AND_ASSIGN(LorenzGenerator);
};

}  // namespace streams

#endif  // STREAMS_LORENZ_GENERATOR_H_
