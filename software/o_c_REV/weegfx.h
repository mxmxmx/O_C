// Copyright (c) 2016 Patrick Dowling
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

#ifndef WEEGFX_H_
#define WEEGFX_H_

#include <stdint.h>
#include <string.h>

namespace weegfx {

typedef int_fast16_t coord_t;

enum DRAW_MODE {
  DRAW_NORMAL,
  DRAW_INVERSE,
  // faster since no read required
  DRAW_OVERWRITE,
  DRAW_OVERWRITE_INVERSE
};

// Quick & dirty graphics for 128 x 64 framebuffer with vertical pixels.
// - Writes to provided framebuffer
// - Interface pseudo-compatible with u8glib
// - Makes some assumptions based on fixed size

class Graphics {
public:

  static const uint8_t kWidth = 128;
  static const uint8_t kHeight = 64;
  static const size_t kFrameSize = kWidth * kHeight / 8;

  void Init();

  void Begin(uint8_t *frame, bool clear_frame);
  void End();

  // Pseudo-compatible with u8glib

  void setDefaultBackgroundColor();
  void setDefaultForegroundColor();

  template <DRAW_MODE draw_mode>
  inline void setPixel(coord_t x, coord_t y) __attribute__((always_inline));

  void drawBox(coord_t x, coord_t y, coord_t w, coord_t h);
  void drawFrame(coord_t x, coord_t y, coord_t w, coord_t h);

  void drawHLine(coord_t x, coord_t y, coord_t w);
  void drawVLine(coord_t x, coord_t y, coord_t h);

  void drawLine(coord_t x1, coord_t y1, coord_t x2, coord_t y2);

  void drawBitmap8(coord_t x, coord_t y, coord_t w, const uint8_t *data);

  // Beware: No clipping
  void drawCircle(coord_t center_x, coord_t center_y, coord_t r);

  void setFont(const void *);

  void setPrintPos(coord_t x, coord_t y) {
    text_x_ = x;
    text_y_ = y;
  }

  void print(char);
  void print(int);
  void print(long);

  void pretty_print(int);

  void print(const char *);
  void drawStr(coord_t x, coord_t y, const char *str);

private:
  uint8_t *frame_;

  coord_t text_x_;
  coord_t text_y_;

  DRAW_MODE draw_mode_;
};

template <>
inline void Graphics::setPixel<DRAW_NORMAL>(coord_t x, coord_t y) {
  *(frame_ + (y/8) * kWidth + x) |= (0x1 << (y & 0x7));
}

};

#endif // WEEGFX_H_
