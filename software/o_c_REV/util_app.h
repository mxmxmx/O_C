#ifndef UTIL_APP_H_
#define UTIL_APP_H_

struct App {
	const char *name;
  void (*init)();
  void (*loop)();
  void (*render_menu)();
  void (*screensaver)();
  void (*top_button)();
  void (*lower_button)();
  void (*right_button)();
  void (*left_button)();
  bool (*update_encoders)();
};

#endif // UTIL_APP_H_
