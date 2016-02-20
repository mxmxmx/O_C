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

#include "harmonograph.h"

#include "streams_resources.h"

#include <cmath>
#define _USE_MATH_DEFINES

namespace streams {

// using namespace stmlib;

// const int64_t sigma = 10.0 * (1 << 24);
//const int64_t rho = 28.0 * (1 << 24);
// const int64_t beta = 8.0 / 3.0 * (1 << 24);

//  xt = function(t) exp(-d[1]*t)*sin(t*f[1]+p[1])+exp(-d[2]*t)*sin(t*f[2]+p[2])
//  yt = function(t) exp(-d[3]*t)*sin(t*f[3]+p[3])+exp(-d[4]*t)*sin(t*f[4]+p[4])

void HarmonoGraph::Init() {
  t_ = 0 ;
  
  d1_ = 0.1 * (1 << 24);
  f1_ = 0;
  p1_ = 0;
}

void HarmonoGraph::Process(
    int32_t freq,
    bool reset) {
  int32_t rate = rate_ + (freq >> 8);
  if (rate < 0) rate = 0;
  if (rate > 255) rate = 255;

  if (reset) Init() ; 

  int64_t dt = static_cast<int64_t>(lut_lorenz_rate[rate] >> 5);
  t_ += dt ;
  float_t xt = exp(-d1_ * t_) * sin(t_ * f1_ + p1_) + exp(-d2_ * t_) * sin(t_ * f2_ + p2_) ;

  dac_code_[0] = xt;
  dac_code_[1] = xt;
  dac_code_[2] = xt;
  dac_code_[3] = xt;

}

}  // namespace streams
