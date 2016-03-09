// Copyright 2013 Olivier Gillet.
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
// Multistage envelope.

#include "peaks_multistage_envelope.h"
#include "extern/stmlib_utils_dsp.h"
#include "peaks_resources.h"

namespace peaks {

using namespace stmlib;

void MultistageEnvelope::Init() {
  set_adsr(0, 8192, 16384, 32767);
  segment_ = num_segments_;
  phase_ = 0;
  phase_increment_ = 0;
  start_value_ = 0;
  value_ = 0;
  hard_reset_ = false;
  attack_shape_ = ENV_SHAPE_QUARTIC;
  decay_shape_ = ENV_SHAPE_EXPONENTIAL;
  release_shape_ = ENV_SHAPE_EXPONENTIAL;
  
}

int16_t MultistageEnvelope::ProcessSingleSample(uint8_t control) {
  if (control & CONTROL_GATE_RISING) {
    start_value_ = (segment_ == num_segments_ || hard_reset_)
        ? level_[0]
        : value_;
    segment_ = 0;
    phase_ = 0;
  } else if (control & CONTROL_GATE_FALLING && sustain_point_) {
    start_value_ = value_;
    segment_ = sustain_point_;
    phase_ = 0;
  } else if (phase_ < phase_increment_) {
    start_value_ = level_[segment_ + 1];
    ++segment_;
    phase_ = 0;
    if (segment_ == loop_end_) {
      segment_ = loop_start_;
    }
  }
  
  bool done = segment_ == num_segments_;
  bool sustained = sustain_point_ && segment_ == sustain_point_ &&
      control & CONTROL_GATE;

  phase_increment_ =
      sustained || done ? 0 : lut_env_increments[time_[segment_] >> 8];

  int32_t a = start_value_;
  int32_t b = level_[segment_ + 1];
  uint16_t t = Interpolate824(
      lookup_table_table[LUT_ENV_LINEAR + shape_[segment_]], phase_);
  value_ = a + ((b - a) * (t >> 1) >> 15);
  phase_ += phase_increment_;
  return value_;
}

size_t MultistageEnvelope::render_preview(
    int16_t *values,
    uint32_t *segment_starts,
    uint32_t *loops,
    uint32_t &current_phase) const {
  size_t points = 0;
  uint16_t num_segments = num_segments_;
  int32_t start_value = level_[0];
  bool loop_end = true;
  for (uint16_t segment = 0; segment < num_segments; ++segment) {

    if (loop_end_ && segment == loop_start_) {
      *loops++ = points;
      loop_end = false;
    }

    if (sustain_point_ && segment == sustain_point_) {
      *segment_starts++ = points;
      size_t width = 16;
      while (--width) {
        *values++ = start_value;
        ++points;
      }
    }

    uint32_t width = 1 + (time_[segment]>>11);
    uint32_t phase = 0;
    uint32_t phase_increment = (0xff << 24) / width;

    if (segment == segment_) {
      current_phase = points + ((phase_ >> 24) * width / 256);
    }

    int32_t a = start_value;
    int32_t b = level_[segment + 1];
    *segment_starts++ = points;
    while (width--) {
      uint16_t t = Interpolate824(
          lookup_table_table[LUT_ENV_LINEAR + shape_[segment]], phase);

      *values++ = a + ((b - a) * (t >> 1) >> 15);
      ++points;
      phase += phase_increment;
    }
    start_value = b;
    if (loop_end_ && segment == loop_end_) {
      *loops++ = points;
      loop_end = true;
    }
  }
  if (!loop_end)
    *loops++ = points;

  *segment_starts = 0xffffffff;
  *loops = 0xffffffff;
  return points;
}

}  // namespace peaks
