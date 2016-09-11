#ifndef OC_PATTERNS_H_
#define OC_PATTERNS_H_

#include "OC_patterns_presets.h"


namespace OC {

typedef OC::Pattern Pattern;

static constexpr int kMaxPatternLength = 16;
static constexpr int kMinPatternLength = 4;

class Patterns {
public:

  static const int NUM_PATTERNS;

  enum {
    PATTERN_USER_0,
    PATTERN_USER_1,
    PATTERN_USER_2,
    PATTERN_USER_3,
    PATTERN_USER_LAST, 
    PATTERN_DEFAULT,
    PATTERN_NONE = PATTERN_USER_LAST,
  };

  static void Init();
  static const Pattern &GetPattern(int index);
  static const int kMin = kMinPatternLength;
  static const int kMax = kMaxPatternLength;
};


extern const char *const pattern_names[];
extern const char *const pattern_names_short[];
extern Pattern user_patterns[OC::Patterns::PATTERN_USER_LAST];
extern Pattern dummy_pattern;

};

#endif // OC_PATTERNS_H_
