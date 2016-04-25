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

#include <arm_math.h>
#include "util_macros.h"

namespace util {

// Really basic critical section/mutex/semaphone type thing
class CriticalSection {
public:
  CriticalSection() { }
  void Init() {
    id_ = 0;
  }

  // Release lock
  inline void Unlock() __attribute__((always_inline));

  // Blocking lock
  inline void Lock(uint32_t id) __attribute__((always_inline));

  // Non-blocking lock
  // @return true if locked claimed, false if already taken
  inline bool TryLock(uint32_t id) __attribute__((always_inline));

private:
  volatile uint32_t id_;
  DISALLOW_COPY_AND_ASSIGN(CriticalSection);
};

inline void CriticalSection::Unlock() {
  __DMB();
  id_ = 0;
}

inline void CriticalSection::Lock(uint32_t id) {
  uint32_t owner;
  do {
    owner = __LDREXW(&id_);
  } while (owner || __STREXW(id, &id_));
  __DMB();
}

inline bool CriticalSection::TryLock(uint32_t id) {
  uint32_t owner;
  do {
    owner = __LDREXW(&id_);
    // If lock hasn't been claimed, try and claim it
  } while (!owner && __STREXW(id, &id_));
  if (owner) __CLREX();
  else __DMB();
  return !owner;
}

// Automatic scoped TryLock
template <uint32_t id>
class TryLock {
public:
  TryLock(CriticalSection &critical_section)
  : critical_section_(critical_section)
  , locked_(critical_section.TryLock(id)) {
  }

  ~TryLock() {
    if (locked_)
      critical_section_.Unlock();
  }

  inline bool locked() const {
    return locked_;
  }

private:
  CriticalSection &critical_section_;
  const bool locked_;

  DISALLOW_COPY_AND_ASSIGN(TryLock);
};

// Automatic scoped Lock
template <uint32_t id>
class Lock {
public:
  Lock(CriticalSection &critical_section) 
  : critical_section_(critical_section) {
    critical_section.Lock(id);
  }

  ~Lock() {
    critical_section_.Unlock();
  }

private:
  CriticalSection &critical_section_;
  DISALLOW_COPY_AND_ASSIGN(Lock);
};

};

#endif // UTIL_SYNC_H_
