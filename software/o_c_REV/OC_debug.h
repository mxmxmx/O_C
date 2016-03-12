#ifndef OC_DEBUG_H_
#define OC_DEBUG_H_

#include "OC_gpio.h"
#include "util/util_math.h"
#include "util/util_macros.h"
#include "util/util_profiling.h"

namespace OC {

namespace DEBUG {

  void Init();

  extern SmoothedValue<uint32_t, 16> ISR_cycles;
  extern SmoothedValue<uint32_t, 16> UI_cycles;
  extern uint32_t UI_event_count;
  extern uint32_t UI_max_queue_depth;
  extern uint32_t UI_queue_overflow;
};

class DebugPins {
public:
  static void Init() {
    pinMode(OC_GPIO_DEBUG_PIN1, OUTPUT);
    pinMode(OC_GPIO_DEBUG_PIN2, OUTPUT);
    digitalWriteFast(OC_GPIO_DEBUG_PIN1, LOW);
    digitalWriteFast(OC_GPIO_DEBUG_PIN2, LOW);
  }
};

}; // namespace OC

#endif // OC_DEBUG_H
