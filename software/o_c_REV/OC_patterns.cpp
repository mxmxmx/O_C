#include "OC_patterns.h"
#include "OC_patterns_presets.h"

namespace OC {

    Pattern user_patterns[Patterns::PATTERN_USER_LAST];
    Pattern dummy_pattern;

    /*static*/
    const int Patterns::NUM_PATTERNS = OC::Patterns::PATTERN_USER_LAST + sizeof(OC::patterns) / sizeof(OC::patterns[0]);

    /*static*/
    void Patterns::Init() {
      for (size_t i = 0; i < PATTERN_USER_LAST; ++i)
        memcpy(&user_patterns[i], &OC::patterns[0], sizeof(Pattern));
    }

    /*static*/
    const Pattern &Patterns::GetPattern(int index) {
      if (index < PATTERN_USER_LAST)
        return user_patterns[index];
      else
        return OC::patterns[index - PATTERN_USER_LAST];
    }

    const char* const pattern_names_short[] = {
        "SEQ-1",
        "SEQ-2",
        "SEQ-3",
        "SEQ-4",
        "DEFAULT"
    };

    const char* const pattern_names[] = {
        "User-defined 1",
        "User-defined 2",
        "User-defined 3",
        "User-defined 4",
        "DEFAULT"
    }; 
} // namespace OC
