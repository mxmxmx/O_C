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

#ifndef PEAKS_MODULATIONS_MULTISTAGE_ENVELOPE_H_
#define PEAKS_MODULATIONS_MULTISTAGE_ENVELOPE_H_

#include <stdint.h>
#include "util/util_macros.h"
#include "OC_options.h"
#include "peaks_gate_processor.h"

namespace peaks {

enum EnvelopeShape {
  ENV_SHAPE_LINEAR,
  ENV_SHAPE_EXPONENTIAL,
  ENV_SHAPE_QUARTIC,
  ENV_SHAPE_SINE,
  ENV_SHAPE_PLATEAU,
  ENV_SHAPE_CLIFF,
  ENV_SHAPE_GATE,
  ENV_SHAPE_BIG_DIPPER,
  ENV_SHAPE_MEDIUM_DIPPER,
  ENV_SHAPE_LITTLE_DIPPER,
  ENV_SHAPE_SINEFOLD,
  ENV_SHAPE_LAST
};

enum EnvResetBehaviour {
  RESET_BEHAVIOUR_NULL,
  RESET_BEHAVIOUR_SEGMENT_PHASE,
  RESET_BEHAVIOUR_SEGMENT_LEVEL_PHASE,
  RESET_BEHAVIOUR_SEGMENT_LEVEL,
  RESET_BEHAVIOUR_PHASE,
  RESET_BEHAVIOUR_LAST
};

enum EnvFallingGateBehaviour {
  FALLING_GATE_BEHAVIOUR_IGNORE,
  FALLING_GATE_BEHAVIOUR_HONOUR,
  FALLING_GATE_BEHAVIOUR_LAST  
} ;

enum EnvStateBitMask {
  ENV_EOC = 1 << 0, // End of envelope reached
};

const uint16_t kMaxNumSegments = 8;
const uint32_t kPreviewWidth = 128;
const uint32_t kFastPreviewWidth = 64;

class MultistageEnvelope {
 public:
  MultistageEnvelope() { }
  ~MultistageEnvelope() { }
  
  void Init();
  uint16_t ProcessSingleSample(uint8_t control);

  void Configure(uint16_t* parameter, ControlMode control_mode) {
    if (control_mode == CONTROL_MODE_HALF) {
      set_ad(parameter[0], parameter[1], 0, 0);
    } else {
      set_adsr(parameter[0], parameter[1], parameter[2] >> 1, parameter[3]);
    }
    if (segment_ > num_segments_) {
      segment_ = 0;
      phase_ = 0;
      value_ = 0;
    }
  }
  
  inline void set_time(uint16_t segment, uint16_t time) {
    time_[segment] = time;
  }

  inline void set_time_multiplier(uint16_t segment, uint16_t time_multiplier) {
    time_multiplier_[segment] = time_multiplier;
  }
  
  inline void set_level(uint16_t segment, int16_t level) {
    level_[segment] = level;
  }
  
  inline void set_num_segments(uint16_t num_segments) {
    num_segments_ = num_segments;
  }
  
  inline void set_sustain_point(uint16_t sustain_point) {
    sustain_point_ = sustain_point;
  }

  inline void set_adsr(
      uint16_t attack,
      uint16_t decay,
      uint16_t sustain,
      uint16_t release) {
    num_segments_ = 3;
    sustain_point_ = 2;
    sustain_index_ = 2;

    level_[0] = 0;
    level_[1] = 32767;
    level_[2] = sustain;
    level_[3] = 0;

    time_[0] = attack;
    time_[1] = decay;
    time_[2] = release;
    
    shape_[0] = attack_shape_;
    shape_[1] = decay_shape_;
    shape_[2] = release_shape_;

    time_multiplier_[0] = attack_multiplier_;
    time_multiplier_[1] = decay_multiplier_;
    time_multiplier_[2] = release_multiplier_;

    loop_start_ = loop_end_ = 0;
  }
  
