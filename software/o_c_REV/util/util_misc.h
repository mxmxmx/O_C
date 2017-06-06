#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include <stdint.h>
#include "../OC_options.h"

template <uint32_t a, uint32_t b, uint32_t c, uint32_t d>
struct FOURCC
{
  static const uint32_t value = ((a&0xff) << 24) | ((b&0xff) << 16) | ((c&0xff) << 8) | (d&0xff);
};

template <uint32_t a, uint32_t b>
struct TWOCC
{
	static constexpr uint16_t value = (((a & 0xff) << 8) | (b & 0xff));
};

void serial_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#ifdef PRINT_DEBUG
#undef SERIAL_PRINTLN
#define SERIAL_PRINTLN(msg, ...) \
	serial_printf(msg "\n", ##__VA_ARGS__)
#else // PRINT_DEBUG 
#undef SERIAL_PRINTLN
#define SERIAL_PRINTLN(msg, ...) 
#endif // PRINT_DEBUG

namespace util {
inline uint8_t reverse_byte(uint8_t b) {
  return ((b & 0x1)  << 7) | ((b & 0x2)  << 5) |
         ((b & 0x4)  << 3) | ((b & 0x8)  << 1) |
         ((b & 0x10) >> 1) | ((b & 0x20) >> 3) |
         ((b & 0x40) >> 5) | ((b & 0x80)  >> 7);
}
};

#endif // UTIL_MISC_H
