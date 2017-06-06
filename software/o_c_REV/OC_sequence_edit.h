#ifndef OC_SEQUENCE_EDIT_H_
#define OC_SEQUENCE_EDIT_H_

#include "OC_bitmaps.h"
#include "OC_patterns.h"
#include "OC_patterns_presets.h"
#include "OC_options.h"

namespace OC {

// Pattern editor
// based on scale editor written by Patrick Dowling, adapted for TU, re-adapted for OC
//

template <typename Owner>
class PatternEditor {
public:

  void Init() {
    owner_ = nullptr;
    pattern_name_ = "?!";
    pattern_ = mutable_pattern_ = &dummy_pattern;
    mask_ = 0;
    cursor_pos_ = 0;
    num_slots_ = 0;
    edit_this_sequence_ = 0;
  }

  bool active() const {
    return nullptr != owner_;
  }

  void Edit(Owner *owner, int pattern) {
    if (OC::Patterns::PATTERN_NONE == pattern)
      return;

    pattern_ = mutable_pattern_ = &OC::user_patterns[pattern];
    pattern_name_ = OC::pattern_names_short[pattern];
    // Serial.print("Editing user pattern "); 
    // Serial.println(pattern_name_);
    owner_ = owner;

    BeginEditing();
  }

  void Close();

  void Draw();
  void HandleButtonEvent(const UI::Event &event);
  void HandleEncoderEvent(const UI::Event &event);
  static uint16_t RotateMask(uint16_t mask, int num_slots, int amount);

private:

  Owner *owner_;
  const char * pattern_name_;
  const OC::Pattern *pattern_;
  Pattern *mutable_pattern_;

  uint16_t mask_;
  int8_t edit_this_sequence_;
  size_t cursor_pos_;
  size_t num_slots_;

  void BeginEditing();

  void move_cursor(int offset);
  void toggle_mask();
  void invert_mask();
  void clear_mask(); 
  void copy_sequence(); 
  void paste_sequence(); 

  void apply_mask(uint16_t mask) {
 
    if (mask_ != mask) {
      mask_ = mask;
      owner_->update_pattern_mask(mask_, edit_this_sequence_);
    }
    
    bool force = (owner_->get_current_sequence() == edit_this_sequence_);
    owner_->pattern_changed(mask, force);
  }
  
