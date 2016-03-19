// Copyright 2013 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
// Modifications for use in Ornaments & Crimes: Tim Churches (tim.churches@gmail.com)
// Modifications copyright 2016 Tim Churches.
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
// Bouncing ball.

#ifndef PEAKS_MODULATIONS_BOUNCING_BALL_H_
#define PEAKS_MODULATIONS_BOUNCING_BALL_H_

// #include "stmlib/stmlib.h"

// #include <algorithm>

#include <stdint.h>
#include "util/util_macros.h"

#include "extern/stmlib_utils_dsp.h"
#include "peaks_resources.h"
#include "peaks_gate_processor.h"

namespace peaks {

class BouncingBall {
 public:
  BouncingBall() { }
  ~BouncingBall() { }
  
  void Init() {
    initial_amplitude_ = 65535L << 14;
    gravity_ = 40;
    bounce_loss_ = 4095;
    initial_velocity_ = 0;
  }
    
  void Configure(int32_t* parameter) {
    ControlMode control_mode = CONTROL_MODE_FULL;
    if (control_mode == CONTROL_MODE_HALF) {
      set_initial_amplitude(65535);
      set_initial_velocity(0);
      set_gravity(parameter[0]);
      set_bounce_loss(parameter[1]);
    } else {
      set_gravity(65535 - parameter[0]);
      set_bounce_loss(parameter[1]);
      set_initial_amplitude(parameter[2]);
      set_initial_velocity(parameter[3] - 32768);
    }
  }
  
  inline int16_t ProcessSingleSample(uint8_t control) {
    uint16_t const kMaxPos = 32767L ;
    if (control & CONTROL_GATE_RISING) {
      if (hard_reset_) {
        velocity_ = initial_velocity_;
        position_ = initial_amplitude_;
      } else {
        velocity_ += initial_velocity_ ;
        if (velocity_ > initial_velocity_) {
          velocity_ = initial_velocity_ ;
        }
      }
    }
    velocity_ -= gravity_;
    position_ += velocity_;
    if (position_ < 0) {
      position_ = 0;
      velocity_ = -(velocity_ >> 12) * bounce_loss_;
    }
    if (position_ > (kMaxPos << 15)) {
      position_ = kMaxPos << 15;
      velocity_ = -(velocity_ >> 12) * bounce_loss_;
    }
    return position_ >> 15; // was 15
  }
  
  inline void set_gravity(uint16_t gravity) {
    gravity_ = stmlib::Interpolate88(lut_gravity, gravity);
  }
  
  inline void set_bounce_loss(uint16_t bounce_loss) {
    uint32_t b = bounce_loss;
    b = b * b >> 16;
    bounce_loss_ = 4095 - (b >> 4); // was 4
  }

  inline void set_initial_amplitude(uint16_t initial_amplitude) {
    initial_amplitude_ = static_cast<int32_t>(initial_amplitude) << 14;
  }
  
  inline void set_initial_velocity(int16_t initial_velocity) {
    initial_velocity_ = static_cast<int32_t>(initial_velocity) << 4; // was 4
  }

  inline void set_hard_reset(bool hard_reset) {
    hard_reset_ = hard_reset;
  }
  
  inline bool FillBuffer() const {
    return true;
  }
  
 private:
  int32_t gravity_;
  int32_t bounce_loss_;
  int32_t initial_amplitude_;
  int32_t initial_velocity_;
   
  int32_t velocity_;
  int64_t position_; 

  bool hard_reset_;

  DISALLOW_COPY_AND_ASSIGN(BouncingBall);
};

}  // namespace peaks

#endif  // PEAKS_MODULATIONS_BOUNCING_BALL_H_
