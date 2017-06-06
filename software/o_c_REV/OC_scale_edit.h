#ifndef OC_SCALE_EDIT_H_
#define OC_SCALE_EDIT_H_

#include "OC_bitmaps.h"
#include "OC_strings.h"
#include "OC_DAC.h"

namespace OC {

// Scale editor
// Edits both scale length and note note values, as well as a mask of "active"
// notes that the quantizer uses; some scales are read-only, in which case
// only the mask is editable
//
// The owner class needs to provide callbacks to get notification when values
// change: see ASR, A_SEQ, DQ, etc for details
//

enum SCALE_EDIT_PAGES {
  _SCALE,
  _ROOT,
  _TRANSPOSE,
  _SCALING
};

template <typename Owner>
class ScaleEditor {
public:

  void Init(bool mode) {
    owner_ = nullptr;
    scale_name_ = "?!";
    scale_ = mutable_scale_ = &dummy_scale;
    mask_ = 0;
    cursor_pos_ = 0;
    scaling_cursor_pos_ = 0;
    num_notes_ = 0;
    edit_this_scale_ = 0;
    /* mode: true = Meta-Q; false = scale editor */
    SEQ_MODE = mode;
    edit_page_ = _SCALE;
    ticks_ = 0;
  }

  bool active() const {
    return nullptr != owner_;
  }

  void Edit(Owner *owner, int scale) {
    if (OC::Scales::SCALE_NONE == scale)
      return;

    if (scale < OC::Scales::SCALE_USER_LAST) {
      scale_ = mutable_scale_ = &OC::user_scales[scale];
      scale_name_ = OC::scale_names_short[scale];
      // Serial.print("Editing mutable scale "); 
      // Serial.println(scale_name_);
    } else {
      scale_ = &OC::Scales::GetScale(scale);
      mutable_scale_ = nullptr;
      scale_name_ = OC::scale_names_short[scale];
      // Serial.print("Editing const scale "); 
      // Serial.println(scale_name_);
    }
    owner_ = owner;
    BeginEditing(SEQ_MODE);
  }

  void Close();
  void Draw();
  void HandleButtonEvent(const UI::Event &event);
  void HandleEncoderEvent(const UI::Event &event);
  static uint16_t RotateMask(uint16_t mask, int num_notes, int amount);

private:

  Owner *owner_;
  const char * scale_name_;
  const braids::Scale *scale_;
  Scale *mutable_scale_;

  uint16_t mask_;
  size_t cursor_pos_;
  size_t scaling_cursor_pos_;
  size_t num_notes_;
  int8_t edit_this_scale_;
  bool SEQ_MODE;
  uint8_t edit_page_; 
  uint32_t ticks_;

  void BeginEditing(bool mode);
  void move_cursor(int offset);

  void toggle_mask();
  void invert_mask(); 

  void apply_mask(uint16_t mask) {
    
    if (mask_ != mask) {
      mask_ = mask;
      owner_->update_scale_mask(mask_, edit_this_scale_);
    }
  }