  void change_slot(size_t pos, int delta, bool notify);
  void handleButtonLeft(const UI::Event &event);
  void handleButtonUp(const UI::Event &event);
  void handleButtonDown(const UI::Event &event);
};

template <typename Owner>
void PatternEditor<Owner>::Draw() {
  size_t num_slots = num_slots_;

  weegfx::coord_t const w = 128;
  weegfx::coord_t const h = 64;
  weegfx::coord_t x = 0;
  weegfx::coord_t y = 0;
  graphics.clearRect(x, y, w, h);
  graphics.drawFrame(x, y, w, h);

  x += 2;
  y += 3;

  graphics.setPrintPos(x, y);
  
  uint8_t id = edit_this_sequence_;

  if (edit_this_sequence_ == owner_->get_sequence())
    graphics.printf("#%d", id + 1);
  else {
    graphics.drawBitmap8(x, y, 4, OC::bitmap_indicator_4x8);
    graphics.setPrintPos(x + 4, y); 
    graphics.print(id + 1);
  }

  if (cursor_pos_ == num_slots) {
    // print length
    graphics.print(":");
    if (num_slots > 9)
        graphics.print((int)num_slots, 2);
      else 
        graphics.print((int)num_slots, 1);  
  }
  else {
    // print pitch value at current step  ... 
    // 0 -> 0V, 1536 -> 1V, 3072 -> 2V
    int pitch = (int)owner_->get_pitch_at_step(edit_this_sequence_, cursor_pos_);
    int32_t octave = pitch / (12 << 7);
    int32_t frac = pitch - octave * (12 << 7);
    if (pitch >= 0) {
      octave += owner_->get_octave();
      if (octave >= 0)
        graphics.printf(": +%d+%.2f", octave, (float)frac / 128.0f);
      else
        graphics.printf(": %d+%.2f", octave, (float)frac / 128.0f);
    }
    else {
      octave--;
      if (!frac) {
        octave++;
        frac = - 12 << 7;
      }
      graphics.printf(": %d+%.2f", octave + owner_->get_octave(), 12.0f + ((float)frac / 128.0f));
    }
  }

  x += 3 + (w >> 0x1) - (num_slots << 0x2); y += 40;
  #ifdef BUCHLA_4U
    y += 16;
  #endif 

  uint8_t clock_pos= owner_->get_clock_cnt();
  bool _draw_clock = (owner_->get_current_sequence() == edit_this_sequence_) && owner_->draw_clock();
  uint16_t mask = mask_;
  
  for (size_t i = 0; i < num_slots; ++i, x += 7, mask >>= 1) {

    int pitch = (int)owner_->get_pitch_at_step(edit_this_sequence_, i);
    
    bool _clock = (i == clock_pos && _draw_clock);
     
    if (mask & 0x1 & (pitch >= 0)) {
      pitch += 0x100;
      if (_clock)
        graphics.drawRect(x - 1, y - (pitch >> 8), 6, pitch >> 8);
      else
        graphics.drawRect(x, y - (pitch >> 8), 4, pitch >> 8);
    }
    else if (mask & 0x1) {
      pitch -= 0x100;
      if (_clock)
        graphics.drawRect(x - 1, y, 6, abs(pitch) >> 8);
      else 
        graphics.drawRect(x, y, 4, abs(pitch) >> 8);
    }
    else if (pitch > - 0x200 && pitch < 0x200) {
     // disabled steps not visible otherwise..
     graphics.drawRect(x + 1, y - 1, 2, 2);
    }
    else if (pitch >= 0) {
      pitch += 0x100;
      graphics.drawFrame(x, y - (pitch >> 8), 4, pitch >> 8);
    }
    else {
      pitch -= 0x100;
      graphics.drawFrame(x, y, 4, abs(pitch) >> 8);
    }

    if (i == cursor_pos_) {
      if (OC::ui.read_immediate(OC::CONTROL_BUTTON_L))
        graphics.drawFrame(x - 3, y - 5, 10, 10);
      else 
        graphics.drawFrame(x - 2, y - 4, 8, 8);
    }
      
    if (_clock)
      graphics.drawRect(x, y + 17, 4, 2);
       
  }
  if (mutable_pattern_) {
     graphics.drawFrame(x, y - 2, 4, 4);
    if (cursor_pos_ == num_slots)
      graphics.drawFrame(x - 2, y - 4, 8, 8);
  }
}

template <typename Owner>
void PatternEditor<Owner>::HandleButtonEvent(const UI::Event &event) {

   if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        handleButtonUp(event);
        break;
      case OC::CONTROL_BUTTON_DOWN:
        handleButtonDown(event);
        break;
      case OC::CONTROL_BUTTON_L:
        handleButtonLeft(event);
        break;    
      case OC::CONTROL_BUTTON_R:
        Close();
        break;
      default:
        break;
    }
  }
  else if (UI::EVENT_BUTTON_LONG_PRESS == event.type) {
     switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        // screensaver    // TODO: ideally, needs to be overridden ... invert_mask();
      break;
      case OC::CONTROL_BUTTON_DOWN:
      {
        if (OC::ui.read_immediate(OC::CONTROL_BUTTON_L)) 
          clear_mask();
        else
          paste_sequence(); 
      }
      break;
      case OC::CONTROL_BUTTON_L: 
      {
        if (OC::ui.read_immediate(OC::CONTROL_BUTTON_DOWN)) 
          clear_mask();
        else
          copy_sequence();
      }
      break;
      case OC::CONTROL_BUTTON_R:
       // app menu
      break;  
      default:
      break;
     }
  }
}

template <typename Owner>
void PatternEditor<Owner>::HandleEncoderEvent(const UI::Event &event) {
 
  uint16_t mask = mask_;

  if (OC::CONTROL_ENCODER_L == event.control) {
    move_cursor(event.value);
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    bool handled = false;
    if (mutable_pattern_) {
      if (cursor_pos_ >= num_slots_) {
       
        if (cursor_pos_ == num_slots_) {
          int num_slots = num_slots_;
          num_slots += event.value;
          CONSTRAIN(num_slots, kMinPatternLength, kMaxPatternLength);

          num_slots_ = num_slots;
           if (event.value > 0) {
            // erase  slots when expanding?
            if (OC::ui.read_immediate(OC::CONTROL_BUTTON_L)) {
               mask &= ~(~(0xffff << (num_slots_ - cursor_pos_)) << cursor_pos_);
               owner_->set_pitch_at_step(edit_this_sequence_, num_slots_, 0x0); 
            }
          } 
          // empty patterns are ok -- 
          owner_->set_sequence_length(num_slots_, edit_this_sequence_);
          cursor_pos_ = num_slots_;
          handled = true;
        }
      }
    }

    if (!handled) {
      
      int32_t pitch = owner_->get_pitch_at_step(edit_this_sequence_, cursor_pos_);
      // Q? might be better to actually add whatever is in the scale
      // or semitone/finetune?
      int16_t delta = event.value;
      if (OC::ui.read_immediate(OC::CONTROL_BUTTON_L)) 
       pitch += delta; // fine
      else
        pitch += (delta << 7); // semitone
      #ifdef BUCHLA_4U
        CONSTRAIN(pitch, 0x0, 8 * (12 << 7)  - 128); 
      #else 
        CONSTRAIN(pitch, -3 * (12 << 7), 5 * (12 << 7)  - 128); 
      #endif 
      owner_->set_pitch_at_step(edit_this_sequence_, cursor_pos_, pitch);

    }
  }
  // This isn't entirely atomic
  apply_mask(mask);
}

