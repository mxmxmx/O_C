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

#ifndef UI_EVENTS_QUEUE_H_
#define UI_EVENTS_QUEUE_H_

#include <Arduino.h>
#include "ui_events.h"
#include "../util/util_ringbuffer.h"

namespace UI {

// Event queue for UI events
// Meant for single producer/single consumer setting
//
// Yes, looks similar to stmlib::EventQueue, but hey, it's a queue for UI events.
template <size_t size = 16>
class EventQueue {
public:

  EventQueue() { }

  void Init() {
    events_.Init();
    last_event_time_ = 0;
  }

  inline void Flush() {
    events_.Flush();
  }

  inline bool available() const {
    return events_.readable();
  }

  inline void PushEvent(EventType t, uint16_t c, int16_t v) {
    // TODO EmplaceWrite?
    events_.Write(Event(t, c, v, 0));
    Poke();
  }

  inline void PushEvent(EventType t, uint16_t c, int16_t v, uint16_t m) {
    events_.Write(Event(t, c, v, m));
    Poke();
  }

  inline Event PullEvent() {
    return events_.Read();
  }

  inline void Poke() {
    last_event_time_ = millis();
  }

  inline uint32_t idle_time() const {
    return millis() - last_event_time_;
  }

  // More for debugging purposes
  inline bool writable() const {
    return events_.writable();
  }

private:

  util::RingBuffer<Event, size> events_;
  uint32_t last_event_time_;
};

}; // namespace UI

#endif // UI_EVENTS_QUEUE_H_
