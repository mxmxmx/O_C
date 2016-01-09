#ifndef UTIL_MACROS_H_
#define UTIL_MACROS_H_

#include <stdint.h>
#include <string.h>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

#endif // UTIL_MACROS_H_
