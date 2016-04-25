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

#ifndef UTIL_HISTORY_H_
#define UTIL_HISTORY_H_

namespace util {

// Simple history tracking; somewhat like a ringbuffer, but more restricted
// The value at tail_ is the oldest value, but also the most likely to change
// during read.
template <typename T, size_t depth>
class History {
public:
  History() { }
  ~History() { }

  static constexpr size_t kDepth = depth;

  void Init(T initial_value) {
    memset(buffer_, initial_value, sizeof(buffer_));
    last_ = initial_value;
    tail_ = 0;
  }

  // Get values from oldest to newest
  inline void Read(T *dst) const {
    size_t head = tail_;
    size_t count = kDepth - head;
    const T *src = buffer_ + head;
    while (count--)
      *dst++ = *src++;

    src = buffer_;
    count = head;
    while (count--)
      *dst++ = *src++;
  }

  inline void Push(T value) {
    size_t tail = tail_;
    buffer_[tail++] = value;
    if (tail >= kDepth)
      tail_ = 0;
    else
      tail_ = tail;
    last_ = value;
  }

  T last() const {
    return last_;
  }

private:

  T last_;
  volatile size_t tail_;
  T buffer_[kDepth] __attribute__((aligned (4)));

  DISALLOW_COPY_AND_ASSIGN(History);
};

}; // namespace util

#endif // UTIL_HISTORY_H_
