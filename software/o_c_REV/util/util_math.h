#ifndef UTIL_MATH_H_
#define UTIL_MATH_H_

// Woo. Funky macro magic to avoid dividing by non-power-of-two.
// Essentially a quick fixed-point calculation, but only valid up to 2^exp

#define FAST_FP_DIV(n, div, exp) \
  (((n) * (((1 << exp) + 1) / div)) >> exp)

#define FAST_FP_MOD(n, div, exp) \
  ((n) - FAST_FP_DIV(n, div, exp) * div)

#define DIV_8(n, div) \
  FAST_FP_DIV(n, div, 8)

#define MOD_8(n, div) \
  FAST_FP_MOD(n, div, 8)

inline uint32_t USAT16(uint32_t value) __attribute__((always_inline));
inline uint32_t USAT16(uint32_t value) {
  uint32_t result;
  __asm("usat %0, %1, %2" : "=r" (result) : "I" (16), "r" (value));
  return result;
}

inline uint32_t USAT16(int32_t value) __attribute__((always_inline));
inline uint32_t USAT16(int32_t value) {
  uint32_t result;
  __asm("usat %0, %1, %2" : "=r" (result) : "I" (16), "r" (value));
  return result;
}

template <typename T, T smoothing>
struct SmoothedValue {
  SmoothedValue() : value_(0) { }

  T value_;

  T value() const {
    return value_;
  }

  void push(T value) {
    value_ = (value_ * (smoothing - 1) + value) / smoothing;
  }
};


#define SCALE8_16(x) ((((x + 1) << 16) >> 8) - 1)


#endif // UTIL_MATH_H_
