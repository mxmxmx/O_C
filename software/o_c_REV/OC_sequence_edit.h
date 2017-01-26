#ifndef OC_SEQUENCE_EDIT_H_
#define OC_SEQUENCE_EDIT_H_

#include "OC_bitmaps.h"
#include "OC_patterns.h"
#include "OC_patterns_presets.h"

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
    id += OC::Patterns::PATTERN_USER_LAST;
  graphics.print(OC::Strings::seq_id[id]);
  graphics.print("/");

  // print length
  if (num_slots > 9)
      graphics.print((int)num_slots, 2);
    else 
      graphics.print((int)num_slots, 1); 

  if (cursor_pos_ != num_slots) {

    graphics.setPrintPos(x, y + 24);
    graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);
    //if (OC::ui.read_immediate(OC::CONTROL_BUTTON_L))
    //  graphics.drawBitmap8(x + 1, y + 23, kBitmapEditIndicatorW, bitmap_edit_indicators_8);

    // this should really be displayed  *below* the slot...
    int pitch = (int)owner_->get_pitch_at_step(edit_this_sequence_, cursor_pos_);
    graphics.print(pitch, 0);  
  }

  x += 2; y += 10;
  uint16_t mask = mask_;
  uint8_t clock_pos= owner_->get_clock_cnt();
  
  for (size_t i = 0; i < num_slots; ++i, x += 7, mask >>= 1) {
    if (mask & 0x1)
      graphics.drawRect(x, y, 4, 8);
    else
      graphics.drawBitmap8(x, y, 4, bitmap_empty_frame4x8);

    if (i == cursor_pos_)
      graphics.drawFrame(x - 2, y - 2, 8, 12);
    // draw clock   
    if (i == clock_pos && (owner_->get_current_sequence() == edit_this_sequence_))
      graphics.drawRect(x, y + 10, 2, 2);
       
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
      
      int32_t pitch = owner_->get_pitch_at_step(edit_this_sequence_, cursor_pos_);
      // Q? might be better to actually add whatever is in the scale
      // or semitone/finetune?
      int16_t delta = event.value;
      if (OC::ui.read_immediate(OC::CONTROL_BUTTON_L)) 
       pitch += delta; // fine
      else
        pitch += (delta << 7); // semitone 
      // TODO .. proper limits  
      CONSTRAIN(pitch, -6272, 6272);  
      owner_->set_pitch_at_step(edit_this_sequence_, cursor_pos_, pitch);
      // TODO
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
