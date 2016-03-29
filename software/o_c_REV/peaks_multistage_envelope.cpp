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
#include "util/util_misc.h"

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
    if (segment_ == loop_end_ && (control & CONTROL_GATE)) {
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

uint16_t MultistageEnvelope::RenderPreview(
    int16_t *values,
    uint16_t *segment_start_points,
    uint16_t *loop_points,
    uint16_t &current_phase) const {

  const uint16_t num_segments = num_segments_;
  const uint16_t current_segment = segment_;
  const uint16_t sustain_point = sustain_point_;
  const uint16_t sustain_index = sustain_index_;
  const uint32_t phase = phase_;
  int32_t start_value = level_[0];

  const uint16_t segment_width = sustain_point
    ? (2 * kPreviewWidth) / (2 * num_segments + 1)
    : kPreviewWidth / num_segments;

  int16_t current_pos = 0;
  uint16_t segment;
  for (segment = 0; segment < num_segments; ++segment) {

    if (loop_end_ && segment == loop_start_)
      *loop_points++ = current_pos;

    if (sustain_point && segment == sustain_point) {
      // Sustain points are half as wide as normal segments
      *segment_start_points++ = current_pos;
      uint16_t w = segment_width / 2;
      current_pos += w;
      while (w--)
        *values++ = start_value;
    } else if (sustain_index && segment == sustain_index) {
      // While this isn't strictly a segment, it's used for visualization
      *segment_start_points++ = current_pos;
    }

    *segment_start_points++ = current_pos;
    uint32_t w = time_[segment] * segment_width >> 16;
    if (w < 1) w = 1;
    if (segment == current_segment)
      current_phase = current_pos + (((phase >> 24) * w) / 256);
    current_pos += w;

    uint32_t phase = 0, phase_increment = (0xff << 24) / w;
    int32_t a = start_value;
    int32_t b = level_[segment + 1];
    while (w--) {
      uint16_t t = Interpolate824(
          lookup_table_table[LUT_ENV_LINEAR + shape_[segment]], phase);
      *values++ = a + ((b - a) * (t >> 1) >> 15);
      phase += phase_increment;
    }
    start_value = b;
  }
  // Current setups loop at num_segments_
  if (loop_end_ && segment == loop_end_)
    *loop_points++ = current_pos;

  *segment_start_points++ = 0xffff;
  *loop_points++ = 0xffff;

  return current_pos;
}

}  // namespace peaks
