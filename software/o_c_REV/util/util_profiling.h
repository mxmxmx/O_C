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

#ifndef OC_PROFILING_H_
#define OC_PROFILING_H_

#include "util_macros.h"
#include "../extern/dspinst.h"

namespace debug {

class CycleMeasurement {
public:

  CycleMeasurement() : start_(ARM_DWT_CYCCNT) {
  }

  uint32_t read() const {
    return ARM_DWT_CYCCNT - start_;
  }

  static void Init() {
    ARM_DEMCR |= ARM_DEMCR_TRCENA;
    ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
  }

private:
  uint32_t start_;

  DISALLOW_COPY_AND_ASSIGN(CycleMeasurement);
};

static inline uint32_t cycles_to_us(uint32_t cycles) {
  return multiply_u32xu32_rshift32(cycles, (1ULL << 32) / (F_CPU / 1000000));
}

struct AveragedCycles {
  static constexpr uint32_t kSmoothing = 8;

  AveragedCycles() : value_(0), min_(0xffffffff), max_(0) { }

  uint32_t value_;
  uint32_t min_, max_;

  uint32_t value() const {
    return value_;
  }

  uint32_t min_value() const {
    return min_;
  }

  uint32_t max_value() const {
    return max_;
  }

  void Reset() {
    min_ = 0xffffffff;
    max_ = 0;
  }

  void push(uint32_t value) {
    if (value < min_) min_ = value;
    if (value > max_) max_ = value;
    value_ = (value_ * (kSmoothing - 1) + value) / kSmoothing;
  }
};

class ScopedCycleMeasurement {
public:
  ScopedCycleMeasurement(AveragedCycles &dest)
  : dest_(dest)
  , cycles_() { }

  ~ScopedCycleMeasurement() {
    dest_.push(cycles_.read());
  }

private:
  AveragedCycles &dest_;
  CycleMeasurement cycles_;
};

}; // namespace debug

#endif // OC_PROFILING_H_
