#include "util_grid.h"
#include "util_settings.h"
#include "util_ui.h"
#include "tonnetz_state.h"

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

#define FRACTIONAL_BITS 24
#define CLOCK_STEP_RES (0x1 << FRACTIONAL_BITS)
#define GRID_EPSILON 6
#define GRID_SIZE 5

const size_t clock_fraction[] = {
  0, CLOCK_STEP_RES/8, 1+CLOCK_STEP_RES/7, CLOCK_STEP_RES/6, 1+CLOCK_STEP_RES/5, 1+CLOCK_STEP_RES/4, 1+CLOCK_STEP_RES/3, CLOCK_STEP_RES/2
};

const char *clock_fraction_names[] = {
  "", "1/8", "1/7", "1/6", "1/5", "1/4", "1/3", "1/2"
};

static const uint8_t TRIGGER_MASK_GRID = 0x1;
static const uint8_t TRIGGER_MASK_ARP = 0x2;
static const uint32_t kTriggerOutMs = 5;

enum ECellSettings {
  CELL_SETTING_TRANSFORM,
  CELL_SETTING_TRANSPOSE,
  CELL_SETTING_INVERSION,
  CELL_SETTING_EVENT,
  CELL_SETTING_LAST
};

enum ECellEvent {
  CELL_EVENT_NONE,
  CELL_EVENT_RAND_TRANFORM,
  CELL_EVENT_LAST
};

#define CELL_MAX_INVERSION 3
#define CELL_MIN_INVERSION -3

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
  "none",
  "rndT"
};

/*static*/ template<>
const settings::value_attr settings::SettingsBase<TransformCell, CELL_SETTING_LAST>::value_attr_[] = {
  {0, tonnetz::TRANSFORM_NONE, tonnetz::TRANSFORM_LAST, "tra  ", tonnetz::transform_names_str},
  {0, -12, 12, "off  ", NULL},
  {0, CELL_MIN_INVERSION, CELL_MAX_INVERSION, "inv  ", NULL},
  {0, CELL_EVENT_NONE, CELL_EVENT_LAST - 1, "evt  ", cell_event_names}
};

enum EGridSettings {
  GRID_SETTING_DX,
  GRID_SETTING_DY,
  GRID_SETTING_MODE,
  GRID_SETTING_OCTAVE,
  GRID_SETTING_OUTPUTMODE,
  GRID_SETTING_CLEARMODE,
  GRID_SETTING_LAST
};

enum EOutputAMode {
  OUTPUTA_MODE_ROOT,
  OUTPUTA_MODE_TRIG,
  OUTPUTA_MODE_ARP,
  OUTPUTA_MODE_STRUM,
  OUTPUTA_MODE_LAST
};

// What happens on long left press
enum EClearMode {
  CLEAR_MODE_ZERO, // empty cells
  CLEAR_MODE_RAND_TRANSFORM, // random transform
  CLEAR_MODE_RAND_TRANSFORM_EV, // random tranform event
  CLEAR_MODE_LAST
};

class AutomatonnetzState : public settings::SettingsBase<AutomatonnetzState, GRID_SETTING_LAST> {
public:
  static const size_t HISTORY_LENGTH = 4;

  void init() {
    init_defaults();
    memset(cells_, 0, sizeof(cells_));
    grid.init(cells_);
    memset(&ui, 0, sizeof(ui));
    memset(history_, 0, sizeof(history_));
    history_tail_ = 0;
  }

  void clear_grid() {
    switch (clear_mode()) {
      case CLEAR_MODE_ZERO:
        memset(cells_, 0, sizeof(cells_));
      break;
      case CLEAR_MODE_RAND_TRANSFORM:
        memset(cells_, 0, sizeof(cells_));
        for (size_t i = 0; i < GRID_SIZE*GRID_SIZE; ++i)
          cells_[i].apply_value(CELL_SETTING_TRANSFORM, random(tonnetz::TRANSFORM_LAST + 1));
        break;
      case CLEAR_MODE_RAND_TRANSFORM_EV:
        memset(cells_, 0, sizeof(cells_));
        for (size_t i = 0; i < GRID_SIZE*GRID_SIZE; ++i)
          cells_[i].apply_value(CELL_SETTING_EVENT, CELL_EVENT_RAND_TRANFORM);
        break;
      default:
      break;
    }
  }

