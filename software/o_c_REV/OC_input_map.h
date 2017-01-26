// Copyright 2015 Olivier Gillet, 2016 mxmxmx
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
// adapted for OC, mxmxmx
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
// input mapping, based on braids quantizer

#ifndef OC_INPUT_MAP_H_
#define OC_INPUT_MAP_H_

#include "util/util_macros.h"

namespace OC {
  
struct Map {
  int8_t num_slots;
  int16_t ranges[2];
};


class Input_Map {
 public:
  Input_Map() { }
  ~Input_Map() { }
  
  void Init();
  
  int8_t Process(int32_t input);
  
  void Configure(const Map& map, uint8_t range) {
    Configure(map.num_slots, map.ranges, range);
  }

  bool enabled() const {
    return enabled_;
  }

 private:
  void Configure(const int8_t num_slots, const int16_t* ranges, uint8_t range);
  bool enabled_;
  bool update_;
  int16_t codebook_[16];
  int8_t code_index_;
  int8_t num_slots_;
  int32_t previous_boundary_;
  int32_t next_boundary_;
  
  DISALLOW_COPY_AND_ASSIGN(Input_Map);
};

}  // namespace OC

#endif 
