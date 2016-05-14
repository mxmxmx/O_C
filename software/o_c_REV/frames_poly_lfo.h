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


namespace frames {

const size_t kNumChannels = 4;

enum PolyLfoFreqDivisions {
  POLYLFO_FREQ_DIV_NONE,     // 0
  POLYLFO_FREQ_DIV_4_OVER_5, // 1
  POLYLFO_FREQ_DIV_2_OVER_3, // 2
  POLYLFO_FREQ_DIV_3_OVER_5, // 3
  POLYLFO_FREQ_DIV_BY2,      // 4
  POLYLFO_FREQ_DIV_2_OVER_5, // 5
  POLYLFO_FREQ_DIV_BY3,      // 6
  POLYLFO_FREQ_DIV_BY4,      // 7
  POLYLFO_FREQ_DIV_BY5,      // 8
  POLYLFO_FREQ_DIV_BY6,      // 9
  POLYLFO_FREQ_DIV_BY7,      // 10
  POLYLFO_FREQ_DIV_BY8,      // 11
  POLYLFO_FREQ_DIV_BY9,      // 12
  POLYLFO_FREQ_DIV_BY10,     // 13
  POLYLFO_FREQ_DIV_BY11,     // 14
  POLYLFO_FREQ_DIV_BY12,     // 15
  POLYLFO_FREQ_DIV_BY13,     // 16
  POLYLFO_FREQ_DIV_BY14,     // 17
  POLYLFO_FREQ_DIV_BY15,     // 18
  POLYLFO_FREQ_DIV_BY16,     // 19
  POLYLFO_FREQ_DIV_LAST      // 20
};

int32_t const PolyLfoFreqDivNumerators[] = {
          16777216, // POLYLFO_FREQ_DIV_NONE     = 0
          13421772, // POLYLFO_FREQ_DIV_4_OVER_5 = 1
          11184810, // POLYLFO_FREQ_DIV_2_OVER_3 = 2
          10066329, // POLYLFO_FREQ_DIV_3_OVER_5 = 3
           8388608, // POLYLFO_FREQ_DIV_BY2      = 4
           6710886, // POLYLFO_FREQ_DIV_2_OVER_5 = 5
           5592405, // POLYLFO_FREQ_DIV_BY3      = 6
           4194304, // POLYLFO_FREQ_DIV_BY4      = 7
           3355443, // POLYLFO_FREQ_DIV_BY5      = 8
           2796202, // POLYLFO_FREQ_DIV_BY6      = 9
           2396745, // POLYLFO_FREQ_DIV_BY7      = 10
           2097152, // POLYLFO_FREQ_DIV_BY8      = 11
           1864135, // POLYLFO_FREQ_DIV_BY9      = 12
           1677721, // POLYLFO_FREQ_DIV_BY10     = 13
           1525201, // POLYLFO_FREQ_DIV_BY11     = 14
           1398101, // POLYLFO_FREQ_DIV_BY12     = 15
           1290555, // POLYLFO_FREQ_DIV_BY13     = 16
           1198372, // POLYLFO_FREQ_DIV_BY14     = 17
           1118481, // POLYLFO_FREQ_DIV_BY15     = 19
           1048576, // POLYLFO_FREQ_DIV_BY16     = 20
} ;

class PolyLfo {
 public:
  PolyLfo() { }
  ~PolyLfo() { }
  
  void Init();
  void Render(int32_t frequency, bool reset_phase);
  void RenderPreview(uint16_t shape, uint16_t *buffer, size_t size);

  inline void set_freq_range(uint16_t freq_range) {
   freq_range_ = freq_range;
  }
  
  inline void set_shape(uint16_t shape) {
    shape_ = shape;
  }

  inline void set_shape_spread(uint16_t shape_spread) {
    shape_spread_ = static_cast<int16_t>(shape_spread - 32768) >> 1;
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

  inline void set_freq_div_b(PolyLfoFreqDivisions div) {
    if (div != freq_div_b_) {
      freq_div_b_ = div;
      phase_reset_flag_ = true;
    }
  }

  inline void set_freq_div_c(PolyLfoFreqDivisions div) {
    if (div != freq_div_c_) {
      freq_div_c_ = div;
      phase_reset_flag_ = true;
    }
  }

  inline void set_freq_div_d(PolyLfoFreqDivisions div) {
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

  inline uint8_t level(uint8_t index) const {
    return level_[index];
  }


  inline const uint16_t dac_code(uint8_t index) const {
    return dac_code_[index];
  }
  static uint32_t FrequencyToPhaseIncrement(int32_t frequency, uint16_t frq_rng);

 private:
  // static const uint8_t rainbow_[17][3];
  uint16_t freq_range_ ;
  uint16_t shape_;
  int16_t shape_spread_;
  int32_t spread_;
  int16_t coupling_;
  PolyLfoFreqDivisions freq_div_b_;
  PolyLfoFreqDivisions freq_div_c_;
  PolyLfoFreqDivisions freq_div_d_;
  uint8_t b_xor_a_ ;
  uint8_t c_xor_a_ ;
  uint8_t d_xor_a_ ;
  bool phase_reset_flag_ ;

  int16_t value_[kNumChannels];
  int16_t wt_value_[kNumChannels];
  uint32_t phase_[kNumChannels];
  uint32_t phase_increment_ch1_;
  uint8_t level_[kNumChannels];
  uint16_t dac_code_[kNumChannels];

  DISALLOW_COPY_AND_ASSIGN(PolyLfo);
};

}  // namespace frames

#endif  // FRAMES_POLY_LFO_H_