  void reset_scale();
  void change_note(size_t pos, int delta, bool notify);
  void handleButtonLeft(const UI::Event &event);
  void handleButtonUp(const UI::Event &event);
  void handleButtonDown(const UI::Event &event);
};

template <typename Owner>
void ScaleEditor<Owner>::Draw() {

  if (edit_page_ < _SCALING) {
  
    size_t num_notes = num_notes_;
    static constexpr weegfx::coord_t kMinWidth = 64;
    weegfx::coord_t w =
      mutable_scale_ ? ((num_notes + 1) * 7) : (num_notes * 7);
  
    if (w < kMinWidth || (edit_page_ == _ROOT) || (edit_page_ == _TRANSPOSE)) w = kMinWidth;
  
    weegfx::coord_t x = 64 - (w + 1) / 2;
    weegfx::coord_t y = 16 - 3;
    weegfx::coord_t h = 36;
  
    graphics.clearRect(x, y, w + 4, h);
    graphics.drawFrame(x, y, w + 5, h);
  
    x += 2;
    y += 3;
  
    graphics.setPrintPos(x, y);
    graphics.print(scale_name_);
    
    if (SEQ_MODE) {
      ticks_++;
      uint8_t id = edit_this_scale_;
      if (edit_this_scale_ == owner_->get_scale_select())
        id += 4;
      graphics.print(OC::Strings::scale_id[id]);
    }
  
    if (SEQ_MODE && edit_page_) {
  
      switch (edit_page_) {
  
        case _ROOT:
        case _TRANSPOSE: {
          x += w / 2 - (25 + 3); y += 10;
          graphics.drawFrame(x, y, 25, 20);
          graphics.drawFrame(x + 32, y, 25, 20);
          // print root:
          int root = owner_->get_root(edit_this_scale_);
          if (root == 1 || root == 3 || root == 6 || root == 8 || root == 10)
            graphics.setPrintPos(x + 7, y + 7);
          else 
            graphics.setPrintPos(x + 9, y + 7);
          graphics.print(OC::Strings::note_names_unpadded[root]);
          // print transpose:
          int transpose = owner_->get_transpose(edit_this_scale_);
          graphics.setPrintPos(x + 32 + 5, y + 7);
          if (transpose >= 0)
            graphics.print("+");
          graphics.print(transpose);
          // draw cursor
          if (edit_page_ == _ROOT)
            graphics.invertRect(x - 1, y - 1, 27, 22); 
          else
            graphics.invertRect(x + 32 - 1, y - 1, 27, 22);
        }
        break;
        default:
        break;
      } 
    }
    else {
      // edit scale    
      graphics.setPrintPos(x, y + 24);
      if (cursor_pos_ != num_notes) {
        graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);
        if (mutable_scale_ && OC::ui.read_immediate(OC::CONTROL_BUTTON_L))
          graphics.drawBitmap8(x + 1, y + 23, kBitmapEditIndicatorW, bitmap_edit_indicators_8);
        else if (mutable_scale_)
          graphics.drawBitmap8(x + 1, y + 23, 4, bitmap_indicator_4x8);
    
        uint32_t note_value = scale_->notes[cursor_pos_];
        uint32_t cents = (note_value * 100) >> 7;
        uint32_t frac_cents = ((note_value * 100000) >> 7) - cents * 1000;
        // move print position, so that things look somewhat nicer
        if (cents < 10)
          graphics.movePrintPos(weegfx::Graphics::kFixedFontW * -3, 0);
        else if (cents < 100)
          graphics.movePrintPos(weegfx::Graphics::kFixedFontW * -2, 0);
        else if (cents < 1000)
          graphics.movePrintPos(weegfx::Graphics::kFixedFontW * -1, 0);
        // justify left ...  
        if (! mutable_scale_)
          graphics.movePrintPos(weegfx::Graphics::kFixedFontW * -1, 0);
        graphics.printf("%4u.%02uc", cents, (frac_cents + 5) / 10);
    
      } else {
        graphics.print((int)num_notes);
      }
  
      x += mutable_scale_ ? (w >> 0x1) - (((num_notes) * 7 + 4) >> 0x1) : (w >> 0x1) - (((num_notes - 1) * 7 + 4) >> 0x1); 
      y += 11;
      uint16_t mask = mask_;
      for (size_t i = 0; i < num_notes; ++i, x += 7, mask >>= 1) {
        if (mask & 0x1)
          graphics.drawRect(x, y, 4, 8);
        else
          graphics.drawBitmap8(x, y, 4, bitmap_empty_frame4x8);
    
        if (i == cursor_pos_)
          graphics.drawFrame(x - 2, y - 2, 8, 12);
      }
      if (mutable_scale_) {
        graphics.drawBitmap8(x, y, 4, bitmap_end_marker4x8);
        if (cursor_pos_ == num_notes)
          graphics.drawFrame(x - 2, y - 2, 8, 12);
      }
    }
  }
  else {
    // scaling ... 
    weegfx::coord_t x = 0;
    weegfx::coord_t y = 0;
    weegfx::coord_t h = 64;
    weegfx::coord_t w = 128;
  
    graphics.clearRect(x, y, w, h);
    graphics.drawFrame(x, y, w, h);

    x += 5;
    y += 5;
    graphics.setPrintPos(x, y);
    graphics.print(OC::Strings::scaling_string[0]);
    graphics.print(OC::Strings::channel_id[DAC_CHANNEL_A]);
    graphics.setPrintPos(x + 87, y);
    graphics.print(OC::voltage_scalings[OC::DAC::get_voltage_scaling(DAC_CHANNEL_A)]);
    y += 16;
    graphics.setPrintPos(x, y);
    graphics.print(OC::Strings::scaling_string[0]);
    graphics.print(OC::Strings::channel_id[DAC_CHANNEL_B]);
    graphics.setPrintPos(x + 87, y);
    graphics.print(OC::voltage_scalings[OC::DAC::get_voltage_scaling(DAC_CHANNEL_B)]);
    y += 16;
    graphics.setPrintPos(x, y);
    graphics.print(OC::Strings::scaling_string[0]);
    graphics.print(OC::Strings::channel_id[DAC_CHANNEL_C]);
    graphics.setPrintPos(x + 87, y);
    graphics.print(OC::voltage_scalings[OC::DAC::get_voltage_scaling(DAC_CHANNEL_C)]);
    y += 16;
    graphics.setPrintPos(x, y);
    graphics.print(OC::Strings::scaling_string[0]);
    graphics.print(OC::Strings::channel_id[DAC_CHANNEL_D]);
    graphics.setPrintPos(x + 87, y);
    graphics.print(OC::voltage_scalings[OC::DAC::get_voltage_scaling(DAC_CHANNEL_D)]);
    // draw cursor:
    graphics.invertRect(x - 2, (scaling_cursor_pos_ << 4) + 3, w - 6, 11);
  }
}

