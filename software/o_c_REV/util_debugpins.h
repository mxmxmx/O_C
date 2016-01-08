#ifndef UTIL_DEBUGPINS_H_
#define UTIL_DEBUGPINS_H_

#ifdef ENABLE_DEBUG_PINS
#define SET_DEBUG_PIN1(value) \
  do { digitalWriteFast(DEBUG_PIN1, value); } while (0)

#define SET_DEBUG_PIN2(value) \
  do { digitalWriteFast(DEBUG_PIN2, value); } while (0)

#define DEBUG_PIN_SCOPE(pin) \
  scoped_debug_pin<pin> debug_pin

#else
#define SET_DEBUG_PIN1(value) \
  do {} while (0)

#define SET_DEBUG_PIN2(value) \
  do {} while (0)

#define DEBUG_PIN_SCOPE(pin) \
  do {} while (0)

#endif

template <int pin>
struct scoped_debug_pin {
  scoped_debug_pin() {
    digitalWriteFast(pin, HIGH);
  }

  ~scoped_debug_pin() {
    digitalWriteFast(pin, LOW);
  }
};

class DebugPins {
public:
  static void Init() {
    pinMode(DEBUG_PIN_1, OUTPUT);
    pinMode(DEBUG_PIN_2, OUTPUT);
    digitalWriteFast(DEBUG_PIN_1, LOW);
    digitalWriteFast(DEBUG_PIN_2, LOW);
  }
};

#endif // UTIL_DEBUGPINS_H_
