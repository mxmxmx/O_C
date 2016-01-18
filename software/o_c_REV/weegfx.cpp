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

#include "weegfx.h"
#include <string.h>
#include <Arduino.h>
#include "util_macros.h"

using weegfx::Graphics;

// TODO
// - Bench templated draw_pixels_h (inlined versions) vs. function pointers
// - Offer specialized functions w/o clipping or specific draw mode?
// - Remainder masks as LUT or switch
// - 32bit ops? Should be possible along x-axis (use SIMD instructions?) but not y (page stride)
// - Clipping for x, y < 0
// - Support 16 bit text characters?
// - Kerning/BBX etc.
// - print(string) -> print(char) can re-use variables
// - etc.

#define CLIPX(x, w) \
  if (x + w > kWidth) w = kWidth - x; \
  if (x < 0) { w += x; x = 0; } \
  if (w <= 0) return; \
  do {} while (0)

#define CLIPY(y, h) \
  if (y + h > kHeight) h = kHeight - y; \
  if (y < 0) { h += y; y = 0; } \
  if (h <= 0) return; \
  do {} while (0)

template <weegfx::DRAW_MODE draw_mode>
inline void draw_pixels_h(uint8_t *dst, weegfx::coord_t count, uint8_t mask) __attribute__((always_inline));

template <weegfx::DRAW_MODE draw_mode>
inline void draw_pixels_h(uint8_t *dst, weegfx::coord_t count, const uint8_t *src) __attribute__((always_inline));

template <weegfx::DRAW_MODE draw_mode>
inline void draw_pixels_h(uint8_t *dst, weegfx::coord_t count, uint8_t mask) {
  while (count--) {
    switch (draw_mode) {
      case weegfx::DRAW_NORMAL: *dst++ |= mask; break;
      case weegfx::DRAW_INVERSE: *dst++ ^= mask; break;
      case weegfx::DRAW_OVERWRITE: *dst = mask; break;
    }
  }
}

template <weegfx::DRAW_MODE draw_mode>
inline void draw_pixels_h(uint8_t *dst, weegfx::coord_t count, const uint8_t *src) {
  while (count--) {
    switch(draw_mode) {
      case weegfx::DRAW_NORMAL: *dst++ |= *src++; break;
      case weegfx::DRAW_INVERSE: *dst++ ^= *src++; break;
      case weegfx::DRAW_OVERWRITE: *dst++ = *src++; break;
    }
  }
}


void Graphics::Init() {
  frame_ = NULL;
  setPrintPos(0, 0);
  draw_mode_ = DRAW_NORMAL;
}

void Graphics::Begin(uint8_t *frame, bool clear_frame) {

  frame_ = frame;
  if (clear_frame)
    memset(frame_, 0, kFrameSize);

  setPrintPos(0, 0);
  draw_mode_ = DRAW_NORMAL;
}

void Graphics::End() {
  frame_ = NULL;
}

void Graphics::setDefaultBackgroundColor() {
  draw_mode_ = DRAW_INVERSE;
}

void Graphics::setDefaultForegroundColor() {
  draw_mode_ = DRAW_NORMAL;
}

#define SETPIXELS_H(start, count, value) \
do { \
  uint8_t *ptr = start; \
  size_t n = count; \
  if (DRAW_NORMAL == draw_mode_) { \
    while (n--) { \
      *ptr++ |= value; \
    }; \
  } else { \
    while (n--) { \
      *ptr++ ^= value; } \
  } \
} while (0)

void Graphics::drawBox(coord_t x, coord_t y, coord_t w, coord_t h) {
  CLIPX(x, w);
  CLIPY(y, h);
  uint8_t *buf = frame_ + (y / 8) * kWidth + x;

  coord_t remainder = y & 0x7;
  if (remainder) {
    remainder = 8 - remainder;
    uint8_t mask = ~(0xff >> remainder);
    if (h < remainder) {
      mask &= (0xff >> (remainder - h));
      h = 0;
    } else {
      h -= remainder;
    }

    if (DRAW_NORMAL == draw_mode_)
      draw_pixels_h<DRAW_NORMAL>(buf, w, mask);
    else
      draw_pixels_h<DRAW_INVERSE>(buf, w, mask);
    buf += kWidth;
  }

  remainder = h & 0x7;
  h >>= 3;
  while (h--) {
    if (DRAW_NORMAL == draw_mode_)
      draw_pixels_h<DRAW_NORMAL>(buf, w, 0xff);
    else
      draw_pixels_h<DRAW_INVERSE>(buf, w, 0xff);
    buf += kWidth;
  }

  if (remainder) {
    SETPIXELS_H(buf, w, ~(0xff << remainder));
  }
}

