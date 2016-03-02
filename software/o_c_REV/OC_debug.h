#ifndef OC_DEBUG_H_
#define OC_DEBUG_H_

#include "util/util_math.h"
#include "util/util_macros.h"

namespace debug {

  class CycleMeasurement {
  public:

    CycleMeasurement() {
      ARM_DEMCR |= ARM_DEMCR_TRCENA;
      ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
      start_ = ARM_DWT_CYCCNT;
    }

    uint32_t read() {
      return ARM_DWT_CYCCNT - start_;
    }

  private:
    uint32_t start_;

    DISALLOW_COPY_AND_ASSIGN(CycleMeasurement);
  };
};

namespace OC {
  namespace CORE {
  extern SmoothedValue<uint32_t, 16> ISR_cycles;
  };
};

#endif // OC_DEBUG_H