  inline void set_ad(uint16_t attack, uint16_t decay, uint16_t loop_start, uint16_t loop_end) {
    num_segments_ = 2;
    sustain_point_ = 0;
    sustain_index_ = 0;

    level_[0] = 0;
    level_[1] = 32767;
    level_[2] = 0;

    time_[0] = attack;
    time_[1] = decay;
    
    shape_[0] = attack_shape_;
    shape_[1] = decay_shape_;

    time_multiplier_[0] = attack_multiplier_;
    time_multiplier_[1] = decay_multiplier_;
    
    loop_start_ = loop_start;
    loop_end_ = loop_end;
  }

  inline void set_adr(
      uint16_t attack,
      uint16_t decay,
      uint16_t sustain,
      uint16_t release,
      uint16_t loop_start,
      uint16_t loop_end) {
    num_segments_ = 3;
    sustain_point_ = 0;
    sustain_index_ = 2;

    level_[0] = 0;
    level_[1] = 32767;
    level_[2] = sustain;
    level_[3] = 0;

    time_[0] = attack;
    time_[1] = decay;
    time_[2] = release;
    
    shape_[0] = attack_shape_;
    shape_[1] = decay_shape_;
    shape_[2] = release_shape_;

    time_multiplier_[0] = attack_multiplier_;
    time_multiplier_[1] = decay_multiplier_;
    time_multiplier_[2] = release_multiplier_;
    
    loop_start_ = loop_start ;
    loop_end_ = loop_end ;
  }

  inline void set_ar(uint16_t attack, uint16_t release) {
    num_segments_ = 2;
    sustain_point_ = 1;
    sustain_index_ = 0;

    level_[0] = 0;
    level_[1] = 32767;
    level_[2] = 0;

    time_[0] = attack;
    time_[1] = release;
    
    shape_[0] = attack_shape_;
    shape_[1] = release_shape_;

    time_multiplier_[0] = attack_multiplier_;
    time_multiplier_[1] = release_multiplier_;
    
    loop_start_ = loop_end_ = 0;
  }
  
  inline void set_adsar(
      uint16_t attack,
      uint16_t decay,
      uint16_t sustain,
      uint16_t release) {
    num_segments_ = 4;
    sustain_point_ = 2;
    sustain_index_ = 2;

    level_[0] = 0;
    level_[1] = 32767;
    level_[2] = sustain;
    level_[3] = 32767;
    level_[4] = 0;

    time_[0] = attack;
    time_[1] = decay;
    time_[2] = attack;
    time_[3] = release;
    
    shape_[0] = attack_shape_;
    shape_[1] = decay_shape_;
    shape_[2] = attack_shape_;
    shape_[3] = release_shape_;

    time_multiplier_[0] = attack_multiplier_;
    time_multiplier_[1] = decay_multiplier_;
    time_multiplier_[2] = attack_multiplier_;
    time_multiplier_[3] = release_multiplier_;
    
    loop_start_ = loop_end_ = 0;
  }
  
  inline void set_adar(
      uint16_t attack,
      uint16_t decay,
      uint16_t sustain,
      uint16_t release,
      uint16_t loop_start,
      uint16_t loop_end) {
    num_segments_ = 4;
    sustain_point_ = 0;
    sustain_index_ = 2;

    level_[0] = 0;
    level_[1] = 32767;
    level_[2] = sustain;
    level_[3] = 32767;
    level_[4] = 0;

    time_[0] = attack;
    time_[1] = decay;
    time_[2] = attack;
    time_[3] = release;
    
    shape_[0] = attack_shape_;
    shape_[1] = decay_shape_;
    shape_[2] = attack_shape_;
    shape_[3] = release_shape_;

    time_multiplier_[0] = attack_multiplier_;
    time_multiplier_[1] = decay_multiplier_;
    time_multiplier_[2] = attack_multiplier_;
    time_multiplier_[3] = release_multiplier_;
   
    loop_start_ = loop_start;
    loop_end_ = loop_end;
  }
        
  inline void set_attack_reset_behaviour(EnvResetBehaviour reset_behaviour) {
    attack_reset_behaviour_ = reset_behaviour;
  }

