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

#ifndef FRAMES_POLY_LFO_H_
#define FRAMES_POLY_LFO_H_

#include "util/util_macros.h"
//#include "stmlib/stmlib.h"
//#include "frames/keyframer.h"
#include "peaks_pattern_predictor.h"

const uint32_t kSyncCounterMaxTime = 8 * 16667;

namespace frames {

const size_t kNumChannels = 4;

enum PolyLfoFreqMultipliers {
  POLYLFO_FREQ_MULT_BY16,     // 0
  POLYLFO_FREQ_MULT_BY15,     // 1
  POLYLFO_FREQ_MULT_BY14,     // 2
  POLYLFO_FREQ_MULT_BY13,     // 3
  POLYLFO_FREQ_MULT_BY12,     // 4
  POLYLFO_FREQ_MULT_BY11,     // 5
  POLYLFO_FREQ_MULT_BY10,     // 6
  POLYLFO_FREQ_MULT_BY9,      // 7
  POLYLFO_FREQ_MULT_BY8,      // 8
  POLYLFO_FREQ_MULT_BY7,      // 9
  POLYLFO_FREQ_MULT_BY6,      // 10
  POLYLFO_FREQ_MULT_BY5,      // 11
  POLYLFO_FREQ_MULT_BY4,      // 12
  POLYLFO_FREQ_MULT_BY3,      // 13
  POLYLFO_FREQ_MULT_5_OVER_2, // 14
  POLYLFO_FREQ_MULT_BY2,      // 15
  POLYLFO_FREQ_MULT_5_OVER_3, // 16
  POLYLFO_FREQ_MULT_3_OVER_2, // 17
  POLYLFO_FREQ_MULT_5_OVER_4, // 18
  POLYLFO_FREQ_MULT_NONE,     // 19
  POLYLFO_FREQ_MULT_4_OVER_5, // 20
  POLYLFO_FREQ_MULT_2_OVER_3, // 21
  POLYLFO_FREQ_MULT_3_OVER_5, // 22
  POLYLFO_FREQ_MULT_1_OVER_2, // 23
  POLYLFO_FREQ_MULT_2_OVER_5, // 24
  POLYLFO_FREQ_MULT_1_OVER_3, // 25
  POLYLFO_FREQ_MULT_1_OVER_4, // 26
  POLYLFO_FREQ_MULT_1_OVER_5, // 27
  POLYLFO_FREQ_MULT_1_OVER_6, // 28
  POLYLFO_FREQ_MULT_1_OVER_7, // 29
  POLYLFO_FREQ_MULT_1_OVER_8, // 30
  POLYLFO_FREQ_MULT_1_OVER_9,  // 31
  POLYLFO_FREQ_MULT_1_OVER_10, // 32
  POLYLFO_FREQ_MULT_1_OVER_11, // 33
  POLYLFO_FREQ_MULT_1_OVER_12, // 34
  POLYLFO_FREQ_MULT_1_OVER_13, // 35
  POLYLFO_FREQ_MULT_1_OVER_14, // 36
  POLYLFO_FREQ_MULT_1_OVER_15, // 37
  POLYLFO_FREQ_MULT_1_OVER_16, // 38
  POLYLFO_FREQ_MULT_LAST       // 39
};

int32_t const PolyLfoFreqMultNumerators[] = {
         268435456,  // POLYLFO_FREQ_MULT_BY16,     = 0
         251658240,  // POLYLFO_FREQ_MULT_BY15,     = 1
         234881024,  // POLYLFO_FREQ_MULT_BY14,     = 2
         218103808,  // POLYLFO_FREQ_MULT_BY13,     = 3
         201326592,  // POLYLFO_FREQ_MULT_BY12,     = 4
         184549376,  // POLYLFO_FREQ_MULT_BY11,     = 5
         167772160,  // POLYLFO_FREQ_MULT_BY10,     = 6
         150994944,  // POLYLFO_FREQ_MULT_BY9,      = 7
         134217728,  // POLYLFO_FREQ_MULT_BY8,      = 8
         117440512,  // POLYLFO_FREQ_MULT_BY7,      = 9
         100663296,  // POLYLFO_FREQ_MULT_BY6,      = 10
          83886080,  // POLYLFO_FREQ_MULT_BY5,      = 11
          67108864,  // POLYLFO_FREQ_MULT_BY4,      = 12
          50331648,  // POLYLFO_FREQ_MULT_BY3,      = 13
          41943040,  // POLYLFO_FREQ_MULT_5_OVER_2, = 14
          33554432,  // POLYLFO_FREQ_MULT_BY2,      = 15
          27962027,  // POLYLFO_FREQ_MULT_5_OVER_3, = 16
          25165824,  // POLYLFO_FREQ_MULT_3_OVER_2, = 17
          20971520,  // POLYLFO_FREQ_MULT_5_OVER_4, = 18
          16777216,  // POLYLFO_FREQ_MULT_NONE,     = 19
          13421772,  // POLYLFO_FREQ_MULT_4_OVER_5, = 20
          11184810,  // POLYLFO_FREQ_MULT_2_OVER_3, = 21
          10066329,  // POLYLFO_FREQ_MULT_3_OVER_5, = 22
           8388608,  // POLYLFO_FREQ_MULT_1_OVER_2, = 23
           6710886,  // POLYLFO_FREQ_MULT_2_OVER_5, = 24
           5592405,  // POLYLFO_FREQ_MULT_1_OVER_3, = 25
           4194304,  // POLYLFO_FREQ_MULT_1_OVER_4, = 26
           3355443,  // POLYLFO_FREQ_MULT_1_OVER_5, = 27
           2796202,  // POLYLFO_FREQ_MULT_1_OVER_6, = 28
           2396745,  // POLYLFO_FREQ_MULT_1_OVER_7, = 29
           2097152,  // POLYLFO_FREQ_MULT_1_OVER_8, = 30
           1864135,  // POLYLFO_FREQ_MULT_1_OVER_9,  = 31
           1677721,  // POLYLFO_FREQ_MULT_1_OVER_10, = 32
           1525201,  // POLYLFO_FREQ_MULT_1_OVER_11, = 33
           1398101,  // POLYLFO_FREQ_MULT_1_OVER_12, = 34
           1290555,  // POLYLFO_FREQ_MULT_1_OVER_13, = 35
           1198372,  // POLYLFO_FREQ_MULT_1_OVER_14, = 36
           1118481,  // POLYLFO_FREQ_MULT_1_OVER_15, = 37
           1048576,  // POLYLFO_FREQ_MULT_1_OVER_16, = 38
} ;

class PolyLfo {
 public:
  PolyLfo() { }
  ~PolyLfo() { }
  
