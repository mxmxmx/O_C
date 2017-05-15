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

#ifndef OC_BITMAPS_H_
#define OC_BITMAPS_H_

// weegfx::Graphics only supports drawing of 8px high bitmaps right now...
// TODO Embed width in first byte?

namespace OC {

extern const uint8_t bitmap_empty_frame4x8[];
extern const uint8_t bitmap_end_marker4x8[];
extern const uint8_t bitmap_indicator_4x8[];
extern const uint8_t bitmap_hold_indicator_4x8[];

static constexpr int16_t kBitmapEditIndicatorW = 5;
extern const uint8_t bitmap_edit_indicators_8[];

extern const uint8_t bitmap_gate_indicators_8[];

static constexpr int16_t kBitmapLoopMarkerW = 2;
extern const uint8_t bitmap_loop_markers_8[];

extern const uint8_t circle_disk_bitmap_8x8[];
extern const uint8_t circle_bitmap_8x8[];

};

#endif // OC_BITMAPS_H_