template <typename Owner>
void PatternEditor<Owner>::move_cursor(int offset) {

  int cursor_pos = cursor_pos_ + offset;
  const int max = mutable_pattern_ ? num_slots_ : num_slots_ - 1;
  CONSTRAIN(cursor_pos, 0, max);
  cursor_pos_ = cursor_pos;
}

template <typename Owner>
void PatternEditor<Owner>::handleButtonUp(const UI::Event &event) {

    // next pattern / edit 'offline':
    edit_this_sequence_++;
    if (edit_this_sequence_ > OC::Patterns::PATTERN_USER_LAST-1)
      edit_this_sequence_ = 0;
      
    cursor_pos_ = 0;
    num_slots_ = owner_->get_sequence_length(edit_this_sequence_);
    mask_ = owner_->get_mask(edit_this_sequence_);
}

template <typename Owner>
void PatternEditor<Owner>::handleButtonDown(const UI::Event &event) {
  
    // next pattern / edit 'offline':
    edit_this_sequence_--;
    if (edit_this_sequence_ < 0)
      edit_this_sequence_ = OC::Patterns::PATTERN_USER_LAST-1;
      
    cursor_pos_ = 0;
    num_slots_ = owner_->get_sequence_length(edit_this_sequence_);
    mask_ = owner_->get_mask(edit_this_sequence_);
}

template <typename Owner>
void PatternEditor<Owner>::handleButtonLeft(const UI::Event &) {
  uint16_t m = 0x1 << cursor_pos_;
  uint16_t mask = mask_;

  if (cursor_pos_ < num_slots_) {
    // toggle slot active state; allow 0 mask
    if (mask & m)
      mask &= ~m;
    else 
      mask |= m;
    apply_mask(mask);
  }
}

template <typename Owner>
void PatternEditor<Owner>::invert_mask() {
  uint16_t m = ~(0xffffU << num_slots_);
  uint16_t mask = mask_ ^ m;
  apply_mask(mask);
}

template <typename Owner>
void PatternEditor<Owner>::clear_mask() {
  // clear the mask
  apply_mask(0x00);
  // and the user pattern:
  owner_->clear_user_pattern(edit_this_sequence_);
}

template <typename Owner>
void PatternEditor<Owner>::copy_sequence() {
  owner_->copy_seq(edit_this_sequence_, num_slots_, mask_);
}

template <typename Owner>
void PatternEditor<Owner>::paste_sequence() {
  uint8_t newslots = owner_->paste_seq(edit_this_sequence_);
  num_slots_ = newslots ? newslots  : num_slots_;
  mask_ = owner_->get_mask(edit_this_sequence_);
}

template <typename Owner>
/*static*/ uint16_t PatternEditor<Owner>::RotateMask(uint16_t mask, int num_slots, int amount) {
  uint16_t used_bits = ~(0xffffU << num_slots);
  mask &= used_bits;

  if (amount < 0) {
    amount = -amount % num_slots;
    mask = (mask >> amount) | (mask << (num_slots - amount));
  } else {
    amount = amount % num_slots;
    mask = (mask << amount) | (mask >> (num_slots - amount));
  }
  return mask | ~used_bits; // fill upper bits
}

template <typename Owner>
void PatternEditor<Owner>::BeginEditing() {

  cursor_pos_ = 0;
  uint8_t seq = owner_->get_sequence();
  edit_this_sequence_ = seq;
  num_slots_ = owner_->get_sequence_length(seq);
  mask_ = owner_->get_mask(seq);
}

template <typename Owner>
void PatternEditor<Owner>::Close() {
  ui.SetButtonIgnoreMask();
  owner_ = nullptr;
}

}; // namespace OC

#endif // OC_PATTERN_EDIT_H_
