
/*static*/ const char *note_names[12] = { "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B " };

const char *note_name(int note) {
  return note_names[(note + 120) % 12];
}

void H1200_menu() {
   u8g.setFont(u8g_font_6x12);
   u8g.setColorIndex(1);
   u8g.setFontRefHeightText();
   u8g.setFontPosTop();

  uint8_t col_x = 96;
  uint8_t y = 0;
  uint8_t h = 11;

  const abstract_triad &current_chord = h1200_state.tonnetz_state.current_chord();

  if (h1200_state.cursor_pos == 0) {
    u8g.drawBox(0, y, 32, h);
    u8g.setDefaultBackgroundColor();
    const int value = h1200_state.cursor_value;
    u8g.setPrintPos(10, y);
    print_int(value);
  } else {
    u8g.setPrintPos(4, y);
    // current chord info
    u8g.setDefaultForegroundColor();
    if (h1200_state.display_notes)
      u8g.print(note_name(h1200_state.tonnetz_state.root()));
    else
      u8g.print(h1200_state.tonnetz_state.root());
    u8g.print(mode_names[current_chord.mode()]);
  }

  u8g.setPrintPos(64, y);
  u8g.setDefaultForegroundColor();
  if (h1200_state.display_notes) {
    for (size_t i=1; i < 4; ++i) {
      if (i > 1) u8g.print(' ');
      u8g.print(note_name(h1200_state.tonnetz_state.outputs(i)));
    }
  } else {
    for (size_t i=1; i < 4; ++i) {
      if (i > 1) u8g.print(' ');
      u8g.print(h1200_state.tonnetz_state.outputs(i));
    }
  }

  u8g.drawLine(0, 13, 128, 13);

  y = 2 * h - 4;
  for (int i = 1; i < H1200_SETTING_LAST; ++i, y+=h) {
    if (i == h1200_state.cursor_pos) {
      u8g.drawBox(0, y, 128, h);
      u8g.setDefaultBackgroundColor();
    } else {
      u8g.setDefaultForegroundColor();
    }
    const settings::value_attr &attr = H1200Settings::value_attr(i);
    u8g.drawStr(10, y, attr.name);
    u8g.setPrintPos(col_x, y);
    int value = i == h1200_state.cursor_pos
        ? h1200_state.cursor_value
        : h1200_settings.get_value(i);
    if (attr.value_names)
      u8g.drawStr(col_x, y, attr.value_names[value]);
    else
      print_int(value);
  }
}

static const uint8_t note_circle_x = 32;
static const uint8_t note_circle_y = 32;
static const uint8_t note_circle_r = 28;

struct coords {
  uint8_t x, y;
} circle_pos_lut[12];

void init_circle_lut() {
  static const float pi = 3.14159265358979323846f;
  static const float semitone_radians = (2.f * pi / 12.f);

  for (int i = 0; i < 12; ++i) {
    float rads = ((i + 12 - 3) % 12) * semitone_radians;
    float x = note_circle_r * cos(rads);
    float y = note_circle_r * sin(rads);
    circle_pos_lut[i].x = note_circle_x + x;
    circle_pos_lut[i].y = note_circle_y + y;
  }
}
const uint8_t circle_disk_bitmap[] = {
  0, 0x18, 0x3c, 0x7e, 0x7e, 0x3c, 0x18, 0
};

void visualize_pitch_classes(uint8_t *normalized) {
  u8g.drawCircle(note_circle_x, note_circle_y, note_circle_r);

  coords last_pos = circle_pos_lut[normalized[0]];
  for (size_t i = 1; i < 3; ++i) {
    u8g.drawBitmap(last_pos.x - 3, last_pos.y - 3, 1, 8, circle_disk_bitmap);
    const coords &current_pos = circle_pos_lut[normalized[i]];
    u8g.drawLine(last_pos.x, last_pos.y, current_pos.x, current_pos.y);
    last_pos = current_pos;
  }
  u8g.drawLine(last_pos.x, last_pos.y, circle_pos_lut[normalized[0]].x, circle_pos_lut[normalized[0]].y);
  u8g.drawBitmap(last_pos.x - 3, last_pos.y - 3, 1, 8, circle_disk_bitmap);

}

void H1200_screensaver() {

  uint8_t y = 0;
  static const uint8_t x_col_0 = 66;
  static const uint8_t x_col_1 = 66 + 24;
  static const uint8_t x_col_2 = 66 + 38;
  static const uint8_t line_h = 16;

  //u8g.setFont(u8g_font_timB12); BBX 19x27
  u8g.setFont(u8g_font_10x20); // fixed-width makes positioning a bit easier
  u8g.setColorIndex(1);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  u8g.setDefaultForegroundColor();
 
  uint8_t normalized[3];
  y = 8;
  for (size_t i=0; i < 3; ++i, y += line_h) {
    int value = h1200_state.tonnetz_state.outputs(i + 1);

    u8g.setPrintPos(x_col_1, y);
    u8g.print(value / 12);

    value = (value + 120) % 12;
    u8g.setPrintPos(x_col_2, y);
    u8g.print(note_names[value]);
    normalized[i] = value;
  }
  y = 0;
  for (size_t i = 0; i < TonnetzState::HISTORY_LENGTH; ++i, y += line_h) {
    u8g.setPrintPos(x_col_0, y);
    u8g.print(h1200_state.tonnetz_state.history(i).str);
  }

  visualize_pitch_classes(normalized);
}
