#ifndef OS_SCALES_H_
#define OS_SCALES_H_

#include "braids_quantizer.h"

// Common scales and stuff
namespace OC {

typedef braids::Scale Scale;

static constexpr long kMaxScaleLength = 16;
static constexpr long kMinScaleLength = 4;

class Scales {
public:

	static const int NUM_SCALES;

  enum {
    SCALE_USER_0,
    SCALE_USER_1,
    SCALE_USER_2,
    SCALE_USER_3,
    SCALE_USER_LAST, // index 0 in braids::scales
    SCALE_SEMI,
    SCALE_NONE = SCALE_USER_LAST,
  };

  static void Init();
  static const Scale &GetScale(int index);
};

extern const char *const scale_names[];
extern Scale user_scales[OC::Scales::SCALE_USER_LAST];
extern Scale dummy_scale;

};

#endif // OS_SCALES_H_