  size_t dx() const {
    const int value = values_[GRID_SETTING_DX];
    return ((value / 8) * CLOCK_STEP_RES) + clock_fraction[value % 8];
  }

  size_t dy() const {
    const int value = values_[GRID_SETTING_DY];
    return ((value / 8) * CLOCK_STEP_RES) + clock_fraction[value % 8];
  }

  EMode mode() const {
    return static_cast<EMode>(values_[GRID_SETTING_MODE]);
  }

  int octave() const {
    return values_[GRID_SETTING_OCTAVE];
  }

  EOutputAMode output_mode() const {
    return static_cast<EOutputAMode>(values_[GRID_SETTING_OUTPUTMODE]);
  }

  EClearMode clear_mode() const {
    return static_cast<EClearMode>(values_[GRID_SETTING_CLEARMODE]);
  }

  void clock(uint8_t triggers);
  void reset();
  void render(bool triggered);
  void update_trigger_out();

  TransformCell cells_[GRID_SIZE * GRID_SIZE];
  CellGrid<TransformCell, GRID_SIZE, FRACTIONAL_BITS, GRID_EPSILON> grid;
  TonnetzState tonnetz_state;
  vec2<size_t> history_[HISTORY_LENGTH];
  size_t history_tail_;
  uint32_t trigger_out_millis_;
  int arp_index_;

  struct {
    int selected_row;
    int selected_col;
    bool edit_cell;
    int selected_param;
    int selected_cell_param;
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

extern const char * const mode_names[];
const char * const outputa_mode_names[] = {
  "root",
  "trig",
  "arp",
  "strm"
};

const char * const clear_mode_names[] = {
  "zero", "rT", "rTev"
};

/*static*/ template<>
const settings::value_attr settings::SettingsBase<AutomatonnetzState, GRID_SETTING_LAST>::value_attr_[] = {
  {8, 0, 8*GRID_SIZE - 1, "dx   ", NULL},
  {4, 0, 8*GRID_SIZE - 1, "dy   ", NULL},
  {MODE_MAJOR, 0, MODE_LAST-1, "mode ", mode_names},
  {0, -3, 3, "oct  ", NULL},
  {OUTPUTA_MODE_ROOT, OUTPUTA_MODE_ROOT, OUTPUTA_MODE_LAST - 1, "outA ", outputa_mode_names},
  {CLEAR_MODE_ZERO, CLEAR_MODE_ZERO, CLEAR_MODE_LAST - 1, "clr  ", clear_mode_names},
};

AutomatonnetzState automatonnetz_state;

static const size_t AUTOMATONNETZ_SETTINGS_SIZE =
  sizeof(int8_t) * GRID_SETTING_LAST +
  GRID_SIZE * GRID_SIZE * sizeof(int8_t) * CELL_SETTING_LAST;


void Automatonnetz_init() {
  init_circle_lut();
  automatonnetz_state.init();

  automatonnetz_state.grid.mutable_cell(0, 0).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_LAST);
  automatonnetz_state.grid.mutable_cell(1, 0).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_P);
  automatonnetz_state.grid.mutable_cell(4, 0).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_LAST);

  automatonnetz_state.grid.mutable_cell(0, 1).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_L);
  automatonnetz_state.grid.mutable_cell(1, 1).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_R);
  automatonnetz_state.grid.mutable_cell(2, 1).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_L);
  automatonnetz_state.grid.mutable_cell(3, 1).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_R);
  automatonnetz_state.grid.mutable_cell(4, 1).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_P);

  automatonnetz_state.grid.mutable_cell(0, 2).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_L);
  automatonnetz_state.grid.mutable_cell(1, 2).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_R);
  automatonnetz_state.grid.mutable_cell(2, 2).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_L);
  automatonnetz_state.grid.mutable_cell(3, 2).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_R);
  automatonnetz_state.grid.mutable_cell(4, 2).apply_value(CELL_SETTING_TRANSFORM, tonnetz::TRANSFORM_P);

  automatonnetz_state.reset();
}