template <typename Owner>
void ScaleEditor<Owner>::HandleButtonEvent(const UI::Event &event) {
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
      {
        if (edit_page_ < _SCALING)
          Close();
        else
          edit_page_ = _SCALE;
      }
      break;
      default:
      break;
    }
  }
  else if (UI::EVENT_BUTTON_LONG_PRESS == event.type) {
    
     switch (event.control) {
      case OC::CONTROL_BUTTON_L:
      {
        if (edit_page_ < _SCALING)
          edit_page_ = _SCALING;
      }
      break;
      default:
      {
        if (edit_page_ == _SCALING)
          edit_page_ = _SCALE;
      }
      break;
     }
  }
}

template <typename Owner>
void ScaleEditor<Owner>::HandleEncoderEvent(const UI::Event &event) {
  bool scale_changed = false;
  uint16_t mask = mask_;

  if (OC::CONTROL_ENCODER_L == event.control) {
    
    if (SEQ_MODE && OC::ui.read_immediate(OC::CONTROL_BUTTON_UP)) {
      
          // we're in Meta-Q, so we can change the scale:
         
          int _scale = owner_->get_scale(edit_this_scale_) + event.value;
          CONSTRAIN(_scale, OC::Scales::SCALE_USER_0, OC::Scales::NUM_SCALES-1);
          
          if (_scale == OC::Scales::SCALE_NONE) {
             // just skip this here ...
             if (event.value > 0) 
                _scale++;
             else 
                _scale--;
          }
          // update active scale with mask/root/transpose settings, and set flag:
          owner_->set_scale_at_slot(_scale, mask_, owner_->get_root(edit_this_scale_), owner_->get_transpose(edit_this_scale_), edit_this_scale_); 
          scale_changed = true; 
          
          if (_scale < OC::Scales::SCALE_USER_LAST) {
            scale_ = mutable_scale_ = &OC::user_scales[_scale];
            scale_name_ = OC::scale_names_short[_scale];
          } 
          else {
            scale_ = &OC::Scales::GetScale(_scale);
            mutable_scale_ = nullptr;
            scale_name_ = OC::scale_names_short[_scale];
          }
          cursor_pos_ = 0;
          num_notes_ = scale_->num_notes;
          mask_ = owner_->get_scale_mask(edit_this_scale_); // ? can go, because the mask didn't change
          ticks_ = 0;
    }
    else {
      move_cursor(event.value);
    }
  } else if (OC::CONTROL_ENCODER_R == event.control) {

      switch (edit_page_) {

        case _SCALE: 
        {
          bool handled = false;
          if (mutable_scale_) {
            if (cursor_pos_ < num_notes_) {
              if (event.mask & OC::CONTROL_BUTTON_L) {
                OC::ui.IgnoreButton(OC::CONTROL_BUTTON_L);
                change_note(cursor_pos_, event.value, false);
                scale_changed = true;
                handled = true;
              }
            } else {
              if (cursor_pos_ == num_notes_) {
                int num_notes = num_notes_;
                num_notes += event.value;
                CONSTRAIN(num_notes, kMinScaleLength, kMaxScaleLength);
      
                num_notes_ = num_notes;
                if (event.value > 0) {
                  for (size_t pos = cursor_pos_; pos < num_notes_; ++pos)
                    change_note(pos, 0, false);
      
                  // Enable new notes by default
                  mask |= ~(0xffff << (num_notes_ - cursor_pos_)) << cursor_pos_;
                } else {
                  // scale might be shortened to where no notes are active in mask
                  if (0 == (mask & ~(0xffff < num_notes_)))
                    mask |= 0x1;
                }
      
                mutable_scale_->num_notes = num_notes_;
                cursor_pos_ = num_notes_;
                handled = scale_changed = true;
              }
            }
          }
          if (!handled) {
              mask = RotateMask(mask_, num_notes_, event.value);
          }
        }
        break;
        case _ROOT:
        {
         int _root = owner_->get_root(edit_this_scale_) + event.value;
         CONSTRAIN(_root, 0, 11);
         owner_->set_scale_at_slot(owner_->get_scale(edit_this_scale_), mask_, _root, owner_->get_transpose(edit_this_scale_), edit_this_scale_); 
        }
        break;
        case _TRANSPOSE: 
        {
         int _transpose = owner_->get_transpose(edit_this_scale_) + event.value;
         CONSTRAIN(_transpose, -12, 12);
         owner_->set_scale_at_slot(owner_->get_scale(edit_this_scale_), mask_, owner_->get_root(edit_this_scale_), _transpose, edit_this_scale_); 
        }
        break;
        case _SCALING:
        {
         int item = scaling_cursor_pos_ + event.value;
         CONSTRAIN(item, DAC_CHANNEL_A, DAC_CHANNEL_LAST - 0x1);
         scaling_cursor_pos_ = item;
        }
        break;
        default:
        break;
      }
  }
  // This isn't entirely atomic
  apply_mask(mask);
  if (scale_changed)
    owner_->scale_changed();
}

