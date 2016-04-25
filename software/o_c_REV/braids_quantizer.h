// Copyright 2015 Olivier Gillet.
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
// Note quantizer

#ifndef BRAIDS_QUANTIZER_H_
#define BRAIDS_QUANTIZER_H_

//#include "stmlib/stmlib.h"
#include "util/util_macros.h"

namespace braids {
  
struct Scale {
  int16_t span;
  size_t num_notes;
  int16_t notes[16];
};

void SortScale(Scale &);

class Quantizer {
 public:
  Quantizer() { }
  ~Quantizer() { }
  
  void Init();
  
  int32_t Process(int32_t pitch) {
    return Process(pitch, 0, 0);
  }
  
  int32_t Process(int32_t pitch, int32_t root, int32_t transpose);
  
  void Configure(const Scale& scale, uint16_t mask = 0xffff) {
    Configure(scale.notes, scale.span, scale.num_notes, mask);
  }

  bool enabled() const {
    return enabled_;
  }

  // HACK for TM
  int32_t Lookup(int32_t index) const;

 private:
  void Configure(const int16_t* notes, int16_t span, size_t num_notes, uint16_t mask);
  bool enabled_;
  int16_t codebook_[128];
  int32_t codeword_;
  int32_t transpose_;
  int32_t previous_boundary_;
  int32_t next_boundary_;
  
  DISALLOW_COPY_AND_ASSIGN(Quantizer);
};

}  // namespace braids

#endif // BRAIDS_QUANTIZER_H_
