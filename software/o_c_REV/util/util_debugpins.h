#ifndef UTIL_DEBUGPINS_H_
#define UTIL_DEBUGPINS_H_

namespace util {

template <int pin>
struct scoped_debug_pin {
  scoped_debug_pin() {
    digitalWriteFast(pin, HIGH);
  }

  ~scoped_debug_pin() {
    digitalWriteFast(pin, LOW);
  }
};

}; // namespace util

//#define ENABLE_DEBUG_PINS

#ifdef ENABLE_DEBUG_PINS
#define DEBUG_PIN_SCOPE(pin) \
  util::scoped_debug_pin<pin> debug_pin
#else
#define DEBUG_PIN_SCOPE(pin) \
  do {} while (0)
#endif

#endif // UTIL_DEBUGPINS_H_
