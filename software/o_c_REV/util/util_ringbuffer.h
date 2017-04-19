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

#ifndef UTIL_RINGBUFFER_H_
#define UTIL_RINGBUFFER_H_

#include <stdint.h>
#include "util_macros.h"

namespace util {

// There seem to be two types of common ring buffer implementations:
// One that doesn't use the last item in order to distinguish between "empty"
// and "full" states (e.g. MI stmlib/utils/ring_buffer.h). The other relies on
// wrapping read/write heads and appears to use all available items.
// This implements the latter.
// - Pretty much assumes single producer/consumer
// - Assume size is pow2
//
template <typename T, size_t size>
class RingBuffer {
public:
  RingBuffer() { }

  void Init() {
    write_ptr_ = read_ptr_ = poke_ptr_ = 0;
  }

  inline size_t readable() const {
    return write_ptr_ - read_ptr_;
  }

  inline size_t writable() const {
    return size - readable();
  }

  inline T Read() {
    size_t read_ptr = read_ptr_;
    T value = buffer_[read_ptr & (size - 1)];
    read_ptr_ = read_ptr + 1;
    return value;
  }

  inline void Write(T value) {
    size_t write_ptr = write_ptr_;
    buffer_[write_ptr & (size - 1)] = value;
    poke_ptr_ = write_ptr_ = write_ptr + 1;
  }

  inline void Flush() {
    write_ptr_ = read_ptr_ = 0;
  }

  inline T Poke(size_t index_offset) {
    size_t read_ptr = (poke_ptr_ - 1) - index_offset;
    T value = buffer_[read_ptr & (size - 1)];
    return value;
  } 

  inline void Freeze(size_t buf_size) {
    size_t start_ptr = (write_ptr_ - buf_size);
    poke_ptr_ = (poke_ptr_ >= write_ptr_) ? start_ptr : poke_ptr_;
    poke_ptr_++;
  } 

private:

  T buffer_[size];
  volatile size_t write_ptr_;
  volatile size_t read_ptr_;
  volatile size_t poke_ptr_;

  DISALLOW_COPY_AND_ASSIGN(RingBuffer);
};

};

#endif // UTIL_RINGBUFFER_H_