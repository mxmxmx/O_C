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
  size_t (*save)(char *);
  size_t (*restore)(const char *);

  void (*suspend)(); // Called before run-time switch
  void (*resume)(); // Called after run-time switch to this app

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
extern bool SELECT_APP;
void init_apps();
void select_app();

#endif // UTIL_APP_H_
