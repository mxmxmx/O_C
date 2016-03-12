#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include <stdint.h>

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

#define SERIAL_PRINTLN(msg, ...) \
	serial_printf(msg "\n", ##__VA_ARGS__)

#endif // UTIL_MISC_H