template <typename Owner>
void ScaleEditor<Owner>::move_cursor(int offset) {

  switch (edit_page_) {

    case _ROOT:
    case _TRANSPOSE:
    {
      int8_t item = edit_page_ + offset;
      CONSTRAIN(item, _ROOT, _TRANSPOSE);
      edit_page_ = item;  
    }  
    break;
    case _SCALING:
    {
      int8_t scaling = OC::DAC::get_voltage_scaling(scaling_cursor_pos_) + offset;
      CONSTRAIN(scaling, VOLTAGE_SCALING_1V_PER_OCT, VOLTAGE_SCALING_LAST - 0x1);
      OC::DAC::set_scaling(scaling, scaling_cursor_pos_);
    }
    break;
    default:
    {
      int cursor_pos = cursor_pos_ + offset;
      const int max = mutable_scale_ ? num_notes_ : num_notes_ - 1;
      CONSTRAIN(cursor_pos, 0, max);
      cursor_pos_ = cursor_pos;
    }
    break;
  }
}

template <typename Owner>
void ScaleEditor<Owner>::handleButtonUp(const UI::Event &event) {

  if (event.mask & OC::CONTROL_BUTTON_L) {
    OC::ui.IgnoreButton(OC::CONTROL_BUTTON_L);
    if (cursor_pos_ == num_notes_)
      reset_scale();
    else
      change_note(cursor_pos_, 128, true);
  } else {
    if (!SEQ_MODE)
      invert_mask();
    else {
      
      if (ticks_ > 250) {
        edit_this_scale_++;  
        if (edit_this_scale_ > 0x3) 
          edit_this_scale_ = 0;
          
        uint8_t scale = owner_->get_scale(edit_this_scale_);  
        // Serial.println(scale);
        if (scale < OC::Scales::SCALE_USER_LAST) {
          scale_ = mutable_scale_ = &OC::user_scales[scale];
          scale_name_ = OC::scale_names_short[scale];
          // Serial.print("Editing mutable scale "); 
          // Serial.println(scale_name_);
         } 
         else {
          scale_ = &OC::Scales::GetScale(scale);
          mutable_scale_ = nullptr;
          scale_name_ = OC::scale_names_short[scale];
          // Serial.print("Editing const scale "); 
          // Serial.println(scale_name_);
        }
        cursor_pos_ = 0;
        num_notes_ = scale_->num_notes;
        mask_ = owner_->get_scale_mask(edit_this_scale_);  
      }
    }
  }
}

