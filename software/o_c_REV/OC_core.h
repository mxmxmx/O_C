#ifndef OC_CORE_H_
#define OC_CORE_H_

#include <stdint.h>
#include "OC_config.h"
#include "util/util_debugpins.h"
#include "util_framebuffer.h"
#include "SH1106_128x64_Driver.h"
#include "weegfx.h"

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

extern weegfx::Graphics graphics;
extern FrameBuffer<SH1106_128x64_Driver::kFrameSize, 2> frame_buffer;

#define GRAPHICS_BEGIN_FRAME(wait) \
do { \
  DEBUG_PIN_SCOPE(DEBUG_PIN_1); \
  uint8_t *frame = NULL; \
  do { \
    if (frame_buffer.writeable()) \
      frame = frame_buffer.writeable_frame(); \
  } while (!frame && wait); \
  if (frame) { \
    graphics.Begin(frame, true); \
    do {} while(0)

#define GRAPHICS_END_FRAME() \
    graphics.End(); \
    frame_buffer.written(); \
  } \
} while (0)

#endif // OC_CORE_H_
