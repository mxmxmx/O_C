// Copyright 2013 Olivier Gillet, 2015, 2016 Patrick Dowling and Tim Churches
//
// Original author: Olivier Gillet (ol.gillet@gmail.com)
// Adapted and modified for use in the Ornament + Crime module by:
// Patrick Dowling (pld@gurkenkiste.com) and Tim Churches (tim.churches@gmail.com)
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
// Poly LFO.

#include "frames_poly_lfo.h"

#include <cstdio>
#include <algorithm>

#include "extern/stmlib_utils_dsp.h"
#include "util/util_math.h"
#include "frames_resources.h"

namespace frames {

using namespace std;
using namespace stmlib;

void PolyLfo::Init() {
  freq_range_ = 9;
  spread_ = 0;
  shape_ = 0;
  shape_spread_ = 0;
  coupling_ = 0;
  attenuation_ = 58880;
  offset_ = 0 ;
  freq_div_b_ = freq_div_c_ = freq_div_d_ = POLYLFO_FREQ_MULT_NONE ;
  b_am_by_a_ = 0 ;
  c_am_by_b_ = 0 ;
  d_am_by_c_ = 0 ;
  phase_reset_flag_ = false;
  sync_counter_ = 0 ;
  sync_ = false;
  period_ = 0 ;
  std::fill(&value_[0], &value_[kNumChannels], 0);
  std::fill(&wt_value_[0], &wt_value_[kNumChannels], 0);
  std::fill(&phase_[0], &phase_[kNumChannels], 0);
  last_phase_difference_ = 0;
  last_phase_difference_ = 0;
  pattern_predictor_.Init();
}

/* static */
uint32_t PolyLfo::FrequencyToPhaseIncrement(int32_t frequency, uint16_t frq_rng) {
  int32_t shifts = frequency / 5040;
  int32_t index = frequency - shifts * 5040;
  uint32_t a;
  uint32_t b;
  switch(frq_rng){
    // "cosm", "geol", "glacl", "snail", "sloth", "vlazy", "lazy", "vslow", "slow", "med", "fast", "vfast",

    case 0: // cosmological
      a = lut_increments_med[index >> 5] >> 11;
      b = lut_increments_med[(index >> 5) + 1] >> 11;
      break;
    case 1: // geological
      a = lut_increments_med[index >> 5] >> 9;
      b = lut_increments_med[(index >> 5) + 1] >> 9;
      break;
    case 2: // glacial
      a = lut_increments_med[index >> 5] >> 7;
      b = lut_increments_med[(index >> 5) + 1] >> 7;
      break;
    case 3: // snail
      a = lut_increments_med[index >> 5] >> 6;
      b = lut_increments_med[(index >> 5) + 1] >> 6;
      break;
    case 4: // sloth
      a = lut_increments_med[index >> 5] >> 5;
      b = lut_increments_med[(index >> 5) + 1] >> 5;
      break;
    case 5: // vlazy
      a = lut_increments_med[index >> 5] >> 4;
      b = lut_increments_med[(index >> 5) + 1] >> 4;
      break;
    case 6: // lazy
      a = lut_increments_med[index >> 5] >> 3;
      b = lut_increments_med[(index >> 5) + 1] >> 3;
      break;
    case 7: // vslow
      a = lut_increments_med[index >> 5] >> 2;
      b = lut_increments_med[(index >> 5) + 1] >> 2;
      break;
    case 8: //slow
      a = lut_increments_med[index >> 5]  >> 1 ;
      b = lut_increments_med[(index >> 5) + 1] >> 1 ;
      break;
    case 9: // medium
      a = lut_increments_med[index >> 5] ;
      b = lut_increments_med[(index >> 5) + 1];
      break;
    case 10: // fast
      a = lut_increments_med[index >> 5] << 1;
      b = lut_increments_med[(index >> 5) + 1] << 1;
      break;
    case 11: // vfast
      a = lut_increments_med[index >> 5] << 2;
      b = lut_increments_med[(index >> 5) + 1] << 2;
      break;
    default:
      a = lut_increments_med[index >> 5];
      b = lut_increments_med[(index >> 5) + 1];
      break;
  }
  return (a + ((b - a) * (index & 0x1f) >> 5)) << shifts;
}

void PolyLfo::Render(int32_t frequency, bool reset_phase, bool tempo_sync, uint8_t freq_mult) {
    ++sync_counter_;
    if (tempo_sync && sync_) {
        if (sync_counter_ < kSyncCounterMaxTime) {
          uint32_t period = 0;
          if (sync_counter_ < 167) {
            period = (3 * period_ + sync_counter_) >> 2;
            tempo_sync = false;
          } else {
            period = pattern_predictor_.Predict(sync_counter_);
          }
          if (period != period_) {
            period_ = period;
            sync_phase_increment_ = 0xffffffff / period_;
            //sync_phase_increment_ = multiply_u32xu32_rshift24((0xffffffff / period_), 16183969) ;
          }
        }
        sync_counter_ = 0;
    }

  
  // reset phase
  if (reset_phase || phase_reset_flag_) {
    std::fill(&phase_[0], &phase_[kNumChannels], 0);
    phase_reset_flag_ = false ;
  } else {
    // increment freqs for each LFO
    if (sync_) {
      phase_increment_ch1_ = sync_phase_increment_;
    } else {
      phase_increment_ch1_ = FrequencyToPhaseIncrement(frequency, freq_range_);
    }
    
    // double F (via TR4) ? ... "/8", "/4", "/2", "x2", "x4", "x8"
    if (freq_mult < 0xFF) {
      phase_increment_ch1_ = (freq_mult < 0x3) ? (phase_increment_ch1_ >> (0x3 - freq_mult)) : phase_increment_ch1_ << (freq_mult - 0x2);
    }
    
    phase_[0] += phase_increment_ch1_;
    PolyLfoFreqMultipliers FreqDivs[] = {POLYLFO_FREQ_MULT_NONE, freq_div_b_, freq_div_c_ , freq_div_d_} ;
    for (uint8_t i = 1; i < kNumChannels; ++i) {
        if (FreqDivs[i] == POLYLFO_FREQ_MULT_NONE) {
            phase_[i] += phase_increment_ch1_;
        } else {
            phase_[i] += multiply_u32xu32_rshift24(phase_increment_ch1_, PolyLfoFreqMultNumerators[FreqDivs[i]]) ;
        }  
    }

    // Advance phasors.
    if (spread_ >= 0) {
      phase_difference_ = static_cast<uint32_t>(spread_) << 15;
      if (freq_div_b_ == POLYLFO_FREQ_MULT_NONE) {
        phase_[1] = phase_[0] + phase_difference_;
      } else {
        phase_[1] = phase_[1] - last_phase_difference_ + phase_difference_;
      }
      if (freq_div_c_ == POLYLFO_FREQ_MULT_NONE) {
        phase_[2] = phase_[0] + (2 * phase_difference_);
      } else {
        phase_[2] = phase_[2] - last_phase_difference_  + phase_difference_;
      }
      if (freq_div_d_ == POLYLFO_FREQ_MULT_NONE) {
        phase_[3] = phase_[0] + (3 * phase_difference_);
      } else {
        phase_[3] = phase_[3] - last_phase_difference_  + phase_difference_;
      }
    } else {
      for (uint8_t i = 1; i < kNumChannels; ++i) { 
        // phase_[i] += FrequencyToPhaseIncrement(frequency, freq_range_);
        phase_[i] -= i * (phase_increment_ch1_ >> 16) * spread_ ;
        // frequency -= 5040 * spread_ >> 15;
      }
    }
    last_phase_difference_ = phase_difference_;
  }
  
  const uint8_t* sine = &wt_lfo_waveforms[17 * 257];
  
  uint16_t wavetable_index = shape_;
  uint8_t xor_depths[] = {0, b_xor_a_, c_xor_a_, d_xor_a_ } ;
  uint8_t am_depths[] = {0, b_am_by_a_, c_am_by_b_, d_am_by_c_ } ;
  // Wavetable lookup
  for (uint8_t i = 0; i < kNumChannels; ++i) {
    uint32_t phase = phase_[i];
    if (coupling_ > 0) {
      phase += value_[(i + 1) % kNumChannels] * coupling_;
    } else {
      phase += value_[(i + kNumChannels - 1) % kNumChannels] * -coupling_;
    }
    const uint8_t* a = &wt_lfo_waveforms[(wavetable_index >> 12) * 257];
    const uint8_t* b = a + 257;
    wt_value_[i] = Crossfade(a, b, phase, wavetable_index << 4) ;
    value_[i] = Interpolate824(sine, phase);
    level_[i] = (wt_value_[i] + 32768) >> 8; 
    // add bit-XOR 
    uint8_t depth_xor = xor_depths[i];
    if (depth_xor) {
      dac_code_[i] = (wt_value_[i] + 32768) ^ (((wt_value_[0] + 32768) >> depth_xor) << depth_xor) ; 
    } else {
      dac_code_[i] = wt_value_[i] + 32768; //Keyframer::ConvertToDacCode(value + 32768, 0);
    }
    // cross-channel AM
    dac_code_[i] = (dac_code_[i] * (65535 - (((65535 - dac_code_[i-1]) * am_depths[i]) >> 8))) >> 16 ; 
    // attenuationand offset
    dac_code_[i] = ((dac_code_[i] * attenuation_) >> 16) + offset_ ;
    wavetable_index += shape_spread_;
  }
}

void PolyLfo::RenderPreview(uint16_t shape, uint16_t *buffer, size_t size) {
  uint16_t wavetable_index = shape;
  uint32_t phase = 0;
  uint32_t phase_increment = (0xff << 24) / size;
  while (size--) {
    const uint8_t* a = &wt_lfo_waveforms[(wavetable_index >> 12) * 257];
    const uint8_t* b = a + 257;
    int16_t value = Crossfade(a, b, phase, wavetable_index << 4);
    *buffer++ = value + 32768;
    phase += phase_increment;
  }
}


}  // namespace frames
