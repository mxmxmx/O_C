// Copyright 2014 Olivier Gillet.
// Copyright 2016 Tim Churches.
//
// Original Author: Olivier Gillet (ol.gillet@gmail.com)
// Modifications for use in Ornament + Crime: Tim Churches (tim.churches@gmail.com)
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
// Lorenz and Rössler systems.

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
  ROSSLER_OUTPUT_X1,
  ROSSLER_OUTPUT_Y1,
  ROSSLER_OUTPUT_Z1,
  ROSSLER_OUTPUT_X2,
  ROSSLER_OUTPUT_Y2,
  ROSSLER_OUTPUT_Z2,
  LORENZ_OUTPUT_LX1_PLUS_RX1,
  LORENZ_OUTPUT_LX1_PLUS_RZ1,
  LORENZ_OUTPUT_LX1_PLUS_LY2,
  LORENZ_OUTPUT_LX1_PLUS_LZ2,
  LORENZ_OUTPUT_LX1_PLUS_RX2,
  LORENZ_OUTPUT_LX1_PLUS_RZ2,
  LORENZ_OUTPUT_LAST,
};

// using namespace stmlib;

const int64_t sigma = 10.0 * (1 << 24);
//const int64_t rho = 28.0 * (1 << 24);
const int64_t beta = 8.0 / 3.0 * (1 << 24);

// Rossler constants
const int64_t a = 0.1 * (1 << 24);
const int64_t b = 0.1 * (1 << 24);
// const int64_t c = 13.0 * (1 << 24);

void LorenzGenerator::Init(uint8_t index) {
  if (index) {
    Lx2_ = 0.1 * (1 << 24);
    Ly2_ = 0;
    Lz2_ = 0;
    Rx2_ = 0.1 * (1 << 24);
    Ry2_ = 0;
    Rz2_ = 0;
  } else {
    Lx1_ = 0.1 * (1 << 24);
    Ly1_ = 0;
    Lz1_ = 0;
    Rx1_ = 0.1 * (1 << 24);
    Ry1_ = 0;
    Rz1_ = 0;
  }
}

