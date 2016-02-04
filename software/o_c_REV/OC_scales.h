#ifndef OS_SCALES_H_
#define OS_SCALES_H_

#include "braids_quantizer.h"

// Common scales and stuff
namespace OC {

typedef braids::Scale Scale;

class Scales {
public:

  enum EUserScale {
    USER_SCALE_0,
    USER_SCALE_1,
    USER_SCALE_2,
    USER_SCALE_3,
    USER_SCALE_LAST
  };

  static void Init();
  static const Scale &GetScale(int index);
};

extern const char *const scale_names[];
extern Scale user_scales[OC::Scales::USER_SCALE_LAST];
extern Scale dummy_scale;

};

#endif // OS_SCALES_H_
