// Copyright (c) 2016 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdint.h>
#include "OC_bitmaps.h"

namespace OC {

const uint8_t bitmap_empty_frame4x8[] = {
  0xff, 0x81, 0x81, 0xff
};

const uint8_t bitmap_end_marker4x8[] = {
  0x66, 0x6f, 0x6f, 0x66
};

const uint8_t bitmap_indicator_4x8[] = {
  0x00, 0x18, 0x18, 0x00
};

const uint8_t bitmap_hold_indicator_4x8[] = {
  0x00, 0x66, 0x66, 0x00
};

const uint8_t bitmap_edit_indicators_8[kBitmapEditIndicatorW * 3] = {
  0x24, 0x66, 0xe7, 0x66, 0x24, // both
  0x04, 0x06, 0x07, 0x06, 0x04, // min
  0x20, 0x60, 0xe0, 0x60, 0x20  // max
};

const uint8_t bitmap_gate_indicators_8[] = {
  0x00, 0x00, 0x00, 0x00,
  0xc0, 0xc0, 0xc0, 0xc0,
  0xf0, 0xf0, 0xf0, 0xf0,
  0xfc, 0xfc, 0xfc, 0xfc,
  0xff, 0xff, 0xff, 0xff
};

const uint8_t bitmap_loop_markers_8[kBitmapLoopMarkerW * 2] = {
  0x07, 0x02,
  0x02, 0x07
};

const uint8_t circle_disk_bitmap_8x8[] = {
  0, 0x18, 0x3c, 0x7e, 0x7e, 0x3c, 0x18, 0
};

const uint8_t circle_bitmap_8x8[] = {
  0, 0x18, 0x24, 0x42, 0x42, 0x24, 0x18, 0
};

};
