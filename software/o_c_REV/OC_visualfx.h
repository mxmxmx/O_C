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

#ifndef OC_VISUALFX_H_
#define OC_VISUALFX_H_

#include "util/util_history.h"

namespace OC {

namespace vfx {

// Ultra simple history that maintains a scroll position. The history depth is
// allowed to be non pow2 to allow keeping an additonal item to scroll in.
//
// Exercises for the reader:
// - Optional start delay after Push
// - Non-linear scrolling, e.g. using easing LUTs from Frames
// - Variable scroll rate depending on time between Push calls
template <typename T, size_t depth>
class ScrollingHistory {
public:
  ScrollingHistory() { }

  static constexpr size_t kDepth = depth;
  static constexpr uint32_t kScrollRate = (1ULL << 32) / (OC_CORE_ISR_FREQ / 8);

  void Init(T initial_value = 0) {
    scroll_pos_ = 0;
    history_.Init(initial_value);
  }

  void Push(T value) {
    history_.Push(value);
    scroll_pos_ = 0xffffffff;
  }

  void Update() {
    if (scroll_pos_ >= kScrollRate)
      scroll_pos_ -= kScrollRate;
    else
      scroll_pos_ = 0;
  }

  uint32_t get_scroll_pos() const {
    return scroll_pos_ >> 24;
  }

  void Read(T *dest) const {
    return history_.Read(dest);
  }

private:

  uint32_t scroll_pos_;
  util::History<T, kDepth> history_;
};

}; // namespace vfx
}; // namespace OC

#endif // OC_VISUALFX_H_
