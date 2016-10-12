#include <Arduino.h>
#include <stdarg.h>
#include "../../util/util_misc.h"

void serial_printf(const char *fmt, ...) {
  char buf[128];
  va_list args;
  va_start(args, fmt );
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.print(buf);
}