  void Init();
  void Render(int32_t frequency, bool reset_phase, bool tempo_sync, uint8_t freq_mult);
  void RenderPreview(uint16_t shape, uint16_t *buffer, size_t size);

  inline void set_freq_range(uint16_t freq_range) {
   freq_range_ = freq_range;
  }
  
  inline void set_shape(uint16_t shape) {
    shape_ = shape;
  }

  inline void set_shape_spread(uint16_t shape_spread) {
    shape_spread_ = static_cast<int16_t>(shape_spread - 32767) >> 1;
  }

  inline void set_spread(uint16_t spread) {
    if (spread < 32768) {
      int32_t x = spread - 32768;
      int32_t scaled = -(x * x >> 15);
      spread_ = (x + 3 * scaled) >> 2;
    } else {
      spread_ = spread - 32768;
    }
  }

  inline void set_coupling(uint16_t coupling) {
    int32_t x = coupling - 32768;
    int32_t scaled = x * x >> 15;
    scaled = x > 0 ? scaled : - scaled;
    scaled = (x + 3 * scaled) >> 2;
    coupling_ = (scaled >> 4) * 10;
    
  }

  inline void set_attenuation(uint16_t attenuation) {
    attenuation_ = attenuation;
    // attenuation_ = 65535;
  }

