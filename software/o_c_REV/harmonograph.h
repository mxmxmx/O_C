// Copyright 2014 Olivier Gillet.
// Copyright 2016 Tim Churches
//
// Original Author: Olivier Gillet (ol.gillet@gmail.com)
// Modifications for use of this code in firmare for the Ornaments and Crime modue:
// Tim Churches (tim.churches@gmail.com)
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
// 4-dimensional harmonograph.

#ifndef STREAMS_HARMONOGRAPH_H_
#define STREAMS_HARMONOGRAPH_H_

#include "util/util_macros.h"
// #include "stmlib/stmlib.h"
// #include "streams/meta_parameters.h"

namespace streams {

const size_t kNumHgChannels = 4;

class HarmonoGraph {
 public:
  HarmonoGraph() { }
  ~HarmonoGraph() { }
  
  void Init();
  
  void Process(int32_t freq, bool reset);
 
  void set_index(uint8_t index) {
    index_ = index;
  }

 
//   inline void set_rho1(int16_t rho) {
//     // rho1_ = (double)rho * (1 << 24);
//     rho1_ = (double)rho * (1 << 16);
//   }
// 
//   inline void set_rho2(int16_t rho) {
//     // rho2_ = (double)rho * (1 << 24);
//     rho2_ = (double)rho * (1 << 16);
//   }
// 
//   inline void set_out_c(uint8_t out_c) {
//     out_c_ = out_c;
//   }
// 
//   inline void set_out_d(uint8_t out_d) {
//     out_d_ = out_d;
//   }
 
 inline const uint16_t dac_code(uint8_t index) const {
    return dac_code_[index];
  }

 private:
  double t_, p1_, f1_, d1_, p2_, f2_, d2_ ;
  int32_t rate_;

  // O+C
  uint16_t dac_code_[kNumHgChannels];
 
  uint8_t index_;
  
  DISALLOW_COPY_AND_ASSIGN(HarmonoGraph);
};

}  // namespace streams

#endif  // STREAMS_HARMONOGRAPH_H_
