#include "util_grid.h"

// Drive the tonnetz transformations from a grid of cells that contain the type
// of transformation and other goodies. Instead of stepping through the grid
// linearly on each clock, at vector (dx, dy) is used to determine the next
// cell. The cell coordinates use fixed-point internally with a configurable
// sub-step accuracy.
// 
// Since the grid is 5x5 and the sub-step accuracy a power-of-two, they might
// not align correctly so there is a cumulative error. As a work-around, the
// grid is given an epsilon and the selectable steps are restricted. Using odd
// dimensions should mean that each cell is reached eventually.
//
// "Vector sequencer" inspired by fcd72, see
// https://dmachinery.wordpress.com/2013/01/05/the-vector-sequencer/

#define FRACTIONAL_BITS 7
#define CLOCK_STEP_RES (0x1 << FRACTIONAL_BITS)
#define GRID_EPSILON 6

const size_t clock_fraction[] = {
  0, CLOCK_STEP_RES/8, 1+CLOCK_STEP_RES/7, CLOCK_STEP_RES/6, 1+CLOCK_STEP_RES/5, 1+CLOCK_STEP_RES/4, 1+CLOCK_STEP_RES/3, CLOCK_STEP_RES/2
};

const char *clock_fraction_names[] = {
  "", "1/8", "1/7", "1/6", "1/5", "1/4", "1/3", "1/2"
};

enum ECellSettings {
  CELL_SETTING_TRANSFORM,
  CELL_SETTING_TRANSPOSE,
  CELL_SETTING_INVERSION,
  CELL_SETTING_EVENT,
  CELL_SETTING_LAST
};

enum ECellEvent {
  CELL_EVENT_NONE,
  CELL_EVENT_LAST
};

struct TransformCell : public settings::SettingsBase<TransformCell, CELL_SETTING_LAST> {

  tonnetz::ETransformType transform() const {
    return static_cast<tonnetz::ETransformType>(values_[CELL_SETTING_TRANSFORM]);
  }

  int transpose() const {
    return values_[CELL_SETTING_TRANSPOSE];
  }

  int inversion() const {
    return values_[CELL_SETTING_INVERSION];
  }

  ECellEvent event() const {
    return static_cast<ECellEvent>(values_[CELL_SETTING_EVENT]);
  }
};

const char *cell_event_names[] = {
  "none"
};

/*static*/ template<>
const settings::value_attr settings::SettingsBase<TransformCell, CELL_SETTING_LAST>::value_attr_[] = {
  {0, tonnetz::TRANSFORM_NONE, tonnetz::TRANSFORM_LAST, "tra ", tonnetz::transform_names_str},
  {0, -12, 12, "off ", NULL},
  {0, -3, 3, "inv ", NULL},
  {0, CELL_EVENT_NONE, CELL_EVENT_LAST - 1, "evt ", cell_event_names}
};

enum EGridSettings {
  GRID_SETTING_DX,
  GRID_SETTING_DY,
  GRID_SETTING_MODE,
  GRID_SETTING_OCTAVE,
  GRID_SETTING_LAST
};

class H1200mk2State : public settings::SettingsBase<H1200mk2State, GRID_SETTING_LAST> {
public:
  static const size_t HISTORY_LENGTH = 4;

  void init() {
    init_defaults();
    grid.init();
    memset(&ui, 0, sizeof(ui));
    memset(history_, 0, sizeof(history_));
    history_tail_ = 0;
  }

  EMode mode() const {
    return static_cast<EMode>(values_[GRID_SETTING_MODE]);
  }

  int octave() const {
    return values_[GRID_SETTING_OCTAVE];
  }

  size_t dx() const {
    const int value = values_[GRID_SETTING_DX];
    return ((value / 8) * CLOCK_STEP_RES) + clock_fraction[value % 8];
  }

  size_t dy() const {
    const int value = values_[GRID_SETTING_DY];
    return ((value / 8) * CLOCK_STEP_RES) + clock_fraction[value % 8];
  }

  void clock();
  void reset();
  void render();

  CellGrid<TransformCell, 5, FRACTIONAL_BITS, GRID_EPSILON> grid;
  TonnetzState tonnetz_state;
  vec2<size_t> history_[HISTORY_LENGTH];
  size_t history_tail_;

  struct {
    int selected_row;
    int selected_col;
    bool edit_cell;
    size_t selected_param;
    size_t selected_cell_param;
  } ui;

  void push_history(const vec2<size_t> &pos) {
    size_t tail = (history_tail_ + 1) % HISTORY_LENGTH;
    history_[tail] = pos;
    history_tail_ = tail;
  }

  const vec2<size_t> &history(size_t index) const {
    return history_[(history_tail_ + HISTORY_LENGTH - index) % HISTORY_LENGTH];
  }
};

