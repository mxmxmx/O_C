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
//

#ifndef UTIL_TRIGGER_DELAY_H_
#define UTIL_TRIGGER_DELAY_H_

namespace util {

// Helper class to manage delayed triggers. The active triggers are maintained
// in a register of bits that is shifted on each Update. Uses an array of
// uint32_t to support "long" delays (in OC: 67 * 60us, i.e. 3xuint32_t)
// Could be special-cased for max_delay < 32 without array overhead.

template <size_t max_delay>
class TriggerDelay {
public:
  TriggerDelay() { }
  ~TriggerDelay() { }

  static constexpr size_t kMaxDelay = max_delay;

  void Init() {
    memset(delays_, 0, sizeof(uint32_t) * kEntries);
  }

  inline void Update() {
    for (size_t i = 0; i < kEntries - 1; ++i) {
      delays_[i] = (delays_[i] >> 1) | (delays_[i + 1] << 31);
    }
    delays_[kEntries - 1] = delays_[kEntries - 1] >> 1;
  }

  inline void Push(size_t delay) {
    const size_t i = delay / 32;
    const size_t b = delay - i * 32;
    delays_[i] |= (0x1 << b);
  }

  inline bool triggered() const {
    return delays_[0] & 0x1;
  }

private:

  static constexpr size_t kEntries = (max_delay + 31) / 32;

  uint32_t delays_[kEntries];
};

}; // namespace util

#endif // UTIL_TRIGGER_DELAY_H_
