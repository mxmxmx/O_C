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
  attack_reset_behaviour_ = RESET_BEHAVIOUR_NULL;
  attack_falling_gate_behaviour_ = FALLING_GATE_BEHAVIOUR_IGNORE;
  decay_release_reset_behaviour_ = RESET_BEHAVIOUR_SEGMENT_PHASE;
  reset_behaviour_ = RESET_BEHAVIOUR_NULL;
  attack_shape_ = ENV_SHAPE_QUARTIC;
  decay_shape_ = ENV_SHAPE_EXPONENTIAL;
  release_shape_ = ENV_SHAPE_EXPONENTIAL;
  attack_multiplier_ = 0;
  decay_multiplier_ = 0;
  release_multiplier_ = 0;
  amplitude_ = 65535 ;
  sampled_amplitude_ = 65535 ;
  amplitude_sampled_ = false ;
  scaled_value_ = 0 ;
  max_loops_ = 0 ;
  loop_counter_ = 0;
  state_mask_ = 0;
}

uint16_t MultistageEnvelope::ProcessSingleSample(uint8_t control) {

  state_mask_ = 0;
  if (control & CONTROL_GATE_RISING) {
    if (segment_ == num_segments_) {
      start_value_ = level_[0];
      segment_ = 0;
      phase_ = 0 ;
      loop_counter_ = 0;
    } else {
      if (segment_ == 0) reset_behaviour_ = attack_reset_behaviour_ ;
      else reset_behaviour_ = decay_release_reset_behaviour_;
      switch(reset_behaviour_) {
        case RESET_BEHAVIOUR_NULL:
          break ;
        case RESET_BEHAVIOUR_SEGMENT_PHASE:
          segment_ = 0;
          phase_ = 0;
          start_value_ = value_;
          break ;
        case RESET_BEHAVIOUR_SEGMENT_LEVEL_PHASE:
          segment_ = 0;
          phase_ = 0;
          start_value_ = level_[0];
          break ;
        case RESET_BEHAVIOUR_SEGMENT_LEVEL:
          start_value_ = level_[0];
          segment_ = 0 ;
          break ;
        case RESET_BEHAVIOUR_PHASE:
          start_value_ = value_;
          phase_ = 0 ;
          break ;
        default:
          break;              
      }
    }
    if (segment_ == 0 and amplitude_sampled_) sampled_amplitude_ = amplitude_ ;
  } else if ((control & CONTROL_GATE_FALLING) && sustain_point_ && attack_falling_gate_behaviour_ == FALLING_GATE_BEHAVIOUR_HONOUR) {
    start_value_ = value_;
    segment_ = sustain_point_;
    phase_ = 0;
  } else if (phase_ < phase_increment_) {
    start_value_ = level_[segment_ + 1];
    ++segment_;
    phase_ = 0;
    if (segment_ == loop_end_ && (control & CONTROL_GATE)) {
      ++loop_counter_;
      if (!max_loops_ || loop_counter_ < max_loops_) {
        segment_ = loop_start_;
      }
    }
    if (segment_ == num_segments_)
      state_mask_ |= ENV_EOC;    
  }
  
  bool done = segment_ == num_segments_;
  bool sustained = sustain_point_ && segment_ == sustain_point_ &&
      control & CONTROL_GATE;

  phase_increment_ =
      sustained || done ? 0 : lut_env_increments[time_[segment_] >> 8] >> time_multiplier_[segment_];

  int32_t a = start_value_;
  int32_t b = level_[segment_ + 1];
  uint16_t t = Interpolate824(
      lookup_table_table[LUT_ENV_LINEAR + shape_[segment_]], phase_);
  value_ = a + ((b - a) * (t >> 1) >> 15);
  phase_ += phase_increment_;
  if (amplitude_sampled_) {
    scaled_value_ = (value_ * sampled_amplitude_) >> 16;
  } else {
    scaled_value_ = (value_ * amplitude_) >> 16;
  }
  #ifdef BUCHLA_4U
    return(static_cast<uint16_t>(scaled_value_ << 1));
  #else
    return(static_cast<uint16_t>(scaled_value_));
  #endif
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

uint16_t MultistageEnvelope::RenderFastPreview(int16_t *values) const {

  const uint16_t num_segments = num_segments_;
  const uint16_t current_segment = segment_;
  const uint16_t sustain_point = sustain_point_;
  const uint32_t phase = phase_;
  int32_t start_value = level_[0];

  const uint16_t segment_width = sustain_point
    ? (2 * kFastPreviewWidth) / (2 * num_segments + 1)
    : kFastPreviewWidth / num_segments;

  int16_t *current_pos = values;
  if (current_segment != num_segments) {
    for (uint16_t segment = 0; segment <= current_segment; ++segment) {

      if (sustain_point && segment == sustain_point) {
        // Sustain points are half as wide as normal segments
        uint16_t w = segment_width / 2;
        while (w--)
          *current_pos++ = start_value;
      }

      uint32_t segment_w = time_[segment] * segment_width >> 16; // segment_width
      CONSTRAIN(segment_w, 1, segment_width);

      const bool phase_in_segment = segment == current_segment;
      uint16_t w = segment_w;
      if (phase_in_segment)
        w = (((phase >> 24) * segment_w) / 256);

      uint32_t phase = 0, phase_increment = (0xff << 24) / segment_w;
      int32_t a = start_value;
      int32_t b = level_[segment + 1];
      while (w--) {
        uint16_t t = Interpolate824(
            lookup_table_table[LUT_ENV_LINEAR + shape_[segment]], phase);
        *current_pos++ = a + ((b - a) * (t >> 1) >> 15);
        phase += phase_increment;
      }
      start_value = b;

      if (phase_in_segment)
        break;
    }
  }

  return current_pos - values;
}


}  // namespace peaks