/*static*/ template<>
const settings::value_attr settings::SettingsBase<H1200mk2State, GRID_SETTING_LAST>::value_attr_[] = {
  {8, 0, 8*5 - 1, "dx   ", NULL},
  {4, 0, 8*5 - 1, "dy   ", NULL},
  {MODE_MAJOR, 0, MODE_LAST-1, "mode ", mode_names},
  {0, -3, 3, "oct  ", NULL}
};

H1200mk2State H1200mk2_state;

void H1200mk2_init() {
  init_circle_lut();
  H1200mk2_state.init();

  H1200mk2_state.grid.mutable_cell(0, 0).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_LAST);
  H1200mk2_state.grid.mutable_cell(1, 0).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_P);
  H1200mk2_state.grid.mutable_cell(4, 0).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_LAST);

  H1200mk2_state.grid.mutable_cell(0, 1).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_L);
  H1200mk2_state.grid.mutable_cell(1, 1).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_R);
  H1200mk2_state.grid.mutable_cell(2, 1).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_L);
  H1200mk2_state.grid.mutable_cell(3, 1).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_R);
  H1200mk2_state.grid.mutable_cell(4, 1).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_P);

  H1200mk2_state.grid.mutable_cell(0, 2).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_L);
  H1200mk2_state.grid.mutable_cell(1, 2).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_R);
  H1200mk2_state.grid.mutable_cell(2, 2).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_L);
  H1200mk2_state.grid.mutable_cell(3, 2).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_R);
  H1200mk2_state.grid.mutable_cell(4, 2).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_P);

  H1200mk2_state.reset();
}

void H1200mk2State::clock() {
  if (grid.move(dx(), dy())) {
    push_history(grid.current_pos());
  
    tonnetz::ETransformType transform = grid.current_cell().transform();
    if (transform >= tonnetz::TRANSFORM_LAST)
      tonnetz_state.reset(mode());
    else if (transform != tonnetz::TRANSFORM_NONE)
      tonnetz_state.apply_transformation(transform);

    MENU_REDRAW = 1;
  }
  render();
}

void H1200mk2State::reset() {
  grid.reset();
  push_history(grid.current_pos());
  tonnetz_state.reset(mode());
  render();

  MENU_REDRAW = 1;
}

void H1200mk2State::render() {
  int32_t sample = cvval[0];
  int root;
  if (sample < 0)
    root = 0;
  else if (sample < S_RANGE)
    root = sample >> 5;
  else
    root = RANGE;

  const TransformCell &current_cell = grid.current_cell();
  root += current_cell.transpose();
  root += octave() * 12;
  tonnetz_state.render(root, current_cell.inversion());

  OUTPUT_NOTE(0,set8565_CHA);
  OUTPUT_NOTE(1,set8565_CHB);
  OUTPUT_NOTE(2,set8565_CHC);
  OUTPUT_NOTE(3,set8565_CHD);
}

#define H1200_CLOCKIT() \
do { \
  if (CLK_STATE[TR1]) { CLK_STATE[TR1] = false; H1200mk2_state.clock(); } \
} while (0)

void H1200mk2_loop() {
  UI();
  H1200_CLOCKIT();
  if (_ADC) CV();
  H1200_CLOCKIT();
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  H1200_CLOCKIT();
  buttons(BUTTON_BOTTOM);
  H1200_CLOCKIT();
  buttons(BUTTON_TOP);
  H1200_CLOCKIT();
  buttons(BUTTON_LEFT);
  H1200_CLOCKIT();
  buttons(BUTTON_RIGHT);
  H1200_CLOCKIT();
}


static const uint8_t kGridXStart = 0;
static const uint8_t kGridYStart = 0;
static const uint8_t kGridH = 12;
static const uint8_t kGridW = 12;
static const uint8_t kMenuStartX = 64;
static const uint8_t kLineHeight = 11;

void H1200mk2_menu_cell() {
  u8g.setDefaultForegroundColor();
  u8g.setPrintPos(64, 0);
  u8g.drawLine(64, 13, 125, 13);
  u8g.print("CELL ");
  u8g.print(H1200mk2_state.ui.selected_col + 1);
  u8g.print(',');
  u8g.print(H1200mk2_state.ui.selected_row + 1);

  uint8_t y = 2 * kLineHeight - 4;
  for (size_t i = 0; i < CELL_SETTING_LAST; ++i, y += kLineHeight) {
    u8g.setDefaultForegroundColor();
    if (H1200mk2_state.ui.selected_cell_param == i) {
      u8g.drawBox(kMenuStartX, y, 62, kLineHeight);
      u8g.setDefaultBackgroundColor();
    }

    u8g.setPrintPos(kMenuStartX + 2, y);
    const settings::value_attr &attr = TransformCell::value_attr(i);
    const TransformCell &cell = H1200mk2_state.grid.at(H1200mk2_state.ui.selected_col, H1200mk2_state.ui.selected_row);
    u8g.print(attr.name);
    if (attr.value_names)
      u8g.print(attr.value_names[cell.get_value(i)]);
    else
      print_int(cell.get_value(i));
  }
}

