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
// Halvorsen system.

#include "halvorsen_generator.h"

#include "streams_resources.h"

namespace streams {

enum EHalvorsenOutputMap {
  HALVORSEN_OUTPUT_X1,
  HALVORSEN_OUTPUT_Y1,
  HALVORSEN_OUTPUT_Z1,
  HALVORSEN_OUTPUT_X2,
  HALVORSEN_OUTPUT_Y2,
  HALVORSEN_OUTPUT_Z2,
  HALVORSEN_OUTPUT_X1_PLUS_Y1,
  HALVORSEN_OUTPUT_X1_PLUS_Z1,
  HALVORSEN_OUTPUT_Y1_PLUS_Z1,
  HALVORSEN_OUTPUT_X2_PLUS_Y2,
  HALVORSEN_OUTPUT_X2_PLUS_Z2,
  HALVORSEN_OUTPUT_Y2_PLUS_Z2,
  HALVORSEN_OUTPUT_X1_PLUS_X2,
  HALVORSEN_OUTPUT_X1_PLUS_Y2,
  HALVORSEN_OUTPUT_X1_PLUS_Z2,
  HALVORSEN_OUTPUT_Y1_PLUS_X2,
  HALVORSEN_OUTPUT_Y1_PLUS_Y2,
  HALVORSEN_OUTPUT_Y1_PLUS_Z2,
  HALVORSEN_OUTPUT_Z1_PLUS_X2,
  HALVORSEN_OUTPUT_Z1_PLUS_Y2,
  HALVORSEN_OUTPUT_Z1_PLUS_Z2,
  HALVORSEN_OUTPUT_LAST,
};

// using namespace stmlib;

// const int64_t sigma = 10.0 * (1 << 24);
const int64_t alpha = 1.4 * (1 << 24);
// const int64_t beta = 8.0 / 3.0 * (1 << 24);

void HalvorsenGenerator::Init() {
  x1_ = 1;
  y1_ = 0;
  z1_ = 0;
  x2_ = x1_;
  y2_ = y1_;
  z2_ = z1_;
}

void HalvorsenGenerator::Process(
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

  // int64_t dt1 = static_cast<int64_t>(lut_lorenz_rate[rate1] >> 5);
  // int64_t dt1 = static_cast<int64_t>(lut_lorenz_rate[rate1]);
  int64_t dt1 = 1 << 21 ;
  // int32_t x1 = x1_ + (dt1 * ((-alpha * x1_ - 4 * y1_ - 4 * z1_ - y1_ * y1_) >> 24) >> 24);
  // int32_t y1 = y1_ + (dt1 * ((-alpha * y1_ - 4 * z1_ - 4 * x1_ - z1_ * z1_) >> 24) >> 24);
  // int32_t z1 = z1_ + (dt1 * ((-alpha * z1_ - 4 * x1_ - 4 * y1_ - x1_ * x1_) >> 24) >> 24);
  int32_t x1 = x1_ + (dt1 * ((-alpha * x1_ - 4 * y1_ - 4 * z1_ - y1_ * y1_) >> 24));
  int32_t y1 = y1_ + (dt1 * ((-alpha * y1_ - 4 * z1_ - 4 * x1_ - z1_ * z1_) >> 24));
  int32_t z1 = z1_ + (dt1 * ((-alpha * z1_ - 4 * x1_ - 4 * y1_ - x1_ * x1_) >> 24));
  x1_ = x1;
  y1_ = y1;
  z1_ = z1; 
  int32_t z1_scaled = (z1 >> 14);
  int32_t x1_scaled = (x1 >> 14) + 32769;
  int32_t y1_scaled = (y1 >> 14) + 32769;
  // int32_t z1_scaled = (z1 >> 4);
  // int32_t x1_scaled = (x1 >> 4) + 32769;
  // int32_t y1_scaled = (y1 >> 4) + 32769;

  int64_t dt2 = static_cast<int64_t>(lut_lorenz_rate[rate2] >> 5);
  int32_t x2 = x2_ + (dt2 * ((-alpha * x2_ - 4 * y2_ - 4 * z2_ - y2_ * y2_) >> 24) >> 24);
  int32_t y2 = y2_ + (dt2 * ((-alpha * y2_ - 4 * z2_ - 4 * x2_ - z2_ * z2_) >> 24) >> 24);
  int32_t z2 = z2_ + (dt2 * ((-alpha * z2_ - 4 * x2_ - 4 * y2_ - x2_ * x2_) >> 24) >> 24);
  x2_ = x2;
  y2_ = y2;
  z2_ = z2; 
  int32_t z2_scaled = (z2 >> 14);
  int32_t x2_scaled = (x2 >> 14) + 32769;
  int32_t y2_scaled = (y2 >> 14) + 32769;

 
  dac_code_[0] = x2_scaled;
  dac_code_[1] = y2_scaled;

  for (uint8_t i = 2; i < 4; ++i) {
 
    switch (i==2 ? out_c_ : out_d_) {
      case HALVORSEN_OUTPUT_X1:
        dac_code_[i] = x1_scaled;
        break;
      case HALVORSEN_OUTPUT_Y1:
        dac_code_[i] = y1_scaled;
        break;
      case HALVORSEN_OUTPUT_Z1:
        dac_code_[i] = z1_scaled;
        break;
      case HALVORSEN_OUTPUT_X2:
        dac_code_[i] = x2_scaled;
        break;
      case HALVORSEN_OUTPUT_Y2:
        dac_code_[i] = y2_scaled;
        break;
      case HALVORSEN_OUTPUT_Z2:
        dac_code_[i] = z2_scaled;
        break;
      case HALVORSEN_OUTPUT_X1_PLUS_Y1:
        dac_code_[i] = (x1_scaled + y1_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_X1_PLUS_Z1:
        dac_code_[i] = (x1_scaled + z1_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_Y1_PLUS_Z1:
        dac_code_[i] = (y1_scaled + z1_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_X2_PLUS_Y2:
        dac_code_[i] = (x2_scaled + y2_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_X2_PLUS_Z2:
        dac_code_[i] = (x2_scaled + z2_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_Y2_PLUS_Z2:
        dac_code_[i] = (y2_scaled + z2_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_X1_PLUS_X2:
        dac_code_[i] = (x1_scaled + x2_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_X1_PLUS_Y2:
        dac_code_[i] = (x1_scaled + y2_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_X1_PLUS_Z2:
        dac_code_[i] = (x1_scaled + z2_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_Y1_PLUS_X2:
        dac_code_[i] = (y1_scaled + x2_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_Y1_PLUS_Y2:
        dac_code_[i] = (y1_scaled + y2_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_Y1_PLUS_Z2:
        dac_code_[i] = (y1_scaled + z2_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_Z1_PLUS_X2:
        dac_code_[i] = (z1_scaled + x2_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_Z1_PLUS_Y2:
        dac_code_[i] = (z1_scaled + y2_scaled) >> 1;
        break;
      case HALVORSEN_OUTPUT_Z1_PLUS_Z2:
        dac_code_[i] = (z1_scaled + z2_scaled) >> 1;
        break;
       default:
        break;
    }
  }

//  dac_code_[0] = x1_scaled;
//  dac_code_[1] = y1_scaled;
//  dac_code_[2] = x2_scaled;
//  dac_code_[3] = y2_scaled;

}

}  // namespace streams
