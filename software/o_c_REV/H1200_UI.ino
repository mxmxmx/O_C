
void H1200_topButton() {
  if (h1200_settings.change_value(H1200_SETTING_INVERSION, 1)) {
    if (H1200_SETTING_INVERSION == h1200_state.cursor_pos) {
      h1200_state.cursor_value = h1200_settings.get_value(H1200_SETTING_INVERSION);
      encoder[RIGHT].setPos(h1200_state.cursor_value);
    }
    h1200_state.value_changed = 1;
  }
}

void H1200_lowerButton() {
  if (h1200_settings.change_value(H1200_SETTING_INVERSION, -1)) {
    if (H1200_SETTING_INVERSION == h1200_state.cursor_pos) {
      h1200_state.cursor_value = h1200_settings.get_value(H1200_SETTING_INVERSION);
      encoder[RIGHT].setPos(h1200_state.cursor_value);
    }
    h1200_state.value_changed = 1;
  }
}

void H1200_rightButton() {
  if (UI_MODE) {
    h1200_state.value_changed =
        h1200_settings.apply_value(h1200_state.cursor_pos,
                                   h1200_state.cursor_value);
  }
}

void H1200_leftButton() {
  h1200_state.display_notes = !h1200_state.display_notes;
}

bool H1200_encoders() {
  bool changed = false;
  int value = encoder[LEFT].pos();
  if (value != h1200_state.cursor_pos) {
    if (value < 0) value = 0;
    else if (value >= H1200_SETTING_LAST) value = H1200_SETTING_LAST - 1;
    encoder[LEFT].setPos(value);
    h1200_state.cursor_pos = value;
    h1200_state.cursor_value = h1200_settings.get_value(h1200_state.cursor_pos);
    encoder[RIGHT].setPos(h1200_state.cursor_value);
    changed = true;
  }

  value = encoder[RIGHT].pos();
  if (value != h1200_state.cursor_value) {
    h1200_state.cursor_value = H1200Settings::clamp_value(h1200_state.cursor_pos, value);
    encoder[RIGHT].setPos(h1200_state.cursor_value);
    changed = true;
  }

  return changed;
}
