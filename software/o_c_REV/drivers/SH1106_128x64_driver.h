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

#ifndef SH1106_128X64_DRIVER_H_
#define SH1106_128X64_DRIVER_H_

#include <stdint.h>
#include <string.h>

struct SH1106_128x64_Driver {
  static constexpr size_t kFrameSize = 128 * 64 / 8;
  static constexpr size_t kNumPages = 8;
  static constexpr size_t kPageSize = kFrameSize / kNumPages;
  static constexpr uint8_t kDefaultOffset = 2;

  static void Init();
  static void Clear();
  static void Flush();
  static void SendPage(uint_fast8_t index, const uint8_t *data);

  // SH1106 ram is 132x64, so it needs an offset to center data in display.
  // However at least one display (mine) uses offset 0 so it's minimally
  // configurable
  static void AdjustOffset(uint8_t offset);
};

#endif // SH1106_128X64_DRIVER_H_