void H1200mk2_menu_grid() {
  u8g.setDefaultForegroundColor();
  u8g.setPrintPos(64, 0);
  u8g.drawLine(64, 13, 125, 13);
  for (size_t i=1; i < 4; ++i) {
    if (i > 1) u8g.print(' ');
    u8g.print(note_name(H1200mk2_state.tonnetz_state.outputs(i)));
  }
  if (MODE_MAJOR == H1200mk2_state.tonnetz_state.current_chord().mode())
    u8g.print(" +");
  else
    u8g.print(" -");

  uint8_t y = 2 * kLineHeight - 4;
  for (size_t i = 0; i < GRID_SETTING_LAST; ++i, y += kLineHeight) {
    u8g.setDefaultForegroundColor();
    if (H1200mk2_state.ui.selected_param == i) {
      u8g.drawBox(kMenuStartX, y, 62, kLineHeight);
      u8g.setDefaultBackgroundColor();
    }

    u8g.setPrintPos(kMenuStartX + 2, y + 2);
    const settings::value_attr &attr = H1200mk2State::value_attr(i);
    u8g.print(attr.name);
    int value = H1200mk2_state.get_value(i);
    if (attr.value_names) {
      u8g.print(attr.value_names[value]);
    } else if (i <= GRID_SETTING_DY) {
      const int integral = value / 8;
      const int fraction = value % 8;
      if (integral || !fraction)
        u8g.print(value/8);
      if (fraction) {
          if (integral)
            u8g.print(' ');
          u8g.print(clock_fraction_names[fraction]);
        }
    } else {
      print_int(value);
    }
  }
}

void H1200mk2_menu() {
  u8g.setFont(u8g_font_6x12);
  u8g.setColorIndex(1);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  u8g.setDefaultForegroundColor();

  const vec2<size_t> current_pos = H1200mk2_state.grid.current_pos();

  uint8_t y = kGridYStart;
  for (size_t row = 0; row < 5; ++row, y += kGridH) {
    const TransformCell *cells = H1200mk2_state.grid.row(row);
    uint8_t x = kGridXStart;
    for (size_t col = 0; col < 5; ++col, x+=kGridW) {
      u8g.setPrintPos(x + 3, y + 2);
      u8g.setDefaultForegroundColor();
      if (row == current_pos.y && col == current_pos.x) {
        u8g.drawBox(x, y, kGridW, kGridH);
        u8g.setDefaultBackgroundColor();
      }

      u8g.print(tonnetz::transform_names[cells[col].transform()]);
    }
  }

  u8g.setDefaultForegroundColor();
  u8g.drawFrame(H1200mk2_state.ui.selected_col * kGridW,
                H1200mk2_state.ui.selected_row * kGridH, kGridW, kGridH);
  u8g.setDefaultBackgroundColor();
  u8g.drawFrame(H1200mk2_state.ui.selected_col * kGridW + 1,
                H1200mk2_state.ui.selected_row * kGridH + 1, kGridW - 2, kGridH - 2);

  if (H1200mk2_state.ui.edit_cell)
    H1200mk2_menu_cell();
  else
    H1200mk2_menu_grid();
}

static const uint8_t kScreenSaverX = 64 + 2;
static const uint8_t kScreenSaverY = 5;
static const uint8_t kScreenSaverGrid = 55 / 5;

void H1200mk2_screensaver() {
  u8g.setColorIndex(1);
  u8g.setDefaultForegroundColor();

  uint8_t normalized[3];
  for (size_t i=0; i < 3; ++i) {
    int value = H1200mk2_state.tonnetz_state.outputs(i + 1);
    value = (value + 120) % 12;
    normalized[i] = value;
  }
  visualize_pitch_classes(normalized);

  vec2<size_t> last_pos = H1200mk2_state.history(0);
  u8g.drawCircle(kScreenSaverX + last_pos.x * kScreenSaverGrid, kScreenSaverY + last_pos.y * kScreenSaverGrid, 3);
  for (size_t i = 1; i < H1200mk2State::HISTORY_LENGTH; ++i) {
    const vec2<size_t> &current = H1200mk2_state.history(i);
    u8g.drawLine(kScreenSaverX + last_pos.x * kScreenSaverGrid, kScreenSaverY + last_pos.y * kScreenSaverGrid,
                 kScreenSaverX + current.x * kScreenSaverGrid, kScreenSaverY + current.y * kScreenSaverGrid);

    u8g.drawCircle(kScreenSaverX + current.x * kScreenSaverGrid, kScreenSaverY + current.y * kScreenSaverGrid, 2);
    last_pos = current;
  }

  uint8_t y = 0;
  for (size_t i = 0; i < TonnetzState::HISTORY_LENGTH; ++i, y += 12) {
    u8g.setPrintPos(128-7, y);
    u8g.print(H1200mk2_state.tonnetz_state.history(i).str[1]);
  }
}

