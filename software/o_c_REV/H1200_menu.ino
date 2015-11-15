const char *note_name(int note) {
  static const char *note_names[12] = { "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B " };
  return note_names[note % 12];
}

const char *output_mode_names[] = {
  "CHORD",
  "TUNE"
};

const char *trigger_mode_names[] = {
  "@PLR"
};

const char *mode_names[] = {
  " + ", " - "
};

struct settings_attr {
  const char *name;
  const char **value_names;
};

void H1200_menu() {

  static const settings_attr settings[4] = {
    {"MODE", mode_names},
    {"INVERSION", NULL },
    {"TRIGGERS", trigger_mode_names},
    {"OUTPUT", output_mode_names}
  };

  uint8_t col_x = 96;
  uint8_t y = 0;
  uint8_t h = 11;

  const abstract_triad &current_chord = tonnetz_state.current_chord();

  // current chord info
  u8g.setPrintPos(10, y);
  u8g.print(tonnetz_state.root());
  u8g.print(mode_names[current_chord.mode()]);
  for (size_t i=1; i < 4; ++i) {
    if (i > 1) u8g.print('-');
    u8g.print(tonnetz_state.outputs(i));
//    u8g.print(note_name(tonnetz_state.outputs(i)));
  }

  u8g.drawLine(0, 13, 128, 13);

  y = 2 * h - 4;
  for (size_t i = 0; i < 4; ++i, y+=h) {
   u8g.setDefaultForegroundColor();
   if (i == 0) {
    u8g.drawBox(0, y, 128, h);
    u8g.setDefaultBackgroundColor();
  }
  const settings_attr &attr = settings[i];
  u8g.drawStr(10, y, attr.name);
  if (attr.value_names)
    u8g.drawStr(col_x, y, attr.value_names[h1200_settings.get_value(i)]);
//    else
//      u8g.drawStr(110, y, h12k_settings.get_value(i));
  }
}
