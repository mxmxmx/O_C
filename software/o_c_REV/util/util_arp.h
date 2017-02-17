// Copyright 2013 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
// 


// see https://github.com/pichenettes/eurorack/blob/master/yarns/part.cc#L364
// adapted for o_C

#ifndef UTIL_ARP_H_
#define UTIL_ARP_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <Arduino.h>


enum ArpeggiatorDirection {
  ARPEGGIATOR_DIRECTION_UP,
  ARPEGGIATOR_DIRECTION_DOWN,
  ARPEGGIATOR_DIRECTION_UP_DOWN,
  ARPEGGIATOR_DIRECTION_RANDOM,
  ARPEGGIATOR_DIRECTION_LAST
};

// ???
const uint16_t lut_arpeggiator_patterns[] = {
   21845,  62965,  46517,  54741,
   43861,  22869,  38293,   2313,
   37449,  21065,  18761,  54553,
   27499,  23387,  30583,  28087,
   22359,  28527,  30431,  43281,
   28609,  53505,
};

namespace util {

class Arpeggiator {
public:

  void Init() {

    arp_step_ = 0x0;
    arp_note_ = 0x0;
    arp_octave_ = 0x0;
    arp_direction_ = 0x1;
    arp_direction_setting_ = ARPEGGIATOR_DIRECTION_UP;
    arp_range_ = 0x1;
    arp_num_notes_ = 0x1;

    for (int i = 0; i < 16; i++)
      note_stack_[i] = 0x0; 
  }

  int32_t ClockArpeggiator() {

  int32_t note;
  uint8_t num_notes;

  note = 0x0;
  num_notes = arp_num_notes_;

  if (num_notes) {

    // Update arpeggiator note/octave counter.
    if (num_notes == 1 && arp_range_ == 1) {
      // This is a corner case for the Up/down pattern code.
      // Get it out of the way.
      arp_note_ = 0;
      arp_octave_ = 0;
    } else {

      if (arp_direction_setting_ == ARPEGGIATOR_DIRECTION_RANDOM) {
        uint16_t _random = random(0xFFFF); // ?? 
        arp_octave_ = (_random & 0xFF) % arp_range_;
        arp_note_ = (_random >> 8) % num_notes;
      } 
      else {

        bool wrapped = true;
        while (wrapped) {

          if (arp_note_ >= num_notes || arp_note_ < 0) {
            arp_octave_ += arp_direction_;
            arp_note_ = arp_direction_ > 0 ? 0 : num_notes - 1;
          }

          wrapped = false;

          if (arp_octave_ >= arp_range_ || arp_octave_ < 0) {
            arp_octave_ = arp_direction_ > 0 ? 0 : arp_range_ - 1;

            if (arp_direction_setting_ == ARPEGGIATOR_DIRECTION_UP_DOWN) {
              arp_direction_ = -arp_direction_;
              arp_note_ = arp_direction_ > 0 ? 1 : num_notes - 2;
              arp_octave_ = arp_direction_ > 0 ? 0 : arp_range_ - 1;
              wrapped = true;
            }
          }
        }
      }
    }
    
    note = sorted_notes(arp_note_);
    note += (arp_octave_ * 12 << 7);
    arp_note_ += arp_direction_;
    ++arp_step_;
 }
 else if (!num_notes)
   note = 0xFFFFFF; // no notes >> pause 

 return note;
}

void UpdateArpeggiator(int channel_id, int sequence_id, int sequence_mask, int sequence_length) {

    // sort notes:
    uint8_t _channel_offset = !channel_id ? 0x0 : OC::Patterns::NUM_PATTERNS;
    OC::Pattern *arp_pattern_ = &OC::user_patterns[sequence_id + _channel_offset];
    for (int i = 0; i < sequence_length; i++) 
      note_stack_[i] = arp_pattern_->notes[i];
    //memcpy(note_stack_, arp_pattern_->notes, sizeof(OC::Pattern)); // ??
    arp_num_notes_ = sort_notes(note_stack_, sequence_length, sequence_mask);
}

void set_direction(int8_t direction) {

  if (arp_direction_setting_ != direction) {

    switch (direction) {

      case ARPEGGIATOR_DIRECTION_UP:
      case ARPEGGIATOR_DIRECTION_UP_DOWN:
      case ARPEGGIATOR_DIRECTION_RANDOM:
      arp_direction_ = 0x1;
      break; 
      case ARPEGGIATOR_DIRECTION_DOWN:
      arp_direction_ = -0x1;
      break; 
      default:
      break;
      }
  }
  arp_direction_setting_ = direction;
  
}

void set_range(int8_t range) {
  arp_range_ = range + 1;
}

void reset() {
  arp_step_ = arp_note_ = 0x0;
}

// brute bubble ... 
uint8_t sort_notes(int32_t *stack, int seq_length, uint16_t mask) {

  int i, j;
  int32_t temp;

  // remove empty slots:
  for(i = 0 ; i < seq_length; i++) {
    if (!(1u & (mask >> i))) 
      stack[i] = 0xFFFF;
  }

  // sort sequence:
  for(i = 0; i < seq_length; i++) {
    
    for(j = seq_length - 1; j > i; j--) {

      if (stack[j] == stack[j - 1]) {
      // filter duplicates:  
        stack[j] = stack[seq_length-1];
        seq_length--;
      }
      else if(stack[j] < stack[j - 1]) {
      // swap
          temp = stack[j-1];
          stack[j-1] = stack[j];
          stack[j] = temp;
      }
    }
  }
  // adjust length:
  if (stack[seq_length - 1] == 0xFFFF) 
      seq_length--;

  return seq_length;
} 

private:

  uint8_t arp_step_;
  int8_t arp_note_;
  int8_t arp_num_notes_;
  int8_t arp_octave_;
  int8_t arp_direction_;
  int8_t arp_direction_setting_;
  int8_t arp_range_;
  int32_t note_stack_[16];

  int32_t sorted_notes(uint8_t index) {
    return note_stack_[index];
  }
};

}; // namespace util

#endif // UTIL_ARP_H_