void Graphics::drawFrame(coord_t x, coord_t y, coord_t w, coord_t h) {
  // Obvious candidate for optimizing
  drawHLine(x, y, w);
  drawVLine(x, y, h);
  drawVLine(x + w, y, h);
  drawHLine(x, y + h, w);
}

void Graphics::drawHLine(coord_t x, coord_t y, coord_t w) {

  coord_t h = 1;
  CLIPX(x, w);
  CLIPY(y, h);
  uint8_t *start = frame_ + (y / 8) * kWidth + x;

  if (DRAW_NORMAL == draw_mode_)
    draw_pixels_h<DRAW_NORMAL>(start, w, 0x1 << (y & 0x7));
  else
    draw_pixels_h<DRAW_INVERSE>(start, w, 0x1 << (y & 0x7));
}

void Graphics::drawVLine(coord_t x, coord_t y, coord_t h) {

  uint8_t *buf = frame_ + (y / 8) * kWidth + x;
  if (y + h > kHeight)
    h = kHeight - y;

  // unaligned start
  coord_t remainder = y & 0x7;
  if (remainder) {
    remainder = 8 - remainder;
    uint8_t mask = ~(0xff >> remainder);
    if (h < remainder) {
      mask &= (0xff >> (remainder - h));
      h = 0;
    } else {
      h -= remainder;
    }

    if (DRAW_NORMAL == draw_mode_)
      *buf |= mask;
    else
      *buf ^= mask;
    buf += kWidth;
  }

  // aligned loop
  if (DRAW_NORMAL == draw_mode_) {
    while (h >= 8) {
      *buf = 0xff;
      buf += kWidth;
      h -= 8;
    }
  } else {
    while (h >= 8) {
      *buf ^= 0xff;
      buf += kWidth;
      h -= 8;
    }
  }

  // unaligned remainder
  if (h) {
    if (DRAW_NORMAL == draw_mode_)
      *buf |= ~(0xff << h);
    else
      *buf ^= ~(0xff << h);
  }
}

void Graphics::drawBitmap8(coord_t x, coord_t y, coord_t w, const uint8_t *data) {

  if (x + w > kWidth) w = kWidth - x;
  if (x < 0) {
    data += x;
    w += x;
  }
  if (w <= 0)
    return;

  coord_t h = 8;
  CLIPY(y, h);

  uint8_t *buf = frame_ + (y / 8) * kWidth + x;

  coord_t remainder = y & 0x7;
  if (!remainder) {
    SETPIXELS_H(buf, w, *data++);
  } else {
    const uint8_t *src = data;
    SETPIXELS_H(buf, w, (*src++) << remainder);
    if (h >= 8) {
      buf += kWidth;
      src = data;
      SETPIXELS_H(buf, w, (*src++) >> (8 - remainder));
    }
  }
}

void Graphics::drawLine(coord_t x0, coord_t y0, coord_t x1, coord_t y1) {
  coord_t dx, dy;
  if (x0 > x1 ) dx = x0-x1; else dx = x1-x0;
  if (y0 > y1 ) dy = y0-y1; else dy = y1-y0;

  bool steep = false;
  if (dy > dx) {
    steep = true;
    SWAP(dx, dy);
    SWAP(x0, y0);
    SWAP(x1, y1);
  }
  if (x0 > x1) {
    SWAP(x0, x1);
    SWAP(y0, y1);
  }
  coord_t err = dx >> 1;
  coord_t ystep = (y1 > y0) ? 1 : -1;
  coord_t y = y0;

  // OPTIMIZE Generate mask/buffer offset before loop and update on-the-fly instead of setPixeling
  // OPTIMIZE Generate spans of pixels to draw

  if (steep) {
    for(coord_t x = x0; x <= x1; x++ ) {
      setPixel<DRAW_NORMAL>(y, x); 
      err -= dy;
      if (err < 0) {
        y += ystep;
        err += dx;
      }
    }
  } else {
    for(coord_t x = x0; x <= x1; x++ ) {
      setPixel<DRAW_NORMAL>(x, y); 
      err -= dy;
      if (err < 0) {
        y += ystep;
        err += dx;
      }
    }
  }
}

