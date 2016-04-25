#ifndef OC_DEBUG_H_
#define OC_DEBUG_H_

#include "OC_gpio.h"
#include "util/util_math.h"
#include "util/util_macros.h"
#include "util/util_profiling.h"

namespace OC {

namespace DEBUG {

  void Init();

  extern debug::AveragedCycles ISR_cycles;
  extern debug::AveragedCycles UI_cycles;
  extern debug::AveragedCycles MENU_draw_cycles;

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


#define OC_DEBUG_PROFILE_SCOPE(var) \
  debug::ScopedCycleMeasurement cycles(var)

#define OC_DEBUG_RESET_CYCLES(counter, count, var) \
  do { \
    if (!((counter) & (count - 1))) \
      var.Reset(); \
  } while (0)

#endif // OC_DEBUG_H