template <typename Owner>
void ScaleEditor<Owner>::handleButtonDown(const UI::Event &event) {
  if (event.mask & OC::CONTROL_BUTTON_L) {
    OC::ui.IgnoreButton(OC::CONTROL_BUTTON_L);
    change_note(cursor_pos_, -128, true);
  } else {
    if (!SEQ_MODE)
      invert_mask();
    else {
      
      if (ticks_ > 250) {
        edit_this_scale_--;  
        if (edit_this_scale_ < 0) 
          edit_this_scale_ = 3; 
          
        uint8_t scale = owner_->get_scale(edit_this_scale_);  
        // Serial.println(scale);
        if (scale < OC::Scales::SCALE_USER_LAST) {
          scale_ = mutable_scale_ = &OC::user_scales[scale];
          scale_name_ = OC::scale_names_short[scale];
          // Serial.print("Editing mutable scale "); 
          // Serial.println(scale_name_);
         } 
         else {
          scale_ = &OC::Scales::GetScale(scale);
          mutable_scale_ = nullptr;
          scale_name_ = OC::scale_names_short[scale];
          // Serial.print("Editing const scale "); 
          // Serial.println(scale_name_);
        }
        cursor_pos_ = 0;
        num_notes_ = scale_->num_notes;
        mask_ = owner_->get_scale_mask(edit_this_scale_);
      }
    }
  }
}

template <typename Owner>
void ScaleEditor<Owner>::handleButtonLeft(const UI::Event &) {

  if (SEQ_MODE && OC::ui.read_immediate(OC::CONTROL_BUTTON_UP)) {

    if (edit_page_ == _SCALE) 
      edit_page_ = _ROOT; // switch to root
    // and don't accidentally change scale slot:  
    ticks_ = 0x0;
  }
  else { 
    // edit scale mask
    if (edit_page_ > _SCALE)
      edit_page_ = _SCALE;
    else {
      uint16_t m = 0x1 << cursor_pos_;
      uint16_t mask = mask_;
    
      if (cursor_pos_ < num_notes_) {
        // toggle note active state; avoid 0 mask
        if (mask & m) {
          if ((mask & ~(0xffff << num_notes_)) != m)
            mask &= ~m;
        } else {
          mask |= m;
        }
        apply_mask(mask);
      } 
    }
  }
}

template <typename Owner>
void ScaleEditor<Owner>::invert_mask() {
  uint16_t m = ~(0xffffU << num_notes_);
  uint16_t mask = mask_;
  // Don't invert to zero
  if ((mask & m) != m)
    mask ^= m;
  apply_mask(mask);
}

template <typename Owner>
/*static*/ uint16_t ScaleEditor<Owner>::RotateMask(uint16_t mask, int num_notes, int amount) {
  uint16_t used_bits = ~(0xffffU << num_notes);
  mask &= used_bits;

  if (amount < 0) {
    amount = -amount % num_notes;
    mask = (mask >> amount) | (mask << (num_notes - amount));
  } else {
    amount = amount % num_notes;
    mask = (mask << amount) | (mask >> (num_notes - amount));
  }
  return mask | ~used_bits; // fill upper bits
}

template <typename Owner>
void ScaleEditor<Owner>::reset_scale() {
  // Serial.println("Resetting scale to SEMI");

  *mutable_scale_ = OC::Scales::GetScale(OC::Scales::SCALE_SEMI);
  num_notes_ = mutable_scale_->num_notes;
  cursor_pos_ = num_notes_;
  mask_ = ~(0xfff << num_notes_);
  apply_mask(mask_);
}

template <typename Owner>
void ScaleEditor<Owner>::change_note(size_t pos, int delta, bool notify) {
  if (mutable_scale_ && pos < num_notes_) {
    int32_t note = mutable_scale_->notes[pos] + delta;

    const int32_t min = pos > 0 ? mutable_scale_->notes[pos - 1] : 0;
    const int32_t max = pos < num_notes_ - 1 ? mutable_scale_->notes[pos + 1] : mutable_scale_->span;

    // TODO It's probably possible to construct a pothological scale,
    // maybe factor cursor_pos into it somehow?
    if (note <= min) note = pos > 0 ? min + 1 : 0;
    if (note >= max) note = max - 1;
    mutable_scale_->notes[pos] = note;
//    braids::SortScale(*mutable_scale_); // TODO side effects?

    if (notify)
      owner_->scale_changed();
  }
}

template <typename Owner>
void ScaleEditor<Owner>::BeginEditing(bool mode) {

  cursor_pos_ = 0;
  num_notes_ = scale_->num_notes;
  mask_ = owner_->get_scale_mask(DUMMY);
  if (mode) { // == meta-Q
    edit_this_scale_ = owner_->get_scale_select();
   OC::ui._preemptScreensaver(true);
  }
}

template <typename Owner>
void ScaleEditor<Owner>::Close() {
  ui.SetButtonIgnoreMask();
  OC::ui._preemptScreensaver(false);
  owner_ = nullptr;
  edit_this_scale_ = 0;
  edit_page_ = _SCALE;
}

}; // namespace OC

#endif // OC_SCALE_EDIT_H_
