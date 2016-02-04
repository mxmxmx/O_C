#ifndef OC_SCALE_EDIT_H_
#define OC_SCALE_EDIT_H_

#include "OC_bitmaps.h"

extern TimerDebouncedButton<butL, 50, 2000> button_left;
extern TimerDebouncedButton<butR, 50, 2000> button_right;

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
//  void update_scale_mask(uint16_t mask);
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

  void Edit(Owner *owner, const Scale *scale, const char *scale_name) {
    Serial.print("Editing const scale "); Serial.println(scale_name);
    owner_ = owner;
    scale_ = scale;
    mutable_scale_ = nullptr;
    scale_name_ = scale_name;

    BeginEditing();
  }

  void Edit(Owner *owner, Scale *mutable_scale, const char *scale_name) {
    Serial.print("Editing mutable scale "); Serial.println(scale_name);
    owner_ = owner;
    scale_ = mutable_scale_ = mutable_scale;
    scale_name_ = scale_name;

    BeginEditing();
  }

  void Draw();
  bool handle_encoders();
  void handle_leftButton();
  void handle_leftButtonLong();
  void handle_rightButton();
  void handle_topButton();
  void handle_bottomButton();

private:

  Owner *owner_;
  const char * scale_name_;
  const braids::Scale *scale_;
  Scale *mutable_scale_;

  uint16_t mask_;
  size_t cursor_pos_;
  size_t num_notes_;

  long encoder_state_[2];

  void BeginEditing();
  void Close();

  void toggle_mask();
  void invert_mask();
  void rol_mask(long count);
  void ror_mask(long count);
  void apply_mask(uint16_t mask) {
    mask_ = mask;
    owner_->update_scale_mask(mask);
  }

  void reset_scale();

  void change_note(size_t pos, long delta);
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
/*
  if (mutable_scale_ && cursor_pos_ > 0) {
    graphics.print(scale_->notes[cursor_pos_ - 1], 4);
    graphics.print('<');
  } else {
    graphics.print("     ");
  }
*/
  if (cursor_pos_ != num_notes) {
    if (mutable_scale_)
      graphics.print('>');
    else
      graphics.print(' ');
    graphics.print(scale_->notes[cursor_pos_], 4);
  } else {
    graphics.print(num_notes, 2);
  }
/*
  if (mutable_scale_) {
    graphics.print('<');
    if (cursor_pos_ < num_notes - 1)
      graphics.print(scale_->notes[cursor_pos_ + 1], 4);
    else
      graphics.print(scale_->span, 4);
  }
*/
  x += 2; y += 10;
  uint16_t mask = mask_;
  for (size_t i = 0; i < num_notes; ++i, x += 7, mask >>= 1) {
    if (mask & 0x1)
      graphics.drawRect(x, y, 4, 8);
    else
      graphics.drawBitmap8(x, y, 4, bitmap_empty_frame4);

    if (i == cursor_pos_)
      graphics.drawFrame(x - 2, y - 2, 8, 12);
  }
  if (mutable_scale_) {
    graphics.drawBitmap8(x, y, 4, bitmap_end_marker4);
    if (cursor_pos_ == num_notes)
      graphics.drawFrame(x - 2, y - 2, 8, 12);
  }
}

template <typename Owner>
bool ScaleEditor<Owner>::handle_encoders() {
  bool changed = false;

  long left_value = encoder[LEFT].pos();
  if (left_value != (long)cursor_pos_) {
    const long max = mutable_scale_ ? num_notes_ : num_notes_ - 1;
    if (left_value < 0) left_value = 0;
    else if (left_value >= max) left_value = max;

    cursor_pos_ = left_value;
    encoder[RIGHT].setPos(0);
    changed = true;
  }

  long right_value = encoder[RIGHT].pos();
  if (mutable_scale_) {
    if (cursor_pos_ < num_notes_) {
      if (button_left.pressed()) {
        change_note(cursor_pos_, right_value);
        changed = true;
        right_value = 0;
      }
    } else {
      if (cursor_pos_ == num_notes_) {
        long num_notes = num_notes_;
        num_notes += right_value;
        if (num_notes < kMinScaleLength)
          num_notes = kMinScaleLength;
        else if (num_notes > kMaxScaleLength)
          num_notes = kMaxScaleLength;

        num_notes_ = num_notes;
        if (right_value > 0) {
          for (size_t pos = cursor_pos_; pos < num_notes_; ++pos)
            change_note(pos, 0);

          uint16_t mask = ~(0xffff << (num_notes_ - cursor_pos_)) << cursor_pos_;
          mask_ |= mask;
        }

        mutable_scale_->num_notes = num_notes_;
        left_value = cursor_pos_ = num_notes_;
        right_value = 0;
        changed = true;
      }
    }
  }

  if (right_value) {
    if (right_value < 0)
      ror_mask(-right_value);
    else
      rol_mask(right_value);
    changed = true;
  }
  right_value = 0;

  encoder[LEFT].setPos(left_value);
  encoder[RIGHT].setPos(right_value);
  return changed;
}

