#ifndef UTIL_BUTTON_H_
#define UTIL_BUTTON_H_

template <int PIN, uint32_t DEBOUNCE_TIME, uint32_t LONG_DEBOUNCE_TIME = 0>
class TimerDebouncedButton {
public:
  void init() {
    pressed_ = false;
    event_ = long_event_ = false;
  }

  bool read() {
    bool changed = false;
    event_ = long_event_= false;
    const bool pin_state = !digitalReadFast(PIN);
    if (!pressed_ && pin_state) {
      pressed_ = changed = true;
      press_time_ = millis();
    } else if (pressed_ && !pin_state) {
      pressed_ = false;
      changed = true;
      if (LONG_DEBOUNCE_TIME && (millis() - press_time_ > LONG_DEBOUNCE_TIME)) {
        long_event_ = true;
      } else if (millis() - press_time_ > DEBOUNCE_TIME) {
        event_ = true;
      }
    }
    return changed;
  }

  bool pressed() const {
    return pressed_;
  }

  bool event() const {
    return event_;
  }

  bool long_event() const {
    return long_event_;
  }

  bool read_immediate() const {
    return !digitalReadFast(PIN);
  }

protected:
  bool pressed_;
  bool event_;
  bool long_event_;
  uint32_t press_time_;
};

#endif // UTIL_BUTTON_H_