void AutomatonnetzState::clock(uint8_t triggers) {
  bool triggered = false;
  if (triggers & TRIGGER_MASK_GRID) {
    switch (grid.current_cell().event()) {
      case CELL_EVENT_RAND_TRANFORM:
        grid.mutable_current_cell().apply_value(CELL_SETTING_TRANSFORM, random(tonnetz::TRANSFORM_LAST + 1));
        break;
      default:
        break;
    }

    if (grid.move(dx(), dy())) {
      push_history(grid.current_pos());
  
      tonnetz::ETransformType transform = grid.current_cell().transform();
      if (transform >= tonnetz::TRANSFORM_LAST) {
        tonnetz_state.reset(mode());
        triggered = true;
      }
      else if (transform != tonnetz::TRANSFORM_NONE) {
        tonnetz_state.apply_transformation(transform);
        triggered = true;
      }

      if (triggered) {
        if (OUTPUTA_MODE_STRUM == output_mode())
          arp_index_ = 0;
      }

      MENU_REDRAW = 1;
    }
  } else if ((triggers & TRIGGER_MASK_ARP) && digitalReadFast(TR4)) {
    // arp disabled if TR4 high, gpio is inverted
    if (arp_index_ < 2)
        ++arp_index_;
    else if (OUTPUTA_MODE_ARP == output_mode())
      arp_index_ = 0;
  }

  render(triggered);
}

void AutomatonnetzState::reset() {
  grid.reset();
  push_history(grid.current_pos());
  tonnetz_state.reset(mode());
  arp_index_ = 0;
  render(true);

  MENU_REDRAW = 1;
}

#define AT_OUTPUT_NOTE(i,dac) \
do { \
  int note = tonnetz_state.outputs(i); \
  while (note > RANGE) note -= 12; \
  while (note < 0) note += 12; \
  const uint16_t dac_code = semitones[note]; \
  DAC::set<dac>(dac_code); \
} while (0)


void AutomatonnetzState::render(bool triggered) {
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

  int inversion = current_cell.inversion() + cvval[3];
  if (inversion > CELL_MAX_INVERSION * 2)
    inversion = CELL_MAX_INVERSION * 2;
  else if (inversion < CELL_MIN_INVERSION * 2)
    inversion = CELL_MIN_INVERSION * 2;
  tonnetz_state.render(root, inversion);

  switch (output_mode()) {
    case OUTPUTA_MODE_ROOT:
      AT_OUTPUT_NOTE(0,DAC_CHANNEL_A);
      break;
    case OUTPUTA_MODE_TRIG:
      if (triggered) {
        trigger_out_millis_ = millis();
        DAC::set<DAC_CHANNEL_A>(octaves[_ZERO + 5]);
      }
      break;
    case OUTPUTA_MODE_ARP:
    case OUTPUTA_MODE_STRUM:
      AT_OUTPUT_NOTE(arp_index_ + 1,DAC_CHANNEL_A);
      break;
    case OUTPUTA_MODE_LAST:
    default:
      break;
  }

  AT_OUTPUT_NOTE(1,DAC_CHANNEL_B);
  AT_OUTPUT_NOTE(2,DAC_CHANNEL_C);
  AT_OUTPUT_NOTE(3,DAC_CHANNEL_D);
}

void AutomatonnetzState::update_trigger_out() {
  // TODO Allow re-triggering?
  if (trigger_out_millis_ && millis() - trigger_out_millis_ > kTriggerOutMs) {
    DAC::set<DAC_CHANNEL_A>(octaves[_ZERO]);
    trigger_out_millis_ = 0;
  }
}

