#ifndef OC_CORE_H_
#define OC_CORE_H_

#include <stdint.h>
#include "OC_config.h"

namespace OC {
  namespace CORE {
  extern volatile uint32_t ticks;
  extern volatile bool app_isr_enabled;

  }; // namespace CORE
}; // namespace OC

#endif // OC_CORE_H_
