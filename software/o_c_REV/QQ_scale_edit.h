#ifndef QQ_SCALE_EDIT_H_
#define QQ_SCALE_EDIT_H_

braids::Scale dummy_scale;

class ScaleEditor {
public:
  enum EDIT_MODE {
    EDIT_MODE_NONE,
    EDIT_MODE_MASK,
    EDIT_MODE_SCALE
  };

  void Init() {
    mode_ = EDIT_MODE_NONE;
  }

  bool active() const {
    return EDIT_MODE_NONE != mode_;
  }

  void EditMask(QuantizerChannel *channel) {
    mode_ = EDIT_MODE_MASK;
    channel_ = channel;
    scale_name_ = quantization_values[channel_->get_scale()];

    int scale = channel_->get_scale();
    if (scale < CUSTOM_SCALE_LAST) {
      scale_ = mutable_scale_ = &QuantizerChannel::custom_scales[scale];
    } else {
      scale_ = &braids::scales[scale - CUSTOM_SCALE_LAST];
      mutable_scale_ = &dummy_scale;
    }

    cursor_pos_ = 0;
    num_notes_ = scale_->num_notes;
    mask_ = channel_->get_mask();

    Begin();
    encoder[LEFT].setPos(cursor_pos_);
    encoder[RIGHT].setPos(0);
  }

  void EditScale(braids::Scale *scale, const char *scale_name) {

    mode_ = EDIT_MODE_SCALE;
    channel_ = nullptr;
    scale_name_ = scale_name;

    scale_ = mutable_scale_ = scale;

    num_notes_ = scale_->num_notes;
    mask_ = ~(0xffff << num_notes_);
    if (num_notes_ < 16)
      ++num_notes_;

    Begin();
    encoder[LEFT].setPos(cursor_pos_);
    encoder[RIGHT].setPos(mutable_scale_->notes[cursor_pos_]);
  }

  void Draw();
  bool handle_encoders();
  void handle_leftButton();
  void handle_rightButton();
  void handle_topButton();
  void handle_bottomButton();

private:
  EDIT_MODE mode_;
  QuantizerChannel *channel_;
  const char *scale_name_;
  const braids::Scale *scale_;
  braids::Scale *mutable_scale_;

  uint16_t mask_;
  size_t cursor_pos_;
  size_t num_notes_;

  long encoder_state_[2];

  void Begin();
  void Close();

  // Only valid for EDIT_MODE_MASK
  void invert_mask();
  void rol_mask(long count);
  void ror_mask(long count);
  void apply_mask(uint16_t mask) {
    mask_ = mask;
    channel_->apply_value(CHANNEL_SETTING_MASK, mask); // Should automatically be updated
  }
};

void ScaleEditor::Draw() {
  size_t num_notes = num_notes_;

  weegfx::coord_t w = (num_notes < 5 ? 5 : num_notes) * 7;
  weegfx::coord_t x = 64 - w / 2;
  weegfx::coord_t y = 12;
  weegfx::coord_t h = 36;

  graphics.clearRect(x, y, w + 4, h);
  graphics.drawFrame(x, y, w + 4, h);

  x += 2;
  y += 3;

  graphics.setPrintPos(x, y);
  graphics.print(scale_name_);

  graphics.setPrintPos(x, y + 24);
  if (EDIT_MODE_SCALE == mode_) graphics.print(">");
  graphics.print(scale_->notes[cursor_pos_]);

  if (EDIT_MODE_SCALE == mode_) {
    graphics.setPrintPos(x + w - 6, y + 24);
    if (cursor_pos_ == mutable_scale_->num_notes - 1) {
      graphics.print('-');
    } else if (cursor_pos_ == num_notes_ - 1) {
      if (mask_ & (0x1 << cursor_pos_))
        graphics.print('-');
      else
        graphics.print('+');
    }
  }

  x += 2; y += 10;
  uint16_t mask = mask_;
  for (size_t i = 0; i < num_notes; ++i, x += 7, mask >>= 1) {
    if (mask & 0x1)
      graphics.drawRect(x, y, 4, 8);
    else
      graphics.drawFrame(x, y, 3, 7);
    if (i == cursor_pos_)
      graphics.drawFrame(x - 2, y - 2, 7, 11);
  }

}

