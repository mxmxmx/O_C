#include "OC_bitmaps.h"
#include "OC_strings.h"

const char *note_name(int note) {
  return OC::Strings::note_names[(note + 120) % 12];
}

void H1200_menu() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem
  graphics.setFont(UI_DEFAULT_FONT);

  const abstract_triad &current_chord = h1200_state.tonnetz_state.current_chord();

  static const uint8_t kStartX = 0;

  UI_DRAW_TITLE(kStartX);
  if (h1200_state.display_notes)
    graphics.print(note_name(h1200_state.tonnetz_state.root()));
  else
    graphics.print(h1200_state.tonnetz_state.root());
  graphics.print(mode_names[current_chord.mode()]);

  graphics.setPrintPos(64, kUiTitleTextY);
  if (h1200_state.display_notes) {
    for (size_t i=1; i < 4; ++i) {
      if (i > 1) graphics.print(' ');
      graphics.print(note_name(h1200_state.tonnetz_state.outputs(i)));
    }
  } else {
    for (size_t i=1; i < 4; ++i) {
      if (i > 1) graphics.print(' ');
      graphics.print(h1200_state.tonnetz_state.outputs(i));
    }
  }

  int first_visible = h1200_state.cursor_pos - kUiVisibleItems + 1;
  if (first_visible < 0)
    first_visible = 0;

  UI_START_MENU(kStartX);
  UI_BEGIN_ITEMS_LOOP(kStartX, first_visible, H1200_SETTING_LAST, h1200_state.cursor_pos, 0)
    const settings::value_attr &attr = H1200Settings::value_attr(current_item);
    UI_DRAW_SETTING(attr, h1200_settings.get_value(current_item), kUiWideMenuCol1X);
  UI_END_ITEMS_LOOP();

  GRAPHICS_END_FRAME();
}

void H1200_screensaver() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem

  uint8_t y = 0;
  static const uint8_t x_col_0 = 66;
  static const uint8_t x_col_1 = 66 + 24;
  static const uint8_t x_col_2 = 66 + 38;
  static const uint8_t line_h = 16;

  //u8g.setFont(u8g_font_timB12); BBX 19x27
  //graphics.setFont(u8g_font_10x20); // fixed-width makes positioning a bit easier
 
  uint32_t history = h1200_state.tonnetz_state.history();

  uint8_t normalized[3];
  y = 8;
  for (size_t i=0; i < 3; ++i, y += line_h) {
    int value = h1200_state.tonnetz_state.outputs(i + 1);

    graphics.setPrintPos(x_col_1, y);
    graphics.print(value / 12);

    value = (value + 120) % 12;
    graphics.setPrintPos(x_col_2, y);
    graphics.print(OC::Strings::note_names[value]);
    normalized[i] = value;
  }
  y = 0;

  size_t len = 4;
  while (len--) {
    graphics.setPrintPos(x_col_0, y);
    graphics.print(history & 0x80 ? '+' : '-');
    graphics.print(tonnetz::transform_names[static_cast<tonnetz::ETransformType>(history & 0x7f)]);
    y += line_h;
    history >>= 8;
  }

  visualize_pitch_classes(normalized);

  GRAPHICS_END_FRAME();
}
