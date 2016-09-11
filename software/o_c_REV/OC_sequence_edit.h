#ifndef OC_SEQUENCE_EDIT_H_
#define OC_SEQUENCE_EDIT_H_

#include "OC_bitmaps.h"
#include "OC_patterns.h"
#include "OC_patterns_presets.h"

namespace OC {

// Pattern editor
// written by Patrick Dowling, adapted for TU, re-adapted for OC
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
      //Serial.print("Editing user pattern "); Serial.println(pattern_name_);
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

  void apply_mask(uint16_t mask) {
 
    if (mask_ != mask) {
      mask_ = mask;
      owner_->update_pattern_mask(mask_, edit_this_sequence_);
    }
    if (owner_->get_sequence() == edit_this_sequence_)
      owner_->pattern_changed(mask);
  }
  
  void change_slot(size_t pos, int delta, bool notify);
  void handleButtonLeft(const UI::Event &event);
  void handleButtonUp(const UI::Event &event);
  void handleButtonDown(const UI::Event &event);
};

template <typename Owner>
void PatternEditor<Owner>::Draw() {
  size_t num_slots = num_slots_;

  static constexpr weegfx::coord_t kMinWidth = 8 * 7;

  weegfx::coord_t w =
    mutable_pattern_ ? (num_slots + 1) * 7 : num_slots * 7;

  if (w < kMinWidth) w = kMinWidth;

  weegfx::coord_t x = 64 - (w + 1)/ 2;
  weegfx::coord_t y = 16 - 3;
  weegfx::coord_t h = 36;

  graphics.clearRect(x, y, w + 4, h);
  graphics.drawFrame(x, y, w + 5, h);

  x += 2;
  y += 3;

  graphics.setPrintPos(x, y);
  
  uint8_t id = edit_this_sequence_;
  
  if (edit_this_sequence_ == owner_->get_sequence())
    id += 4;
  graphics.print(OC::Strings::seq_id[id]);

  graphics.setPrintPos(x, y + 24);
  
  if (cursor_pos_ != num_slots) 
    graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);
  else 
    graphics.print((int)num_slots, 2);

  x += 2; y += 10;
  uint16_t mask = mask_;
  for (size_t i = 0; i < num_slots; ++i, x += 7, mask >>= 1) {
    if (mask & 0x1)
      graphics.drawRect(x, y, 4, 8);
    else
      graphics.drawBitmap8(x, y, 4, bitmap_empty_frame4x8);

    if (i == cursor_pos_)
      graphics.drawFrame(x - 2, y - 2, 8, 12);
  }
  if (mutable_pattern_) {
    graphics.drawBitmap8(x, y, 4, bitmap_end_marker4x8);
    if (cursor_pos_ == num_slots)
      graphics.drawFrame(x - 2, y - 2, 8, 12);
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
    }
  }
  else if (UI::EVENT_BUTTON_LONG_PRESS == event.type) {
     switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        invert_mask();
        break;
      case OC::CONTROL_BUTTON_DOWN:
        clear_mask();
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
            if (OC::ui.read_immediate(OC::CONTROL_BUTTON_L)) 
               mask &= ~(~(0xffff << (num_slots_ - cursor_pos_)) << cursor_pos_);
             //mask |= ~(0xffff << (num_slots_ - cursor_pos_)) << cursor_pos_; // alternative behaviour would be to fill them
          } 
          // empty patterns are ok -- 
          /*
          else {
            // pattern might be shortened to where no slots are active in mask
            if (0 == (mask & ~(0xffff < num_slots_)))
              mask |= 0x1;
          }
          */
          owner_->set_sequence_length(num_slots_, edit_this_sequence_);
          cursor_pos_ = num_slots_;
          handled = true;
        }
      }
    }

    if (!handled) {

      int32_t pitch = owner_->get_pitch_at_step(cursor_pos_);
      pitch += event.value;
      owner_->set_pitch_at_step(cursor_pos_, pitch);
      //mask = RotateMask(mask_, num_slots_, event.value);
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
  apply_mask(0x00);
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
