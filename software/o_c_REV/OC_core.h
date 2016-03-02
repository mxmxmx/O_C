#ifndef OC_CORE_H_
#define OC_CORE_H_

#include <stdint.h>
#include "OC_config.h"

namespace OC {
  namespace CORE {
  extern volatile uint32_t ticks;
  extern volatile bool app_isr_enabled;

  }; // namespace CORE


  struct TickCount {
    TickCount() { }
    void Init() {
      last_ticks = 0;
    }

    uint32_t Update() {
      uint32_t now = OC::CORE::ticks;
      uint32_t ticks = now - last_ticks;
      last_ticks = now;
      return ticks;
    }

    uint32_t last_ticks;
  };
}; // namespace OC

#endif // OC_CORE_H_
