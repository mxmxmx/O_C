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

#ifndef OC_EUCLIDEAN_MASK_DRAW_H_
#define OC_EUCLIDEAN_MASK_DRAW_H_

#include "bjorklund.h"
#include "OC_menus.h"

namespace OC {

class EuclideanMaskDraw {
public:
  EuclideanMaskDraw() { }
  ~EuclideanMaskDraw() { }

  void Init()
  {
    last_length_ = last_fill_ = last_offset_ = 0;
    mask_ = 0;
  }

  void Render(weegfx::coord_t x, weegfx::coord_t y, uint8_t length, uint8_t fill, uint8_t offset, uint8_t active_step)
  {
    (void)x;
    UpdateMask(length, fill, offset);
    menu::DrawMask<false, 32, 7, 1>(64 + 2 + (length + 1) * 3 / 2, y, mask_, length + 1, active_step % (length + 1));
  }

private:

  uint32_t mask_;
  uint8_t last_length_, last_fill_, last_offset_;

  void UpdateMask(uint8_t length, uint8_t fill, uint8_t offset)
  {
    if (length != last_length_ || fill != last_fill_ || offset != last_offset_) {
      mask_ = EuclideanPattern(length, fill, offset);
      last_length_ = length;
      last_fill_ = fill;
      last_offset_ = offset;
    }
  }
};

};

#endif // OC_EUCLIDEAN_MASK_DRAW_H_