void LorenzGenerator::Process(
    int32_t freq1,
    int32_t freq2,
    bool reset1,
    bool reset2) {
  int32_t rate1 = rate1_ + (freq1 >> 8);
  if (rate1 < 0) rate1 = 0;
  if (rate1 > 255) rate1 = 255;
  int32_t rate2 = rate2_ + (freq2 >> 8);
  if (rate2 < 0) rate2 = 0;
  if (rate2 > 255) rate2 = 255;

  if (reset1) Init(0) ;
  if (reset2) Init(1) ; 

  // Lorenz 1
  int64_t Ldt1 = static_cast<int64_t>(lut_lorenz_rate[rate1] >> 5);
  int32_t Lx1 = Lx1_ + (Ldt1 * ((sigma * (Ly1_ - Lx1_)) >> 24) >> 24);
  int32_t Ly1 = Ly1_ + (Ldt1 * ((Lx1_ * (rho1_ - Lz1_) >> 24) - Ly1_) >> 24);
  int32_t Lz1 = Lz1_ + (Ldt1 * ((Lx1_ * int64_t(Ly1_) >> 24) - (beta * Lz1_ >> 24)) >> 24); 
  Lx1_ = Lx1;
  Ly1_ = Ly1;
  Lz1_ = Lz1; 
  int32_t Lz1_scaled = (Lz1 >> 14);
  int32_t Lx1_scaled = (Lx1 >> 14) + 32769;
  int32_t Ly1_scaled = (Ly1 >> 14) + 32769;
  // Rossler 1
  int64_t Rdt1 = static_cast<int64_t>(lut_lorenz_rate[rate1] >> 0);
  int32_t Rx1 = Rx1_ + ((Rdt1 * (-Ry1_ - Rz1_ )) >> 24);
  int32_t Ry1 = Ry1_ + ((Rdt1 * (Rx1_ + ((a * Ry1_) >> 24))) >> 24);
  int32_t Rz1 = Rz1_ + ((Rdt1 * (b + ((Rz1_ * (Rx1_ - c1_)) >> 24)))  >> 24);
  Rx1_ = Rx1;
  Ry1_ = Ry1;
  Rz1_ = Rz1; 
  int32_t Rz1_scaled = (Rz1 >> 14);
  int32_t Rx1_scaled = (Rx1 >> 14) + 32769;
  int32_t Ry1_scaled = (Ry1 >> 14) + 32769;

  // Lorenz 2
  int64_t Ldt2 = static_cast<int64_t>(lut_lorenz_rate[rate2] >> 5);
  int32_t Lx2 = Lx2_ + (Ldt2 * ((sigma * (Ly2_ - Lx2_)) >> 24) >> 24);
  int32_t Ly2 = Ly2_ + (Ldt2 * ((Lx2_ * (rho2_ - Lz2_) >> 24) - Ly2_) >> 24);
  int32_t Lz2 = Lz2_ + (Ldt2 * ((Lx2_ * int64_t(Ly2_) >> 24) - (beta * Lz2_ >> 24)) >> 24); 
  Lx2_ = Lx2;
  Ly2_ = Ly2;
  Lz2_ = Lz2; 
  int32_t Lz2_scaled = (Lz2 >> 14);
  int32_t Lx2_scaled = (Lx2 >> 14) + 32769;
  int32_t Ly2_scaled = (Ly2 >> 14) + 32769;
  // Rossler 2
  int64_t Rdt2 = static_cast<int64_t>(lut_lorenz_rate[rate2] >> 0);
  int32_t Rx2 = Rx2_ + ((Rdt2 * (-Ry2_ - Rz2_ )) >> 24);
  int32_t Ry2 = Ry2_ + ((Rdt2 * (Rx2_ + ((a * Ry2_) >> 24))) >> 24);
  int32_t Rz2 = Rz2_ + ((Rdt2 * (b + ((Rz2_ * (Rx2_ - c2_)) >> 24)))  >> 24);
  Rx2_ = Rx2;
  Ry2_ = Ry2;
  Rz2_ = Rz2; 
  int32_t Rz2_scaled = (Rz2 >> 14);
  int32_t Rx2_scaled = (Rx2 >> 14) + 32769;
  int32_t Ry2_scaled = (Ry2 >> 14) + 32769;

  uint8_t out_channel ;
  
  for (uint8_t i = 0; i < 4; ++i) {
    switch(i) {
      case 0:
        out_channel = out_a_ ;
        break ;
      case 1:
        out_channel = out_b_ ;
        break ;
      case 2:
        out_channel = out_c_ ;
        break ;
      case 3:
        out_channel = out_d_ ;
        break ; 
      default:
        // shut up compiler warning: 'out_channel' may be used uninitialized
        out_channel = LORENZ_OUTPUT_LAST; 
        break ;       
    }
 
    switch (out_channel) {
      case LORENZ_OUTPUT_X1:
        dac_code_[i] = Lx1_scaled;
        break;
      case LORENZ_OUTPUT_Y1:
        dac_code_[i] = Ly1_scaled;
        break;
      case LORENZ_OUTPUT_Z1:
        dac_code_[i] = Lz1_scaled;
        break;
      case LORENZ_OUTPUT_X2:
        dac_code_[i] = Lx2_scaled;
        break;
      case LORENZ_OUTPUT_Y2:
        dac_code_[i] = Ly2_scaled;
        break;
      case LORENZ_OUTPUT_Z2:
        dac_code_[i] = Lz2_scaled;
        break;
      case ROSSLER_OUTPUT_X1:
        dac_code_[i] = Rx1_scaled;
        break;
      case ROSSLER_OUTPUT_Y1:
        dac_code_[i] = Ry1_scaled;
        break;
      case ROSSLER_OUTPUT_Z1:
        dac_code_[i] = Rz1_scaled;
        break;
      case ROSSLER_OUTPUT_X2:
        dac_code_[i] = Rx2_scaled;
        break;
      case ROSSLER_OUTPUT_Y2:
        dac_code_[i] = Ry2_scaled;
        break;
      case ROSSLER_OUTPUT_Z2:
        dac_code_[i] = Rz2_scaled;
        break;
      case LORENZ_OUTPUT_LX1_PLUS_RX1:
        dac_code_[i] = (Lx1_scaled + Rx1_scaled) >> 1;
        break;
      case LORENZ_OUTPUT_LX1_PLUS_RZ1:
        dac_code_[i] = (Lx1_scaled + Rz1_scaled) >> 1;
        break;
      case LORENZ_OUTPUT_LX1_PLUS_LY2:
        dac_code_[i] = (Lx1_scaled + Ly2_scaled) >> 1;
        break;
      case LORENZ_OUTPUT_LX1_PLUS_LZ2:
        dac_code_[i] = (Lx1_scaled + Lz2_scaled) >> 1;
        break;
      case LORENZ_OUTPUT_LX1_PLUS_RX2:
        dac_code_[i] = (Lx1_scaled + Rx2_scaled) >> 1;
        break;
      case LORENZ_OUTPUT_LX1_PLUS_RZ2:
        dac_code_[i] = (Lx1_scaled + Rz2_scaled) >> 1;
        break;
       default:
        break;
    }
  }

}

}  // namespace streams