bool ScaleEditor::handle_encoders() {
  long left_value = encoder[LEFT].pos();
  bool changed = false;

  if (left_value != (long)cursor_pos_) {
    if (left_value < 0) left_value = 0;
    else if (left_value >= (long)num_notes_) left_value = num_notes_ - 1;

    cursor_pos_ = left_value;
    if (EDIT_MODE_SCALE == mode_)
      encoder[RIGHT].setPos(mutable_scale_->notes[cursor_pos_]);
    changed = true;
  }

  long right_value = encoder[RIGHT].pos();
  if (EDIT_MODE_MASK == mode_) {
    if (right_value) {
      if (right_value < 0)
        ror_mask(-right_value);
      else
        rol_mask(right_value);
      changed = true;
      right_value = 0;
    }
  } else if (EDIT_MODE_SCALE == mode_) {

    long min = cursor_pos_ > 1 ? mutable_scale_->notes[cursor_pos_ - 1] : 0;
    long max = cursor_pos_ < num_notes_ - 1 ? mutable_scale_->notes[cursor_pos_ + 1] : mutable_scale_->span;

    if (right_value < min) right_value = min;
    else if (right_value > max) right_value = max;

    if (static_cast<uint16_t>(right_value) != mutable_scale_->notes[cursor_pos_]) {
      mutable_scale_->notes[cursor_pos_] = right_value;
      changed = true;
    }
  }

  encoder[LEFT].setPos(left_value);
  encoder[RIGHT].setPos(right_value);

  return changed;
}

void ScaleEditor::handle_topButton() {
  if (EDIT_MODE_MASK == mode_)
    invert_mask();
}

void ScaleEditor::handle_bottomButton() {
  if (EDIT_MODE_MASK == mode_)
    invert_mask();
}

void ScaleEditor::handle_leftButton() {
  uint16_t m = 0x1 << cursor_pos_;
  uint16_t mask = mask_;
  if (EDIT_MODE_MASK == mode_) {
    if (mask & m) {
      // We have to avoid having a zero-mask
      if (0 != ((mask & ~m) & channel_->max_mask()))
        mask &= ~m;
    } else {
      mask |= m;
    }
    apply_mask(mask);
  } else if (EDIT_MODE_SCALE == mode_) {
    if (mask & m) {
      if (cursor_pos_ > 0 && cursor_pos_ == mutable_scale_->num_notes - 1) {
          --mutable_scale_->num_notes;
          --cursor_pos_;
          num_notes_ = mutable_scale_->num_notes + 1;
          mask >>= 1;
      }
    } else if (cursor_pos_ == num_notes_ - 1) {
      if (num_notes_ < 16) {
        if (mutable_scale_->notes[num_notes_ + 1] < mutable_scale_->notes[cursor_pos_])
          mutable_scale_->notes[num_notes_ + 1] = mutable_scale_->notes[cursor_pos_];
        ++mutable_scale_->num_notes;
        ++num_notes_;
        cursor_pos_ = num_notes_ - 1;
        mask |= m;
      } else {
        if (mutable_scale_->notes[15] < mutable_scale_->notes[14])
          mutable_scale_->notes[15] = mutable_scale_->notes[14];
        ++mutable_scale_->num_notes;
        mask = 0xffff;
      }
    }
    mask_ = mask;
    encoder[LEFT].setPos(cursor_pos_);
    encoder[RIGHT].setPos(mutable_scale_->notes[cursor_pos_]);
  }
}

void ScaleEditor::handle_rightButton() {
  Close();
}

void ScaleEditor::invert_mask() {
  uint16_t m = ~(0xffffU << scale_->num_notes);
  uint16_t mask = mask_;
  // Don't invert to zero
  if ((mask & m) != m)
    mask ^= m;
  apply_mask(mask);
}

void ScaleEditor::rol_mask(long count) {
  uint16_t m = ~(0xffffU << num_notes_);
  uint16_t mask = mask_ & m;
  while (count--)
    mask = (mask << 1) | ((mask >> (num_notes_ - 1)) & 0x1);
  mask |= ~m;
  apply_mask(mask);
}

void ScaleEditor::ror_mask(long count) {
  uint16_t m = ~(0xffffU << num_notes_);
  uint16_t mask = mask_ & m;
  while (count--)
    mask = (mask >> 1) | ((mask & 0x1) << (num_notes_ - 1));
  mask |= ~m;
  apply_mask(mask);
}

void ScaleEditor::Begin() {
  encoder_state_[0] = encoder[LEFT].pos();
  encoder_state_[1] = encoder[RIGHT].pos();
}

void ScaleEditor::Close() {
  encoder[LEFT].setPos(encoder_state_[0]);
  encoder[RIGHT].setPos(0);
  mode_ = EDIT_MODE_NONE;
}

#endif // QQ_SCALE_EDIT_H_
