#ifndef OS_SCALES_H_
#define OS_SCALES_H_

#include "braids_quantizer.h"

// Common scales and stuff
namespace OC {

typedef braids::Scale Scale;

static constexpr int kMaxScaleLength = 16;
static constexpr int kMinScaleLength = 4;

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

// H1200/A11Z are semitone based, so don't need to go "full quanty" for now.
// They still need some hysteresis though
class SemitoneQuantizer {
public:
  static constexpr int32_t kHysteresis = 16;

  SemitoneQuantizer() { }
  ~SemitoneQuantizer() { }

  void Init() {
    last_pitch_ = 0;
  }

  int32_t Process(int32_t pitch) {
    if ((pitch > last_pitch_ + kHysteresis) || (pitch < last_pitch_ - kHysteresis)) {
      last_pitch_ = pitch;
    } else {
      pitch = last_pitch_;
    }
    return (pitch + 63) >> 7;
  }

private:
  int32_t last_pitch_;
};

extern const char *const scale_names[];
extern const char *const scale_names_short[];
extern Scale user_scales[OC::Scales::SCALE_USER_LAST];
extern Scale dummy_scale;
extern const uint32_t fibseries[48];

};

#endif // OS_SCALES_H_
