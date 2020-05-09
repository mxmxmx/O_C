#ifndef OC_UI_H_
#define OC_UI_H_

#include "OC_config.h"
#include "OC_options.h"
#include "OC_debug.h"
#include "UI/ui_button.h"
#include "UI/ui_encoder.h"
#include "UI/ui_event_queue.h"

namespace OC {

enum EncoderConfig : uint32_t;
struct App;

// UI::Event::control is uint16_t, but we only have 6 controls anyway.
// So we can helpfully make things into bitmasks, which seems useful.
enum UiControl {
  CONTROL_BUTTON_UP   = 0x1,
  CONTROL_BUTTON_DOWN = 0x2,
  CONTROL_BUTTON_L    = 0x4,
  CONTROL_BUTTON_R    = 0x8,
  CONTROL_BUTTON_M    = 0x10,

  CONTROL_ENCODER_L   = 0x20,
  CONTROL_ENCODER_R   = 0x40,

  #ifdef VOR
  CONTROL_LAST = 6,
  CONTROL_BUTTON_LAST = 5,
  #else
  CONTROL_LAST = 5,
  CONTROL_BUTTON_LAST = 4,
  #endif
};

static inline uint16_t control_mask(unsigned i) {
  return 1 << i;
}

enum UiMode {
  UI_MODE_SCREENSAVER,
  UI_MODE_MENU,
  UI_MODE_APP_SETTINGS,
  UI_MODE_CALIBRATE
};

class Ui {
public:
  static const size_t kEventQueueDepth = 16;
  static const uint32_t kLongPressTicks = 1000;

  Ui() { }

  void Init();

  UiMode Splashscreen(bool &reset_settings);
  bool ConfirmReset();
  void DebugStats();
  void Calibrate();
  void AppSettings();
  UiMode DispatchEvents(OC::App *app);

  void Poll();
  void _Poke();
  void _preemptScreensaver(bool v);

  inline bool read_immediate(UiControl control) {
    return button_state_ & control;
  }

  inline void encoders_enable_acceleration(bool enable) {
    encoder_left_.enable_acceleration(enable);
    encoder_right_.enable_acceleration(enable);
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

  void configure_encoders(EncoderConfig encoder_config);

  inline uint32_t idle_time() const {
    return event_queue_.idle_time();
  }

  inline uint32_t ticks() const {
    return ticks_;
  }

  inline void SetButtonIgnoreMask() {
    button_ignore_mask_ = button_state_;
  }

  inline void IgnoreButton(UiControl control) {
    button_ignore_mask_ |= control;
  }

  uint32_t screensaver_timeout() const {
    return screensaver_timeout_;
  }

  void set_screensaver_timeout(uint32_t seconds);

private:

  uint32_t ticks_;
  uint32_t screensaver_timeout_;

  UI::Button buttons_[CONTROL_BUTTON_LAST];
  uint32_t button_press_time_[CONTROL_BUTTON_LAST];
  uint16_t button_state_;
  uint16_t button_ignore_mask_;
  bool screensaver_;
  bool preempt_screensaver_;

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

  bool IgnoreEvent(const UI::Event &event) {
    bool ignore = false;
    if (button_ignore_mask_ & event.control) {
      button_ignore_mask_ &= ~event.control;
      ignore = true;
    }
    if (screensaver_) {
      screensaver_ = false;
      ignore = true;
    }

    return ignore;
  }

};

extern Ui ui;

}; // namespace OC

#endif // OC_UI_H_
