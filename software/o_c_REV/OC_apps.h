#ifndef OC_APP_H_
#define OC_APP_H_

#include "util/util_misc.h"

namespace OC {

enum AppEvent {
  APP_EVENT_SUSPEND,
  APP_EVENT_RESUME,
  APP_EVENT_SCREENSAVER
};

// This is a very poor-man's application "switching" framework. The main UI/
// drawing functions are mostly unchanged and just call into the current_app
// hooks.
// This does mean some extra call overhead (function pointer vs. function) but
// with some re-organization this might be avoidable, or could be embraced with
// interface class and virtual functions instead.
//
struct App {
  uint16_t id;
	const char *name;

  void (*Init)(); // one-time init
  size_t (*storageSize)();
  size_t (*Save)(void *);
  size_t (*Restore)(const void *);

  void (*handleEvent)(AppEvent); // Generic event handler

  void (*loop)(); // main loop function
  void (*draw_menu)(); 
  void (*draw_screensaver)();

  void (*top_button)();
  void (*lower_button)();
  void (*right_button)();
  void (*left_button)();
  void (*left_button_long)();
  bool (*update_encoders)();

  void (*isr)();
};

extern App *current_app;

namespace APPS {

  void Init(bool use_defaults);
  void Select();

  inline void ISR() __attribute__((always_inline));
  inline void ISR() {
    if (current_app && current_app->isr)
      current_app->isr();
  }

  App *find(uint16_t id);
  int index_of(uint16_t id);
};

};

#endif // OC_APP_H_
