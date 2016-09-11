#ifndef OC_PATTERNS_PRESETS_H_
#define OC_PATTERNS_PRESETS_H_

#include <Arduino.h>

namespace OC {

  struct Pattern {
    size_t num_slots;
  };


  const Pattern patterns[] = {
    // default
    { 16 }
  
  };
}
#endif // OC_PATTERNS__PRESETS_H_
