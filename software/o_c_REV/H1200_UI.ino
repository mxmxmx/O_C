
void H1200_topButton() {
  if (h1200_settings.apply_value(
          SETTING_INVERSION, h1200_settings.get_value(SETTING_INVERSION) + 1)) {
    if (SETTING_INVERSION == h1200_state.cursor_pos) {
      h1200_state.cursor_value = h1200_settings.get_value(SETTING_INVERSION);
      encoder[RIGHT].setPos(h1200_state.cursor_value);
    }
    h1200_state.value_changed = 1;
    MENU_REDRAW = 1;
  }
}

void H1200_lowerButton() {
  if (h1200_settings.apply_value(
          SETTING_INVERSION, h1200_settings.get_value(SETTING_INVERSION) - 1)) {
    if (SETTING_INVERSION == h1200_state.cursor_pos) {
      h1200_state.cursor_value = h1200_settings.get_value(SETTING_INVERSION);
      encoder[RIGHT].setPos(h1200_state.cursor_value);
    }
    h1200_state.value_changed = 1;
    MENU_REDRAW = 1;
  }
}

void H1200_rightButton() {
  if (UI_MODE) {
    h1200_state.value_changed =
        h1200_settings.apply_value(h1200_state.cursor_pos,
                                   h1200_state.cursor_value);
    if (h1200_state.value_changed)
      MENU_REDRAW = 1;
  }
}

void H1200_leftButton() {
  h1200_state.display_notes = !h1200_state.display_notes;
  MENU_REDRAW = 1;
}

bool H1200_encoders() {
  bool changed = false;
  int value = encoder[LEFT].pos();
  if (value != h1200_state.cursor_pos) {
    if (value < 0) value = 0;
    else if (value >= SETTING_LAST) value = SETTING_LAST - 1;
    encoder[LEFT].setPos(value);
    h1200_state.cursor_pos = value;
    h1200_state.cursor_value = h1200_settings.get_value(h1200_state.cursor_pos);
    encoder[RIGHT].setPos(h1200_state.cursor_value);
    MENU_REDRAW = 1;
    changed = true;
  }

  value = encoder[RIGHT].pos();
  if (value != h1200_state.cursor_value) {
    h1200_state.cursor_value = H1200Settings::clamp_value(h1200_state.cursor_pos, value);
    encoder[RIGHT].setPos(h1200_state.cursor_value);
    MENU_REDRAW = 1;
    changed = true;
  }

  return changed;
}
