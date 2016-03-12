#ifndef OC_UI_H_
#define OC_UI_H_

#include "weegfx.h"
#include "OC_config.h"
#include "OC_debug.h"
#include "UI/ui_button.h"
#include "UI/ui_encoder.h"
#include "UI/ui_event_queue.h"

// TEMP
#include <rotaryplus.h>
enum {
  LEFT, RIGHT
};
// TEMP

extern Rotary encoder[2];
// TEMP

namespace OC {

struct App;

enum UiControl {
  CONTROL_BUTTON_UP,
  CONTROL_BUTTON_DOWN,
  CONTROL_BUTTON_L,
  CONTROL_BUTTON_R,
  CONTROL_ENCODER_L,
  CONTROL_ENCODER_R,
  CONTROL_LAST,
  CONTROL_BUTTON_LAST = CONTROL_ENCODER_L
};

enum UiMode {
  UI_MODE_SCREENSAVER,
  UI_MODE_MENU,
  UI_MODE_SELECT_APP,
  UI_MODE_CALIBRATE
};

class Ui {
public:
  static const size_t kEventQueueDepth = 16;
  static const uint32_t kLongPressTicks = 2000;

  Ui() { }

  void Init();

  void Splashscreen();
  void DebugStats();
  void Calibrate();
  void SelectApp();
  UiMode DispatchEvents(OC::App *app);

  void Poll();

  inline bool read_immediate(UiControl control) {
    if (control < CONTROL_BUTTON_LAST)
      return buttons_[control].read_immediate();
    else
      return false;
  }

  inline void encoder_enable_acceleration(UiControl encoder, bool enable) {
    switch (encoder) {
    case CONTROL_ENCODER_L:
      encoder_left_.enable_acceleration(enable);
      break;
    case CONTROL_ENCODER_R:
      encoder_right_.enable_acceleration(enable);
      break;
    default: break;
    }
  }

  inline uint32_t idle_time() const {
    return event_queue_.idle_time();
  }

private:

  uint32_t ticks_;

  UI::Button buttons_[4];
  uint32_t button_press_time_[4];

  UI::Encoder<encR1, encR2> encoder_right_;
  UI::Encoder<encL1, encL2> encoder_left_;

  UI::EventQueue<kEventQueueDepth> event_queue_;

  inline void PushEvent(UI::EventType t, uint16_t c, int16_t v, uint16_t m) {
#ifdef OC_UI_DEBUG
    if (!event_queue_.writable())
      ++DEBUG::UI_queue_overflow;
    ++DEBUG::UI_event_count;
#endif
    event_queue_.PushEvent(t, c, v, m);
  }
};

extern Ui ui;

}; // namespace OC

extern uint_fast8_t MENU_REDRAW;

void init_circle_lut();
void visualize_pitch_classes(uint8_t *normalized, weegfx::coord_t centerx, weegfx::coord_t centery);

extern weegfx::Graphics graphics;

#endif // OC_UI_H_
