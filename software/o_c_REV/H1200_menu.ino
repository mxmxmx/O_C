const char *note_name(int note) {
  static const char *note_names[12] = { "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B " };
  return note_names[(note + 120) % 12];
}

const char *transform_names[] {
  "-", "P", "L", "R", "N", "S", "H"
};

void print_int(int value) {
  if (value >= 0) {
    u8g.print('+'); u8g.print(value);
  } else {
    u8g.print('-'); u8g.print(-value);
  }
}

void H1200_menu() {
   u8g.setFont(u8g_font_6x12);
   u8g.setColorIndex(1);
   u8g.setFontRefHeightText();
   u8g.setFontPosTop();

  uint8_t col_x = 96;
  uint8_t y = 0;
  uint8_t h = 11;

  const abstract_triad &current_chord = tonnetz_state.current_chord();

  if (menu_state.cursor_pos == 0) {
    u8g.drawBox(0, y, 32, h);
    u8g.setDefaultBackgroundColor();
    const int value = menu_state.cursor_value;
    u8g.setPrintPos(10, y);
    print_int(value);
  } else {
    u8g.setPrintPos(4, y);
    // current chord info
    u8g.setDefaultForegroundColor();
    if (menu_state.display_notes)
      u8g.print(note_name(tonnetz_state.root()));
    else
      u8g.print(tonnetz_state.root());
    u8g.print(mode_names[current_chord.mode()]);
  }

  u8g.setPrintPos(64, y);
  u8g.setDefaultForegroundColor();
  if (menu_state.display_notes) {
    for (size_t i=1; i < 4; ++i) {
      if (i > 1) u8g.print(' ');
      u8g.print(note_name(tonnetz_state.outputs(i)));
    }
  } else {
    for (size_t i=1; i < 4; ++i) {
      if (i > 1) u8g.print(' ');
      u8g.print(tonnetz_state.outputs(i));
    }
  }

  u8g.drawLine(0, 13, 128, 13);

  y = 2 * h - 4;
  for (int i = 1; i < SETTING_LAST; ++i, y+=h) {
    if (i == menu_state.cursor_pos) {
      u8g.drawBox(0, y, 128, h);
      u8g.setDefaultBackgroundColor();
    } else {
      u8g.setDefaultForegroundColor();
    }
    const settings::value_attr &attr = H1200Settings::value_attr(i);
    u8g.drawStr(10, y, attr.name);
    u8g.setPrintPos(col_x, y);
    int value = i == menu_state.cursor_pos
        ? menu_state.cursor_value
        : h1200_settings.get_value(i);
    if (attr.value_names)
      u8g.drawStr(col_x, y, attr.value_names[value]);
    else
      print_int(value);
  }
}

void H1200_screensaver() {

  uint8_t y = 0;

  const abstract_triad &current_chord = tonnetz_state.current_chord();
  const String &last_transform = tonnetz_state.last_trans().trim();
  const float pi = 3.14159265358979323846;
  const float semitone_radians = (2 * pi / 12.0);

  u8g.setFont(u8g_font_timB14);
  u8g.setColorIndex(1);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  u8g.setDefaultForegroundColor();
 
  // u8g.setPrintPos(4, y);
  // current chord info
  // u8g.print(tonnetz_state.root() / 12);
  // u8g.setPrintPos(4, y + 20);
  // u8g.print(note_name(tonnetz_state.root()));
  // u8g.print(mode_names[current_chord.mode()]);
  u8g.setPrintPos(88, y + 50);
  u8g.print(last_transform);

  u8g.drawCircle(32, 32, 30);

  u8g.setPrintPos(64, y);
  u8g.setDefaultForegroundColor();
  float first_x ;
  float first_y ;
  float prev_x ;
  float prev_y ;
  for (size_t i=1; i < 4; ++i) {
      u8g.setPrintPos(102, y + ((i - 1) * 16)) ;
      u8g.print(note_name(tonnetz_state.outputs(i)));
      float rads = ((tonnetz_state.outputs(i) + 120 - 3) % 12) * semitone_radians;
      float circ_x = 30*cos(rads) + 32;
      float circ_y = 30*sin(rads) + 32;
      u8g.drawDisc(circ_x, circ_y, 3);
      if (i == 1) {
        first_x = circ_x;
        first_y = circ_y;
      }
      if (i > 1) u8g.drawLine(prev_x, prev_y, circ_x, circ_y);
      prev_x = circ_x;
      prev_y = circ_y;
      if (i == 3) u8g.drawLine(first_x, first_y, circ_x, circ_y);
  }
  u8g.setDefaultForegroundColor();
  for (size_t i=1; i < 4; ++i) {
      u8g.setPrintPos(88, y + ((i - 1) * 16)) ;
      u8g.print(tonnetz_state.outputs(i) / 12) ;
  }

}
