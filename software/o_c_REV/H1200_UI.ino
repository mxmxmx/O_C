
void H1200_topButton() {
  if (h1200_settings.change_value(H1200_SETTING_INVERSION, 1)) {
    h1200_state.force_update = true;
  }
}

void H1200_lowerButton() {
  if (h1200_settings.change_value(H1200_SETTING_INVERSION, -1)) {
    h1200_state.force_update = 1;
  }
}

void H1200_rightButton() {
  ++h1200_state.cursor_pos;
  if (h1200_state.cursor_pos >= H1200_SETTING_LAST)
    h1200_state.cursor_pos = 0;
}

void H1200_leftButton() {
  h1200_state.display_notes = !h1200_state.display_notes;
}

void H1200_leftButtonLong() {
  h1200_settings.InitDefaults();
  h1200_state.force_update = true;
}

bool H1200_encoders() {
  bool changed = false;
  int value = encoder[LEFT].pos();
  encoder[LEFT].setPos(0);

  if (value) {
    value += h1200_state.cursor_pos;
    CONSTRAIN(value, 0, H1200_SETTING_LAST - 1);
    h1200_state.cursor_pos = value;
    h1200_state.force_update = changed = true;
  }

  value = encoder[RIGHT].pos();
  encoder[RIGHT].setPos(0);
  if (value && h1200_settings.change_value(h1200_state.cursor_pos, value)) {
    h1200_state.force_update = changed = true;
  }

  return changed;
}