  inline void set_offset(int16_t offs) {
    offset_ = offs;
  }

  inline void set_freq_div_b(PolyLfoFreqMultipliers div) {
    if (div != freq_div_b_) {
      freq_div_b_ = div;
      phase_reset_flag_ = true;
    }
  }

  inline void set_freq_div_c(PolyLfoFreqMultipliers div) {
    if (div != freq_div_c_) {
      freq_div_c_ = div;
      phase_reset_flag_ = true;
    }
  }

  inline void set_freq_div_d(PolyLfoFreqMultipliers div) {
    if (div != freq_div_d_) {
      freq_div_d_ = div;
      phase_reset_flag_ = true;
    }
  }

  inline void set_b_xor_a(uint8_t xor_value) {
    if (xor_value) {
      b_xor_a_ = 16 - xor_value ;
    } else {
      b_xor_a_ = 0;
    }
  }

  inline void set_c_xor_a(uint8_t xor_value) {
    if (xor_value) {
      c_xor_a_ = 16 - xor_value ;
    } else {
      c_xor_a_ = 0;
    }
  }

  inline void set_d_xor_a(uint8_t xor_value) {
    if (xor_value) {
      d_xor_a_ = 16 - xor_value ;
    } else {
      d_xor_a_ = 0;
    }
  }

  inline void set_b_am_by_a(uint8_t am_value) {
    b_am_by_a_ = (am_value << 1);
  }

  inline void set_c_am_by_b(uint8_t am_value) {
    c_am_by_b_ = (am_value << 1);
  }

  inline void set_d_am_by_c(uint8_t am_value) {
    d_am_by_c_ = (am_value << 1);
  }

  inline void set_phase_reset_flag(bool reset) {
    phase_reset_flag_ = reset;
  }

  inline void set_sync(bool sync) {
    sync_ = sync;
  }

  inline int get_sync() {
    return static_cast<int>(sync_);
  }

  inline long get_sync_phase_increment() {
    return sync_phase_increment_;
  }

  inline float get_freq_ch1() {
    return(static_cast<float>(16666.6666666666666666667f * static_cast<double>(phase_increment_ch1_) / static_cast<double>(0xffffffff)));
  }

  inline long get_sync_counter() {
    return sync_counter_;
  }

  inline uint8_t level(uint8_t index) const {
    return level_[index];
  }

  inline const uint16_t dac_code(uint8_t index) const {
    return dac_code_[index];
  }

  static uint32_t FrequencyToPhaseIncrement(int32_t frequency, uint16_t frq_rng);


 private:
  uint16_t freq_range_ ;
  uint16_t shape_;
  int16_t shape_spread_;
  int32_t spread_;
  int16_t coupling_;
  int32_t attenuation_;
  int32_t offset_;
  PolyLfoFreqMultipliers freq_div_b_;
  PolyLfoFreqMultipliers freq_div_c_;
  PolyLfoFreqMultipliers freq_div_d_;
  uint8_t b_xor_a_ ;
  uint8_t c_xor_a_ ;
  uint8_t d_xor_a_ ;
  uint8_t b_am_by_a_ ;
  uint8_t c_am_by_b_ ;
  uint8_t d_am_by_c_ ;
  bool phase_reset_flag_ ;

  int16_t value_[kNumChannels];
  int16_t wt_value_[kNumChannels];
  uint32_t phase_[kNumChannels];
  uint32_t phase_increment_ch1_;
  uint8_t level_[kNumChannels];
  uint16_t dac_code_[kNumChannels];

  bool sync_ ;
  uint32_t sync_counter_;
  stmlib::PatternPredictor<32, 8> pattern_predictor_;
  uint32_t period_;
  uint32_t sync_phase_increment_;
  uint32_t phase_difference_ ;
  uint32_t last_phase_difference_ ;
  
  DISALLOW_COPY_AND_ASSIGN(PolyLfo);
};

}  // namespace frames

#endif  // FRAMES_POLY_LFO_H_