#define AT() \
do { \
  automatonnetz_state.update_trigger_out(); \
  uint8_t triggers = 0; \
  if (CLK_STATE[TR1]) { CLK_STATE[TR1] = false; triggers |= TRIGGER_MASK_GRID; } \
  if (CLK_STATE[TR2]) { CLK_STATE[TR2] = false; triggers |= TRIGGER_MASK_ARP; } \
  if (triggers) \
    automatonnetz_state.clock(triggers); \
} while (0)

void Automatonnetz_loop() {
  UI();
  AT();
  if (_ADC) CV();
  AT();
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  AT();
  buttons(BUTTON_BOTTOM);
  AT();
  buttons(BUTTON_TOP);
  AT();
  buttons(BUTTON_LEFT);
  AT();
  buttons(BUTTON_RIGHT);
  AT();
}

static const uint8_t kGridXStart = 0;
static const uint8_t kGridYStart = 0;
static const uint8_t kGridH = 12;
static const uint8_t kGridW = 12;
static const uint8_t kMenuStartX = 64;
static const uint8_t kLineHeight = 11;

void Automatonnetz_menu_cell() {

  UI_DRAW_TITLE(kMenuStartX);
  graphics.print("CELL ");
  graphics.print(automatonnetz_state.ui.selected_col + 1);
  graphics.print(',');
  graphics.print(automatonnetz_state.ui.selected_row + 1);

  UI_START_MENU(kMenuStartX);
  UI_BEGIN_ITEMS_LOOP(kMenuStartX, 0, CELL_SETTING_LAST, automatonnetz_state.ui.selected_cell_param, 0)
    const TransformCell &cell = automatonnetz_state.grid.at(automatonnetz_state.ui.selected_col, automatonnetz_state.ui.selected_row);
    const settings::value_attr &attr = TransformCell::value_attr(current_item);
    UI_DRAW_SETTING(attr, cell.get_value(current_item), 0);
  UI_END_ITEMS_LOOP();
}

void Automatonnetz_menu_grid() {
  UI_DRAW_TITLE(kMenuStartX);
  for (size_t i=1; i < 4; ++i) {
    if (i > 1) graphics.print(' ');
    graphics.print(note_name(automatonnetz_state.tonnetz_state.outputs(i)));
  }
  if (MODE_MAJOR == automatonnetz_state.tonnetz_state.current_chord().mode())
    graphics.print(" +");
  else
    graphics.print(" -");

  int first_visible = automatonnetz_state.ui.selected_param - kUiVisibleItems + 1;
  if (first_visible < 0)
    first_visible = 0;

  UI_START_MENU(kMenuStartX);
  UI_BEGIN_ITEMS_LOOP(kMenuStartX, first_visible, GRID_SETTING_LAST, automatonnetz_state.ui.selected_param, 0)

    const settings::value_attr &attr = AutomatonnetzState::value_attr(current_item);
    graphics.print(attr.name);
    int value = automatonnetz_state.get_value(current_item);
    if (attr.value_names) {
      graphics.print(attr.value_names[value]);
    } else if (i <= GRID_SETTING_DY) {
      const int integral = value / 8;
      const int fraction = value % 8;
      if (integral || !fraction)
        graphics.print((char)('0' + value/8));
      if (fraction) {
          if (integral)
            graphics.print(' ');
          graphics.print(clock_fraction_names[fraction]);
        }
    } else {
      graphics.print_int(value);
    }

  UI_END_ITEMS_LOOP();
}

