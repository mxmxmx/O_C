#ifndef OC_PATTERNS_PRESETS_H_
#define OC_PATTERNS_PRESETS_H_

#include <Arduino.h>

namespace OC {

  struct Pattern {
    // length + mask is stored within app
    int16_t notes[16];
  };


  const Pattern patterns[] = {
    
    // default
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
  
  };
}
#endif // OC_PATTERNS__PRESETS_H_
