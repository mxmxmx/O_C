#ifndef OC_SCALE_EDIT_H_
#define OC_SCALE_EDIT_H_

#include "OC_bitmaps.h"

namespace OC {

// Scale editor
// Edits both scale length and note note values, as well as a mask of "active"
// notes that the quantizer uses; some scales are read-only, in which case
// only the mask is editable
//
// The owner class needs to provide callbacks to get notification when values
// change:
// void scale_changed();
// uint16_t get_scale_mask();
// void update_scale_mask(uint16_t mask);
//

template <typename Owner>
class ScaleEditor {
public:

  void Init() {
    owner_ = nullptr;
    scale_name_ = "?!";
    scale_ = mutable_scale_ = &dummy_scale;
    mask_ = 0;
    cursor_pos_ = 0;
    num_notes_ = 0;
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
      Serial.print("Editing mutable scale "); Serial.println(scale_name_);
    } else {
      scale_ = &OC::Scales::GetScale(scale);
      mutable_scale_ = nullptr;
      scale_name_ = OC::scale_names_short[scale];
      Serial.print("Editing const scale "); Serial.println(scale_name_);
    }
    owner_ = owner;

    BeginEditing();
  }

  void Close();

  void Draw();
  void HandleButtonEvent(const UI::Event &event);
  void HandleEncoderEvent(const UI::Event &event);

private:

  Owner *owner_;
  const char * scale_name_;
  const braids::Scale *scale_;
  Scale *mutable_scale_;

  uint16_t mask_;
  size_t cursor_pos_;
  size_t num_notes_;

  void BeginEditing();

  void move_cursor(int offset);

  void toggle_mask();
  void invert_mask(); 

  uint16_t rol_mask(int count);
  uint16_t ror_mask(int count);

  void apply_mask(uint16_t mask) {
    if (mask_ != mask) {
      mask_ = mask;
      owner_->update_scale_mask(mask_);
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
  size_t num_notes = num_notes_;

  static constexpr weegfx::coord_t kMinWidth = 8 * 7;

  weegfx::coord_t w =
    mutable_scale_ ? (num_notes + 1) * 7 : num_notes * 7;

  if (w < kMinWidth) w = kMinWidth;

  weegfx::coord_t x = 64 - (w + 1)/ 2;
  weegfx::coord_t y = 16 - 3;
  weegfx::coord_t h = 36;

  graphics.clearRect(x, y, w + 4, h);
  graphics.drawFrame(x, y, w + 5, h);

  x += 2;
  y += 3;

  graphics.setPrintPos(x, y);
  graphics.print(scale_name_);

  graphics.setPrintPos(x, y + 24);
  if (cursor_pos_ != num_notes) {
    graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);
    if (mutable_scale_ && OC::ui.read_immediate(OC::CONTROL_BUTTON_L))
      graphics.drawBitmap8(x + 1, y + 23, kBitmapEditIndicatorW, bitmap_edit_indicators_8);
    graphics.print(scale_->notes[cursor_pos_], 4);
  } else {
    graphics.print((int)num_notes, 2);
  }

  x += 2; y += 10;
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
        Close();
        break;
    }
  }
}

template <typename Owner>
void ScaleEditor<Owner>::HandleEncoderEvent(const UI::Event &event) {
  bool scale_changed = false;
  uint16_t mask = mask_;

  if (OC::CONTROL_ENCODER_L == event.control) {
    move_cursor(event.value);
  } else if (OC::CONTROL_ENCODER_R == event.control) {
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
      if (event.value < 0)
        mask = ror_mask(-event.value);
      else
        mask = rol_mask(event.value);
    }
  }

  // This isn't entirely atomic
  apply_mask(mask);
  if (scale_changed)
    owner_->scale_changed();
}

template <typename Owner>
void ScaleEditor<Owner>::move_cursor(int offset) {

  int cursor_pos = cursor_pos_ + offset;
  const int max = mutable_scale_ ? num_notes_ : num_notes_ - 1;
  CONSTRAIN(cursor_pos, 0, max);
  cursor_pos_ = cursor_pos;
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
    invert_mask();
  }
}

template <typename Owner>
void ScaleEditor<Owner>::handleButtonDown(const UI::Event &event) {
  if (event.mask & OC::CONTROL_BUTTON_L) {
    OC::ui.IgnoreButton(OC::CONTROL_BUTTON_L);
    change_note(cursor_pos_, -128, true);
  } else {
    invert_mask();
  }
}

template <typename Owner>
void ScaleEditor<Owner>::handleButtonLeft(const UI::Event &) {
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
uint16_t ScaleEditor<Owner>::rol_mask(int count) {
  uint16_t m = ~(0xffffU << num_notes_);
  uint16_t mask = mask_ & m;
  while (count--)
    mask = (mask << 1) | ((mask >> (num_notes_ - 1)) & 0x1);
  mask |= ~m;
  return mask;
}

template <typename Owner>
uint16_t ScaleEditor<Owner>::ror_mask(int count) {
  uint16_t m = ~(0xffffU << num_notes_);
  uint16_t mask = mask_ & m;
  while (count--)
    mask = (mask >> 1) | ((mask & 0x1) << (num_notes_ - 1));
  mask |= ~m;
  return mask;
}

template <typename Owner>
void ScaleEditor<Owner>::reset_scale() {
  Serial.println("Resetting scale to SEMI");

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
    const int32_t max = pos < num_notes_ - 1 ? mutable_scale_->notes[pos + 1] : mutable_scale_->span + 1;

    // TODO It's probably possible to construct a pothological scale,
    // maybe factor cursor_pos into it somehow?
    if (note < min) note = pos > 0 ? min + 1 : 0;
    if (note > max) note = max - 1;
    mutable_scale_->notes[pos] = note;
//    braids::SortScale(*mutable_scale_); // TODO side effects?

    if (notify)
      owner_->scale_changed();
  }
}

template <typename Owner>
void ScaleEditor<Owner>::BeginEditing() {

  cursor_pos_ = 0;
  num_notes_ = scale_->num_notes;
  mask_ = owner_->get_scale_mask();
}

template <typename Owner>
void ScaleEditor<Owner>::Close() {
  ui.SetButtonIgnoreMask();
  owner_ = nullptr;
}

}; // namespace OC

#endif // OC_SCALE_EDIT_H_