void Graphics::drawCircle(coord_t center_x, coord_t center_y, coord_t r) {
  coord_t f = 1 - r;
  coord_t ddF_x = 1;
  coord_t ddF_y = -2 * r;
  coord_t x = 0;
  coord_t y = r;

  setPixel<DRAW_NORMAL>(center_x  , center_y+r);
  setPixel<DRAW_NORMAL>(center_x  , center_y-r);
  setPixel<DRAW_NORMAL>(center_x+r, center_y  );
  setPixel<DRAW_NORMAL>(center_x-r, center_y  );

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  
    setPixel<DRAW_NORMAL>(center_x + x, center_y + y);
    setPixel<DRAW_NORMAL>(center_x - x, center_y + y);
    setPixel<DRAW_NORMAL>(center_x + x, center_y - y);
    setPixel<DRAW_NORMAL>(center_x - x, center_y - y);
    setPixel<DRAW_NORMAL>(center_x + y, center_y + x);
    setPixel<DRAW_NORMAL>(center_x - y, center_y + x);
    setPixel<DRAW_NORMAL>(center_x + y, center_y - x);
    setPixel<DRAW_NORMAL>(center_x - y, center_y - x);
  }
}

#include "gfx_font_6x8.h"

void Graphics::setFont(const void *) {
}

void Graphics::print(char c) {
  if (c < 32 || c > 128) {
    return;
  }
  const uint8_t *data = ssd1306xled_font6x8 + 6 * (c - 32);
  uint8_t *buf = frame_ + (text_y_ / 8) * kWidth + text_x_;
  coord_t w = text_x_ + 6 > kWidth ? 6 - text_x_ : 6;
  coord_t h = text_y_ + 8 > kHeight ? 8 - text_y_ : 8;

  coord_t remainder = text_y_ & 0x7;
  if (!remainder) {
    SETPIXELS_H(buf, w, *data++);
  } else {
    const uint8_t *src = data;
    SETPIXELS_H(buf, w, (*src++) << remainder);
    if (h >= 8) {
      buf += kWidth;
      src = data;
      SETPIXELS_H(buf, w, (*src++) >> (8 - remainder));
    }
  }
  
  text_x_ += 6;
}

template <typename type, bool pretty>
char *itos(type value, char *buf, size_t buflen) {
  char *pos = buf + buflen;
  *--pos = '\0';
  if (!value) {
    *--pos = '0';
  } else {
    char sign = 0;
    if (value < 0)  {
      sign = '-';
      value = -value;
    } else if (pretty) {
      sign = '+';
    }

    while (value) {
      *--pos = '0' + value % 10;
      value /= 10;
    }
    if (sign)
      *--pos = sign;
  }

  return pos;
}

void Graphics::print(int value) {
  char buf[12];
  print(itos<int, false>(value, buf, sizeof(buf)));
}

void Graphics::print(long value) {
  char buf[24];
  print(itos<long, false>(value, buf, sizeof(buf)));
}

void Graphics::pretty_print(int value) {
  char buf[12];
  print(itos<int, true>(value, buf, sizeof(buf)));
}

void Graphics::print(const char *s) {
  while (*s) {
    print(*s++);
  }
}

void Graphics::drawStr(coord_t x, coord_t y, const char *str) {
  const coord_t tx = text_x_;
  const coord_t ty = text_y_;

  setPrintPos(x, y);
  print(str);

  text_x_ = tx;
  text_y_ = ty;
}
