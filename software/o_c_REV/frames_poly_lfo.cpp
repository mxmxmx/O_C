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
  spread_ = 0;
  shape_ = 0;
  shape_spread_ = 0;
  coupling_ = 0;
  freq_div_b_ = freq_div_c_ = freq_div_d_ = POLYLFO_FREQ_DIV_NONE ;
  phase_reset_flag_ = false;
  std::fill(&value_[0], &value_[kNumChannels], 0);
  std::fill(&phase_[0], &phase_[kNumChannels], 0);
}

/* static */
uint32_t PolyLfo::FrequencyToPhaseIncrement(int32_t frequency) {
  int32_t shifts = frequency / 5040;
  int32_t index = frequency - shifts * 5040;
  uint32_t a = lut_increments[index >> 5];
  uint32_t b = lut_increments[(index >> 5) + 1];
  return (a + ((b - a) * (index & 0x1f) >> 5)) << shifts;
}

void PolyLfo::Render(int32_t frequency, bool reset_phase) {

  // reset phase
  if (reset_phase || phase_reset_flag_) {
    std::fill(&phase_[0], &phase_[kNumChannels], 0);
    phase_reset_flag_ = false ;
  } else {
    // increment freqs for each LFO
    phase_increment_ch1_ = FrequencyToPhaseIncrement(frequency);
    phase_[0] += phase_increment_ch1_ ;
    PolyLfoFreqDivisions FreqDivs[] = {POLYLFO_FREQ_DIV_NONE, freq_div_b_, freq_div_c_ , freq_div_d_ } ;
    for (uint8_t i = 1; i < kNumChannels; ++i) {
        if (FreqDivs[i] == POLYLFO_FREQ_DIV_NONE) {
            phase_[i] += phase_increment_ch1_;
        } else {
            phase_[i] += multiply_u32xu32_rshift24(phase_increment_ch1_, PolyLfoFreqDivNumerators[FreqDivs[i]]) ;
        }  
    }

    // Advance phasors.
    if (!(freq_div_b_ || freq_div_c_ || freq_div_c_)) {
      // original Frames behaviour
      if (spread_ >= 0) {
        phase_[0] += FrequencyToPhaseIncrement(frequency);
        uint32_t phase_difference = static_cast<uint32_t>(spread_) << 15;
        phase_[1] = phase_[0] + phase_difference;
        phase_[2] = phase_[1] + phase_difference;
        phase_[3] = phase_[2] + phase_difference;
      } else {
        for (uint8_t i = 0; i < kNumChannels; ++i) {
          phase_[i] += FrequencyToPhaseIncrement(frequency);
          frequency -= 5040 * spread_ >> 15;
        }
      }
    } else {
      // if frequency division is in use
      if (spread_ > 10) {
        uint32_t phase_difference = static_cast<uint32_t>(spread_ - 10) << 2;
        phase_[1] = phase_[1] + phase_difference;
        phase_[2] = phase_[2] + phase_difference;
        phase_[3] = phase_[3] + phase_difference;
      }
    }
  }
  
  const uint8_t* sine = &wt_lfo_waveforms[17 * 257];
  
  uint16_t wavetable_index = shape_;
  bool xor_flags[] = {false, a_xor_b_, b_xor_c_, c_xor_d_ } ;
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
    int16_t value = Crossfade(a, b, phase, wavetable_index << 4);
    value_[i] = Interpolate824(sine, phase);
    level_[i] = (value + 32768) >> 8;
    if (xor_flags[i]) {
      dac_code_[i] = (value + 32768) ^ dac_code_[i - 1] ; 
    } else {
      dac_code_[i] = value + 32768; //Keyframer::ConvertToDacCode(value + 32768, 0);
    }
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