void Automatonnetz_menu() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem

  graphics.setFont(UI_DEFAULT_FONT);

  const vec2<size_t> current_pos = automatonnetz_state.grid.current_pos();

  uint8_t y = kGridYStart;
  for (size_t row = 0; row < GRID_SIZE; ++row, y += kGridH) {
    const TransformCell *cells = automatonnetz_state.grid.row(row);
    uint8_t x = kGridXStart;
    for (size_t col = 0; col < GRID_SIZE; ++col, x+=kGridW) {
      graphics.setPrintPos(x + 3, y + 2);
      graphics.setDefaultForegroundColor();
      if (row == current_pos.y && col == current_pos.x) {
        graphics.drawBox(x, y, kGridW, kGridH);
        graphics.setDefaultBackgroundColor();
      }

      graphics.print(tonnetz::transform_names[cells[col].transform()]);
    }
  }

  graphics.setDefaultForegroundColor();
  graphics.drawFrame(automatonnetz_state.ui.selected_col * kGridW,
                automatonnetz_state.ui.selected_row * kGridH, kGridW, kGridH);
  graphics.setDefaultBackgroundColor();
  graphics.drawFrame(automatonnetz_state.ui.selected_col * kGridW + 1,
                automatonnetz_state.ui.selected_row * kGridH + 1, kGridW - 2, kGridH - 2);

  if (automatonnetz_state.ui.edit_cell)
    Automatonnetz_menu_cell();
  else
    Automatonnetz_menu_grid();

  GRAPHICS_END_FRAME();
}

static const uint8_t kScreenSaverX = 64 + 2;
static const uint8_t kScreenSaverY = 5;
static const uint8_t kScreenSaverGrid = 55 / 5;

void Automatonnetz_screensaver() {
  GRAPHICS_BEGIN_FRAME(false);

#if 0
  uint8_t normalized[3];
  for (size_t i=0; i < 3; ++i) {
    int value = automatonnetz_state.tonnetz_state.outputs(i + 1);
    value = (value + 120) % 12;
    normalized[i] = value;
  }
  visualize_pitch_classes(normalized);

  vec2<size_t> last_pos = automatonnetz_state.history(0);
  u8g.drawCircle(kScreenSaverX + last_pos.x * kScreenSaverGrid, kScreenSaverY + last_pos.y * kScreenSaverGrid, 3);
  for (size_t i = 1; i < AutomatonnetzState::HISTORY_LENGTH; ++i) {
    const vec2<size_t> &current = automatonnetz_state.history(i);
    u8g.drawLine(kScreenSaverX + last_pos.x * kScreenSaverGrid, kScreenSaverY + last_pos.y * kScreenSaverGrid,
                 kScreenSaverX + current.x * kScreenSaverGrid, kScreenSaverY + current.y * kScreenSaverGrid);

    u8g.drawCircle(kScreenSaverX + current.x * kScreenSaverGrid, kScreenSaverY + current.y * kScreenSaverGrid, 2);
    last_pos = current;
  }
#endif

  uint8_t y = 0;
  for (size_t i = 0; i < TonnetzState::HISTORY_LENGTH; ++i, y += 12) {
    graphics.setPrintPos(128-7, y);
    graphics.print(automatonnetz_state.tonnetz_state.history(i).str[1]);
  }

  GRAPHICS_END_FRAME();
}

size_t Automatonnetz_save(char *storage) {
  size_t used = automatonnetz_state.save<int8_t>(storage);
  for (size_t cell = 0; cell < GRID_SIZE*GRID_SIZE; ++cell)
    used += automatonnetz_state.cells_[cell].save<int8_t>(storage + used);

  return used;
}

size_t Automatonnetz_restore(const char *storage) {
  size_t used = automatonnetz_state.restore<int8_t>(storage);
  for (size_t cell = 0; cell < GRID_SIZE * GRID_SIZE; ++cell)
    used += automatonnetz_state.cells_[cell].restore<int8_t>(storage + used);

  return used;
}

void Automatonnetz_resume() {
  encoder[LEFT].setPos(automatonnetz_state.ui.selected_row * GRID_SIZE + automatonnetz_state.ui.selected_col);
  if (automatonnetz_state.ui.edit_cell) {
    const TransformCell &cell = automatonnetz_state.grid.at(automatonnetz_state.ui.selected_col, automatonnetz_state.ui.selected_row);
    encoder[RIGHT].setPos(cell.get_value(automatonnetz_state.ui.selected_cell_param));
  } else {
    encoder[RIGHT].setPos(automatonnetz_state.get_value(automatonnetz_state.ui.selected_param));
  }
  automatonnetz_state.reset();
}

