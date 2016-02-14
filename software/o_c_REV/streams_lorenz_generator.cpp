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

#include "streams_lorenz_generator.h"

#include "streams_resources.h"

namespace streams {

// using namespace stmlib;

//const int64_t sigma = 10.0 * (1 << 24);
//const int64_t rho = 28.0 * (1 << 24);
//const int64_t beta = 8.0 / 3.0 * (1 << 24);

void LorenzGenerator::Init() {
  x_ = 0.1 * (1 << 24);
  y_ = 0;
  z_ = 0;
}

void LorenzGenerator::Process(
    int32_t freq,
    bool reset) {
  int32_t rate = rate_ + (freq >> 8);
  if (rate < 0) rate = 0;
  if (rate > 255) rate = 255;

  if (reset) Init() ;

  int64_t dt = static_cast<int64_t>(lut_lorenz_rate[rate] >> 5);
  
  int32_t x = x_ + (dt * ((sigma_ * (y_ - x_)) >> 24) >> 24);
  int32_t y = y_ + (dt * ((x_ * (rho_ - z_) >> 24) - y_) >> 24);
  int32_t z = z_ + (dt * ((x_ * int64_t(y_) >> 24) - (beta_ * z_ >> 24)) >> 24);
  
  x_ = x;
  y_ = y;
  z_ = z;
  
  int32_t z_scaled = (z >> 14);
  int32_t x_scaled = (x >> 14) + 32769;
  int32_t y_scaled = (y >> 14) + 32769;

  dac_code_[0] = x_scaled;
  dac_code_[1] = y_scaled;
  dac_code_[2] = z_scaled;
  // just make channel D the mean of the others for now
  dac_code_[3] = ((x_scaled + y_scaled + z_scaled) * 10) >> 5 ;

}

}  // namespace streams
