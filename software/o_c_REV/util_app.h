#ifndef UTIL_APP_H_
#define UTIL_APP_H_

// This is a very poor-man's application "switching" framework. The main UI/
// drawing functions are mostly unchanged and just call into the current_app
// hooks.
// This does mean some extra call overhead (function pointer vs. function) but
// with some re-organization this might be avoidable, or could be embraced with
// interface class and virtual functions instead.
//
struct App {
	const char *name;
  void (*init)(); // one-time init
  void (*loop)(); // main loop function
  void (*suspend)();
  void (*resume)();

  void (*render_menu)(); 
  void (*screensaver)();

  void (*top_button)();
  void (*lower_button)();
  void (*right_button)();
  void (*left_button)();
  bool (*update_encoders)();
};

extern App *current_app;
extern bool SELECT_APP;
void init_apps();
void select_app();

#endif // UTIL_APP_H_