void H1200mk2_resume() {
  encoder[LEFT].setPos(H1200mk2_state.ui.selected_row * 5 + H1200mk2_state.ui.selected_col);
  if (H1200mk2_state.ui.edit_cell) {
    const TransformCell &cell = H1200mk2_state.grid.at(H1200mk2_state.ui.selected_col, H1200mk2_state.ui.selected_row);
    encoder[RIGHT].setPos(cell.get_value(H1200mk2_state.ui.selected_cell_param));
  } else {
    encoder[RIGHT].setPos(H1200mk2_state.get_value(H1200mk2_state.ui.selected_param));
  }
}

void H1200mk2_topButton() {
  H1200mk2_state.reset();
}

void H1200mk2_lowerButton() {
  H1200mk2_state.clock();
}

void H1200mk2_rightButton() {
  if (H1200mk2_state.ui.edit_cell) {
    ++H1200mk2_state.ui.selected_cell_param;
    if (H1200mk2_state.ui.selected_cell_param >= CELL_SETTING_LAST)
      H1200mk2_state.ui.selected_cell_param = 0;
      const TransformCell &cell = H1200mk2_state.grid.at(H1200mk2_state.ui.selected_col, H1200mk2_state.ui.selected_row);
      encoder[RIGHT].setPos(cell.get_value(H1200mk2_state.ui.selected_cell_param));
  } else {
    ++H1200mk2_state.ui.selected_param;
    if (H1200mk2_state.ui.selected_param >= GRID_SETTING_LAST)
      H1200mk2_state.ui.selected_param = 0;
     encoder[RIGHT].setPos(H1200mk2_state.get_value(H1200mk2_state.ui.selected_param));
 }
}

void H1200mk2_leftButton() {
  bool edit_cell = !H1200mk2_state.ui.edit_cell;
  if (edit_cell) {
    const TransformCell &cell = H1200mk2_state.grid.at(H1200mk2_state.ui.selected_col, H1200mk2_state.ui.selected_row);
    encoder[RIGHT].setPos(cell.get_value(H1200mk2_state.ui.selected_cell_param));
  } else {
    encoder[RIGHT].setPos(H1200mk2_state.get_value(H1200mk2_state.ui.selected_param));
  }

  H1200mk2_state.ui.edit_cell = edit_cell;
}

bool H1200mk2_encoders() {
  bool changed = false;

  int value = encoder[LEFT].pos();
  int selected = H1200mk2_state.ui.selected_row * 5 + H1200mk2_state.ui.selected_col;
  if (value != selected) {
    selected = value;
    while (selected < 0) selected += 25;
    while (selected > 24) selected -= 25;
    H1200mk2_state.ui.selected_row = selected / 5;
    H1200mk2_state.ui.selected_col = selected - H1200mk2_state.ui.selected_row * 5;
    encoder[LEFT].setPos(selected);
    MENU_REDRAW = 1;
    changed = true;
    if (H1200mk2_state.ui.edit_cell) {
      const TransformCell &cell = H1200mk2_state.grid.at(H1200mk2_state.ui.selected_col, H1200mk2_state.ui.selected_row);
      encoder[RIGHT].setPos(cell.get_value(H1200mk2_state.ui.selected_cell_param));
    } else {
      encoder[RIGHT].setPos(H1200mk2_state.get_value(H1200mk2_state.ui.selected_param));
    }
  }

  value = encoder[RIGHT].pos();
  if (H1200mk2_state.ui.edit_cell) {
    size_t selected_cell_param = H1200mk2_state.ui.selected_cell_param;
    TransformCell &cell = H1200mk2_state.grid.mutable_cell(H1200mk2_state.ui.selected_col, H1200mk2_state.ui.selected_row);
    changed = cell.apply_value(selected_cell_param, value);
    encoder[RIGHT].setPos(cell.get_value(selected_cell_param));
  } else {
    size_t selected_param = H1200mk2_state.ui.selected_param;
    changed = H1200mk2_state.apply_value(selected_param, value);
    encoder[RIGHT].setPos(H1200mk2_state.get_value(selected_param));
  }

  return changed;
}