  inline void set_attack_falling_gate_behaviour(EnvFallingGateBehaviour falling_gate_behaviour) {
    attack_falling_gate_behaviour_ = falling_gate_behaviour;
  }

  inline void set_decay_release_reset_behaviour(EnvResetBehaviour reset_behaviour) {
    decay_release_reset_behaviour_ = reset_behaviour;
  }

  inline void reset() {
    if (segment_ > num_segments_) {
      segment_ = 0;
      phase_ = 0;
      value_ = 0;
    }
  }

  inline void set_attack_shape(EnvelopeShape shape) {
    attack_shape_ = shape;
  }

  inline void set_decay_shape(EnvelopeShape shape) {
    decay_shape_ = shape;
  }

  inline void set_release_shape(EnvelopeShape shape) {
    release_shape_ = shape;
  }

  inline void set_attack_time_multiplier(uint16_t mult) {
    attack_multiplier_ = mult;
  }

  inline void set_decay_time_multiplier(uint16_t mult) {
    decay_multiplier_ = mult;
  }

  inline void set_release_time_multiplier(uint16_t mult) {
    release_multiplier_ = mult;
  }

  inline void set_amplitude(uint16_t amp, bool sampled) {
    amplitude_ = amp;
    amplitude_sampled_ = sampled;
  }

  inline void set_max_loops(uint16_t max_loops) {
    max_loops_ = static_cast<uint8_t>(max_loops >> 9);
  }
 
#ifdef ENVGEN_DEBUG
  inline uint16_t get_amplitude_value() {
    return(amplitude_) ;
  }

   inline uint16_t get_sampled_amplitude_value() {
    return(sampled_amplitude_) ;
  }

   inline bool get_is_amplitude_sampled() {
    return(amplitude_sampled_) ;
  }
#endif

  // Get current state mask; note that this is reset every call to ::Process
  inline uint8_t get_state_mask() const {
    return state_mask_;
  }

  // Render preview, normalized to kPreviewWidth pixels width
  // NOTE Lives dangerously and uses live values that might be updated by ISR
  uint16_t RenderPreview(int16_t *values, uint16_t *segment_start_points, uint16_t *loop_points, uint16_t &current_phase) const;

  // Render fast preview, normalized to kFastPreviewWidth, and only includes
  // points up to the current phase.
  // Also likes to live dangerously
  uint16_t RenderFastPreview(int16_t *values) const;

 private:
  int16_t level_[kMaxNumSegments];
  uint16_t time_[kMaxNumSegments];
  uint16_t time_multiplier_[kMaxNumSegments];
  EnvelopeShape shape_[kMaxNumSegments];
  
  int16_t segment_;
  int16_t start_value_;
  int16_t value_;

  uint32_t phase_;
  uint32_t phase_increment_;
  
  uint16_t num_segments_;
  uint16_t sustain_point_;
  uint16_t loop_start_;
  uint16_t loop_end_;
  uint8_t max_loops_;
  uint8_t loop_counter_;
  
  EnvResetBehaviour attack_reset_behaviour_;
  EnvFallingGateBehaviour attack_falling_gate_behaviour_;
  EnvResetBehaviour decay_release_reset_behaviour_;
  EnvResetBehaviour reset_behaviour_;
  EnvelopeShape attack_shape_;
  EnvelopeShape decay_shape_;
  EnvelopeShape release_shape_;
  uint16_t sustain_index_;
  uint16_t attack_multiplier_;
  uint16_t decay_multiplier_;
  uint16_t release_multiplier_;
  uint16_t amplitude_;
  bool amplitude_sampled_ ;
  uint16_t sampled_amplitude_;
  uint32_t scaled_value_ ;

  uint8_t state_mask_;

  DISALLOW_COPY_AND_ASSIGN(MultistageEnvelope);
};

}  // namespace peaks

#endif  // PEAKS_MODULATIONS_MULTISTAGE_ENVELOPE_H_