template <typename Owner>
void ScaleEditor<Owner>::handle_topButton() {
  if (button_left.pressed()) {
    if (cursor_pos_ == num_notes_)
      reset_scale();
    else
      change_note(cursor_pos_, 128);
  }
  else
    invert_mask();
}

template <typename Owner>
void ScaleEditor<Owner>::handle_bottomButton() {
  if (button_left.pressed())
    change_note(cursor_pos_, -128);
  else
    invert_mask();
}

template <typename Owner>
void ScaleEditor<Owner>::handle_leftButton() {
  uint16_t m = 0x1 << cursor_pos_;
  uint16_t mask = mask_;

  if (cursor_pos_ < num_notes_) {
    // toggle note active state; avoid 0 mask
    if (mask & m) {
      mask &= ~m;
      if (!mask) mask |= m;
    } else {
      mask |= m;
    }
    apply_mask(mask);
    return;
  }
}

template <typename Owner>
void ScaleEditor<Owner>::handle_leftButtonLong() {
}

template <typename Owner>
void ScaleEditor<Owner>::handle_rightButton() {
  Close();
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
void ScaleEditor<Owner>::rol_mask(long count) {
  uint16_t m = ~(0xffffU << num_notes_);
  uint16_t mask = mask_ & m;
  while (count--)
    mask = (mask << 1) | ((mask >> (num_notes_ - 1)) & 0x1);
  mask |= ~m;
  apply_mask(mask);
}

template <typename Owner>
void ScaleEditor<Owner>::ror_mask(long count) {
  uint16_t m = ~(0xffffU << num_notes_);
  uint16_t mask = mask_ & m;
  while (count--)
    mask = (mask >> 1) | ((mask & 0x1) << (num_notes_ - 1));
  mask |= ~m;
  apply_mask(mask);
}

template <typename Owner>
void ScaleEditor<Owner>::reset_scale() {
  Serial.println("Resetting scale to SEMI");

  *mutable_scale_ = braids::scales[1];
  num_notes_ = mutable_scale_->num_notes;
  cursor_pos_ = num_notes_;
  mask_ = ~(0xfff << num_notes_);
  apply_mask(mask_);
}

template <typename Owner>
void ScaleEditor<Owner>::change_note(size_t pos, long delta) {
  if (mutable_scale_ && pos < num_notes_) {
    int32_t note = mutable_scale_->notes[pos] + delta;

    const int32_t min = pos > 0 ? mutable_scale_->notes[pos - 1] : 0;
    const int32_t max = pos < num_notes_ - 1 ? mutable_scale_->notes[pos + 1] : mutable_scale_->span + 1;

    // TODO It's probably possible to construct a pothological scale,
    // maybe factor cursor_pos into it somehow?
    if (note < min) note = min + 1;
    if (note > max) note = max - 1;
    mutable_scale_->notes[pos] = note;
//    braids::SortScale(*mutable_scale_); // TODO side effects?

    owner_->scale_changed();
  }
}

template <typename Owner>
void ScaleEditor<Owner>::BeginEditing() {

  cursor_pos_ = 0;
  num_notes_ = scale_->num_notes;
  mask_ = owner_->get_scale_mask();

  encoder_state_[0] = encoder[LEFT].pos();
  encoder_state_[1] = encoder[RIGHT].pos();

  encoder[LEFT].setPos(cursor_pos_);
  encoder[RIGHT].setPos(scale_->notes[cursor_pos_]);
}

template <typename Owner>
void ScaleEditor<Owner>::Close() {
  encoder[LEFT].setPos(encoder_state_[LEFT]);
  encoder[RIGHT].setPos(encoder_state_[RIGHT]);

  owner_ = nullptr;
}

}; // namespace OC

#endif // OC_SCALE_EDIT_H_
