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
typedef const uint8_t *font_glyph;

// Quick & dirty graphics for 128 x 64 framebuffer with vertical pixels.
// - Writes to provided framebuffer
// - Interface pseudo-compatible with u8glib
// - Makes some assumptions based on fixed size and pixel orientation
class Graphics {
public:

  static const uint8_t kWidth = 128;
  static const uint8_t kHeight = 64;
  static const size_t kFrameSize = kWidth * kHeight / 8;

  static const weegfx::coord_t kFixedFontW = 6;
  static const weegfx::coord_t kFixedFontH = 8;

  void Init();

  void Begin(uint8_t *frame, bool clear_frame);
  void End();

  // Pseudo-compatible functions with u8lib
  void setDefaultBackgroundColor() { };
  void setDefaultForegroundColor() { };

  // All Drawing is with foreground color except where otherwise noted

  inline void setPixel(coord_t x, coord_t y) __attribute__((always_inline));

  void drawRect(coord_t x, coord_t y, coord_t w, coord_t h);
  void clearRect(coord_t x, coord_t y, coord_t w, coord_t h);
  void invertRect(coord_t x, coord_t y, coord_t w, coord_t h);
  void drawFrame(coord_t x, coord_t y, coord_t w, coord_t h);

  void drawHLine(coord_t x, coord_t y, coord_t w);
  void drawVLine(coord_t x, coord_t y, coord_t h);

  void drawLine(coord_t x1, coord_t y1, coord_t x2, coord_t y2);

  void drawBitmap8(coord_t x, coord_t y, coord_t w, const uint8_t *data);

  // Beware: No clipping
  void drawCircle(coord_t center_x, coord_t center_y, coord_t r);

  void setPrintPos(coord_t x, coord_t y);
  void movePrintPos(coord_t dx, coord_t dy);

  void print(char);
  void print(int);
  void print(int, unsigned width);
  void print(uint16_t, unsigned width);
  void print(uint32_t, unsigned width);
  void print(long);

  void pretty_print(int);
  void pretty_print(int, unsigned width);

  // Print right-aligned number at current print pos; print pos is unchanged  
  void pretty_print_right(int);

  // Print string at current print pos and move print pos
  void print(const char *);

  // Print right-aligned string at current print pos; print pos is unchanged
  void print_right(const char *);

  // Print string at absolute coords, doesn't move print pos
  void drawStr(coord_t x, coord_t y, const char *str);

  // Might be time-consuming
  void printf(const char *fmt, ...);

private:
  uint8_t *frame_;

  coord_t text_x_;
  coord_t text_y_;

  inline uint8_t *get_frame_ptr(const coord_t x, const coord_t y) __attribute__((always_inline));
  void draw_char(char c, coord_t x, coord_t y);
};

inline void Graphics::setPixel(coord_t x, coord_t y) {
  *(get_frame_ptr(x, y)) |= (0x1 << (y & 0x7));
}

inline void Graphics::setPrintPos(coord_t x, coord_t y) {
  text_x_ = x;
  text_y_ = y;
}

inline void Graphics::movePrintPos(coord_t dx, coord_t dy) {
  text_x_ += dx;
  text_y_ += dy;
}

inline uint8_t *Graphics::get_frame_ptr(const coord_t x, const coord_t  y) {
  return frame_ + ((y >>3) << 7) + x;
}

};

#endif // WEEGFX_H_
