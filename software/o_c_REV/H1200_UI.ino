
void H1200_topButton() {
  if (h1200_settings.change_value(H1200_SETTING_INVERSION, 1)) {
    if (H1200_SETTING_INVERSION == h1200_state.cursor_pos) {
      int value = h1200_settings.get_value(H1200_SETTING_INVERSION);
      encoder[RIGHT].setPos(value);
    }
    h1200_state.value_changed = 1;
  }
}

void H1200_lowerButton() {
  if (h1200_settings.change_value(H1200_SETTING_INVERSION, -1)) {
    if (H1200_SETTING_INVERSION == h1200_state.cursor_pos) {
      int value = h1200_settings.get_value(H1200_SETTING_INVERSION);
      encoder[RIGHT].setPos(value);
    }
    h1200_state.value_changed = 1;
  }
}

void H1200_rightButton() {
  ++h1200_state.cursor_pos;
  if (h1200_state.cursor_pos >= H1200_SETTING_LAST)
    h1200_state.cursor_pos = 0;

  encoder[LEFT].setPos(h1200_state.cursor_pos);
  encoder[RIGHT].setPos(h1200_settings.get_value(h1200_state.cursor_pos));
}

void H1200_leftButton() {
  h1200_state.display_notes = !h1200_state.display_notes;
}

void H1200_leftButtonLong() {
  h1200_settings.InitDefaults();

  encoder[RIGHT].setPos(h1200_settings.get_value(h1200_state.cursor_pos));
  h1200_state.value_changed = true;
}

bool H1200_encoders() {
  bool changed = false;
  int value = encoder[LEFT].pos();
  if (value != h1200_state.cursor_pos) {
    if (value < 0) value = 0;
    else if (value >= H1200_SETTING_LAST) value = H1200_SETTING_LAST - 1;
    encoder[LEFT].setPos(value);
    h1200_state.cursor_pos = value;
    
    encoder[RIGHT].setPos(h1200_settings.get_value(h1200_state.cursor_pos));
    h1200_state.value_changed = changed = true;
  }

  value = encoder[RIGHT].pos();
  if (h1200_settings.apply_value(h1200_state.cursor_pos, value)) {
    encoder[RIGHT].setPos(h1200_settings.get_value(h1200_state.cursor_pos));
    h1200_state.value_changed = changed = true;
  }

  return changed;
}
