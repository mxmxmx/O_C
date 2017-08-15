// Copyright (c) 2017 Patrick Dowling
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

#ifndef OC_TRIGGER_DELAYS_H_
#define OC_TRIGGER_DELAYS_H_

#include "OC_digital_inputs.h"
#include "util/util_trigger_delay.h"

namespace OC {

template <uint32_t max_delay>
class TriggerDelays {
public:
  TriggerDelays() { }
  ~TriggerDelays() { }

  void Init() {
    for (auto &d : delays_)
      d.Init();
  }

  uint32_t Process(uint32_t trigger_mask, uint32_t delay_ticks) {
    uint32_t triggered = 0;
    uint32_t mask = 0x1;
    for (auto &d : delays_) {
      d.Update();
      if (trigger_mask & mask)
        d.Push(delay_ticks);
      if (d.triggered())
        triggered |= mask;
      mask <<= 1;
    }
    return triggered;
  }

protected:

  util::TriggerDelay<max_delay> delays_[DIGITAL_INPUT_LAST];

};

}; // namespace OC

#endif // OC_TRIGGER_DELAYS_H_

