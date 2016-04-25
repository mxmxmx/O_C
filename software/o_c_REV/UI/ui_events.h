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

#ifndef UI_EVENTS_H_
#define UI_EVENTS_H_

namespace UI {

enum EventType {
  EVENT_NONE,
  EVENT_BUTTON_PRESS,
  EVENT_BUTTON_LONG_PRESS,
  EVENT_ENCODER
};

// UI event struct
// Yes, looks similar to stmlib::Event but hey, they're UI events.
struct Event {
  EventType type;
  uint16_t control;
  int16_t value;
  uint16_t mask;

  Event() { }
  Event(EventType t, uint16_t c, int16_t v, uint16_t m)
  : type(t), control(c), value(v), mask(m) { }
};

}; // namespace UI

#endif // UI_EVENTS_H_
