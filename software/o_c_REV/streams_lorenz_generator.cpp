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

enum ELorenzOutputMap {
  LORENZ_OUTPUT_X1,
  LORENZ_OUTPUT_Y1,
  LORENZ_OUTPUT_Z1,
  LORENZ_OUTPUT_X2,
  LORENZ_OUTPUT_Y2,
  LORENZ_OUTPUT_Z2,
  LORENZ_OUTPUT_LAST,
};

// using namespace stmlib;

//const int64_t sigma = 10.0 * (1 << 24);
//const int64_t rho = 28.0 * (1 << 24);
//const int64_t beta = 8.0 / 3.0 * (1 << 24);

void LorenzGenerator::Init() {
  x1_ = 0.1 * (1 << 24);
  y1_ = 0;
  z1_ = 0;
  x2_ = x1_;
  y2_ = y1_;
  z2_ = z1_;
}

void LorenzGenerator::Process(
    int32_t freq1,
    int32_t freq2,
    bool reset) {
  int32_t rate1 = rate1_ + (freq1 >> 8);
  if (rate1 < 0) rate1 = 0;
  if (rate1 > 255) rate1 = 255;
  int32_t rate2 = rate2_ + (freq2 >> 8);
  if (rate2 < 0) rate2 = 0;
  if (rate2 > 255) rate2 = 255;

  if (reset) Init() ; 

  int64_t dt1 = static_cast<int64_t>(lut_lorenz_rate[rate1] >> 5);
  int32_t x1 = x1_ + (dt1 * ((sigma_ * (y1_ - x1_)) >> 24) >> 24);
  int32_t y1 = y1_ + (dt1 * ((x1_ * (rho1_ - z1_) >> 24) - y1_) >> 24);
  int32_t z1 = z1_ + (dt1 * ((x1_ * int64_t(y1_) >> 24) - (beta_ * z1_ >> 24)) >> 24); 
  x1_ = x1;
  y1_ = y1;
  z1_ = z1; 
  int32_t z1_scaled = (z1 >> 14);
  int32_t x1_scaled = (x1 >> 14) + 32769;
  int32_t y1_scaled = (y1 >> 14) + 32769;

  int64_t dt2 = static_cast<int64_t>(lut_lorenz_rate[rate2] >> 5);
  int32_t x2 = x2_ + (dt2 * ((sigma_ * (y2_ - x2_)) >> 24) >> 24);
  int32_t y2 = y2_ + (dt2 * ((x2_ * (rho2_ - z2_) >> 24) - y2_) >> 24);
  int32_t z2 = z2_ + (dt2 * ((x2_ * int64_t(y2_) >> 24) - (beta_ * z2_ >> 24)) >> 24); 
  x2_ = x2;
  y2_ = y2;
  z2_ = z2; 
  int32_t z2_scaled = (z2 >> 14);
  int32_t x2_scaled = (x2 >> 14) + 32769;
  int32_t y2_scaled = (y2 >> 14) + 32769;

  switch (out_a_) {
    case LORENZ_OUTPUT_X1:
      dac_code_[0] = x1_scaled;
      break;
    case LORENZ_OUTPUT_Y1:
      dac_code_[0] = y1_scaled;
      break;
    case LORENZ_OUTPUT_Z1:
      dac_code_[0] = z1_scaled;
      break;
    case LORENZ_OUTPUT_X2:
      dac_code_[0] = x2_scaled;
      break;
    case LORENZ_OUTPUT_Y2:
      dac_code_[0] = y2_scaled;
      break;
    case LORENZ_OUTPUT_Z2:
      dac_code_[0] = z2_scaled;
      break;
    default:
      break;
  }

  switch (out_b_) {
    case LORENZ_OUTPUT_X1:
      dac_code_[1] = x1_scaled;
      break;
    case LORENZ_OUTPUT_Y1:
      dac_code_[1] = y1_scaled;
      break;
    case LORENZ_OUTPUT_Z1:
      dac_code_[1] = z1_scaled;
      break;
    case LORENZ_OUTPUT_X2:
      dac_code_[1] = x2_scaled;
      break;
    case LORENZ_OUTPUT_Y2:
      dac_code_[1] = y2_scaled;
      break;
    case LORENZ_OUTPUT_Z2:
      dac_code_[1] = z2_scaled;
      break;
    default:
      break;
  }

  switch (out_c_) {
    case LORENZ_OUTPUT_X1:
      dac_code_[2] = x1_scaled;
      break;
    case LORENZ_OUTPUT_Y1:
      dac_code_[2] = y1_scaled;
      break;
    case LORENZ_OUTPUT_Z1:
      dac_code_[2] = z1_scaled;
      break;
    case LORENZ_OUTPUT_X2:
      dac_code_[2] = x2_scaled;
      break;
    case LORENZ_OUTPUT_Y2:
      dac_code_[2] = y2_scaled;
      break;
    case LORENZ_OUTPUT_Z2:
      dac_code_[2] = z2_scaled;
      break;
    default:
      break;
  }

  switch (out_d_) {
    case LORENZ_OUTPUT_X1:
      dac_code_[3] = x1_scaled;
      break;
    case LORENZ_OUTPUT_Y1:
      dac_code_[3] = y1_scaled;
      break;
    case LORENZ_OUTPUT_Z1:
      dac_code_[3] = z1_scaled;
      break;
    case LORENZ_OUTPUT_X2:
      dac_code_[3] = x2_scaled;
      break;
    case LORENZ_OUTPUT_Y2:
      dac_code_[3] = y2_scaled;
      break;
    case LORENZ_OUTPUT_Z2:
      dac_code_[3] = z2_scaled;
      break;
    default:
      break;
  }

//  dac_code_[0] = x1_scaled;
//  dac_code_[1] = y1_scaled;
//  dac_code_[2] = x2_scaled;
//  dac_code_[3] = y2_scaled;

}

}  // namespace streams