void Automatonnetz_topButton() {
  automatonnetz_state.reset();
}

void Automatonnetz_lowerButton() {
  automatonnetz_state.clock(TRIGGER_MASK_GRID);
}

void Automatonnetz_rightButton() {
  if (automatonnetz_state.ui.edit_cell) {
    ++automatonnetz_state.ui.selected_cell_param;
    if (automatonnetz_state.ui.selected_cell_param >= CELL_SETTING_LAST)
      automatonnetz_state.ui.selected_cell_param = 0;
      const TransformCell &cell = automatonnetz_state.grid.at(automatonnetz_state.ui.selected_col, automatonnetz_state.ui.selected_row);
      encoder[RIGHT].setPos(cell.get_value(automatonnetz_state.ui.selected_cell_param));
  } else {
    ++automatonnetz_state.ui.selected_param;
    if (automatonnetz_state.ui.selected_param >= GRID_SETTING_LAST)
      automatonnetz_state.ui.selected_param = 0;
     encoder[RIGHT].setPos(automatonnetz_state.get_value(automatonnetz_state.ui.selected_param));
 }
}

void Automatonnetz_leftButton() {
  bool edit_cell = !automatonnetz_state.ui.edit_cell;
  if (edit_cell) {
    const TransformCell &cell = automatonnetz_state.grid.at(automatonnetz_state.ui.selected_col, automatonnetz_state.ui.selected_row);
    encoder[RIGHT].setPos(cell.get_value(automatonnetz_state.ui.selected_cell_param));
  } else {
    encoder[RIGHT].setPos(automatonnetz_state.get_value(automatonnetz_state.ui.selected_param));
  }

  automatonnetz_state.ui.edit_cell = edit_cell;
}

void Automatonnetz_leftButtonLong() {
  automatonnetz_state.clear_grid();
  if (automatonnetz_state.ui.edit_cell) {
    const TransformCell &cell = automatonnetz_state.grid.at(automatonnetz_state.ui.selected_col, automatonnetz_state.ui.selected_row);
    encoder[RIGHT].setPos(cell.get_value(automatonnetz_state.ui.selected_cell_param));
  }
}

bool Automatonnetz_encoders() {
  bool changed = false;

  int value = encoder[LEFT].pos();
  int selected = automatonnetz_state.ui.selected_row * 5 + automatonnetz_state.ui.selected_col;
  if (value != selected) {
    selected = value;
    while (selected < 0) selected += 25;
    while (selected > 24) selected -= 25;
    automatonnetz_state.ui.selected_row = selected / 5;
    automatonnetz_state.ui.selected_col = selected - automatonnetz_state.ui.selected_row * 5;
    encoder[LEFT].setPos(selected);
    MENU_REDRAW = 1;
    changed = true;
    if (automatonnetz_state.ui.edit_cell) {
      const TransformCell &cell = automatonnetz_state.grid.at(automatonnetz_state.ui.selected_col, automatonnetz_state.ui.selected_row);
      encoder[RIGHT].setPos(cell.get_value(automatonnetz_state.ui.selected_cell_param));
    } else {
      encoder[RIGHT].setPos(automatonnetz_state.get_value(automatonnetz_state.ui.selected_param));
    }
  }

  value = encoder[RIGHT].pos();
  if (automatonnetz_state.ui.edit_cell) {
    size_t selected_cell_param = automatonnetz_state.ui.selected_cell_param;
    TransformCell &cell = automatonnetz_state.grid.mutable_cell(automatonnetz_state.ui.selected_col, automatonnetz_state.ui.selected_row);
    changed = cell.apply_value(selected_cell_param, value);
    encoder[RIGHT].setPos(cell.get_value(selected_cell_param));
  } else {
    size_t selected_param = automatonnetz_state.ui.selected_param;
    changed = automatonnetz_state.apply_value(selected_param, value);
    encoder[RIGHT].setPos(automatonnetz_state.get_value(selected_param));
  }

  return changed;
}
