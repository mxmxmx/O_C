// Copyright (c) 2016 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef UTIL_SYNC_H_
#define UTIL_SYNC_H_

#include <stdint.h>

namespace util {

inline uint32_t ldrexw(volatile uint32_t *addr) __attribute__((always_inline));
inline uint32_t ldrexw(volatile uint32_t *addr) {
  uint32_t result;
  asm volatile ("ldrex %0, [%1]" : "=r" (result) : "r" (addr) );
  return result;
}

inline uint32_t strexw(uint32_t value, volatile uint32_t *addr) __attribute__((always_inline));
inline uint32_t strexw(uint32_t value, volatile uint32_t *addr) {
   uint32_t result;
   asm volatile ("strex %0, %2, [%1]" : "=&r" (result) : "r" (addr), "r" (value) );
   return(result);
}

// Really basic critical section/mutex/semaphone type thing
class CriticalSection {
public:
  CriticalSection() { }
  void Init() {
    id_ = 0;
  }

  inline void Lock(uint32_t id) __attribute__((always_inline));
  inline void Unlock() {
    id_ = 0;
  }

private:
  volatile uint32_t id_;
};

inline void CriticalSection::Lock(uint32_t id) {
  uint32_t owner = 0;
  do {
    owner = ldrexw(&id_);
  } while (owner || strexw(id, &id_));
  // Is DMB required here?
}


template <uint32_t id>
struct Lock {
  Lock(CriticalSection &critical_section) 
  : critical_section_(critical_section) {
    critical_section.Lock(id);
  }

  ~Lock() {
    critical_section_.Unlock();
  }

  CriticalSection &critical_section_;
};

};

#endif // UTIL_SYNC_H_
