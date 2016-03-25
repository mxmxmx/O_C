#include "util/util_grid.h"
#include "util/util_ringbuffer.h"
#include "util/util_settings.h"
#include "util/util_sync.h"
#include "tonnetz/tonnetz_state.h"
#include "OC_bitmaps.h"
#include "OC_menus.h"

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
//
// NOTES
// The initial implementation was using a polling loop (since that's what the
// firmware paradigm was at the time) but has now been updated to be clocked by
// the core ISR. This means some additional hoops are necessary to be able to
// write settings/state from UI and ISR. The main use cases are:
// - Clearing the grid (left long press)
// - Cell event types that modify the cell itself
//
// So these have been wrapped in a minimal critical section/mutex style locking,
// which seems preferable than brute-forcey __disable_irq/__enable_irq, and the
// clear function shouldn't take long anyway. This doesn't cover all cases, but
// but editing while clocking could be classified as "undefined behaviour" ;)
//
// It's also possible for the displayed state to be slightly inconsistent; but
// these should be temporary glitches if they are even noticeable, so the risk
// seems acceptable there.
//
// TODO With fast clocking, trigger out mode might not provide a rising edge

#define FRACTIONAL_BITS 24
#define CLOCK_STEP_RES (0x1 << FRACTIONAL_BITS)
#define GRID_EPSILON 6
#define GRID_DIMENSION 5
#define GRID_CELLS (GRID_DIMENSION * GRID_DIMENSION)

// TODO Better selection, e.g. .125, .15 .2 or 3 digits with encoder acceleration

const size_t clock_fraction[] = {
  0, CLOCK_STEP_RES/8, 1+CLOCK_STEP_RES/7, CLOCK_STEP_RES/6, 1+CLOCK_STEP_RES/5, 1+CLOCK_STEP_RES/4, 1+CLOCK_STEP_RES/3, CLOCK_STEP_RES/2
};

const char *clock_fraction_names[] = {
  " \0\0\0\0", "  1/8", "  1/7", "  1/6", "  1/5", "  1/4", "  1/3", "  1/2"
};

static constexpr uint32_t TRIGGER_MASK_GRID = OC::DIGITAL_INPUT_1_MASK;
static constexpr uint32_t TRIGGER_MASK_ARP = OC::DIGITAL_INPUT_2_MASK;
static constexpr uint32_t kTriggerOutTicks = 1000U / OC_CORE_TIMER_RATE;

enum CellSettings {
  CELL_SETTING_TRANSFORM,
  CELL_SETTING_TRANSPOSE,
  CELL_SETTING_INVERSION,
  CELL_SETTING_EVENT,
  CELL_SETTING_LAST
};

enum CellEventMasks {
  CELL_EVENT_NONE,
  CELL_EVENT_RAND_TRANFORM = 0x1,
  CELL_EVENT_RAND_TRANSPOSE = 0x2,
  CELL_EVENT_RAND_INVERSION = 0x4,
  CELL_EVENT_ALL = 0x7
};

#define CELL_MAX_INVERSION 3
#define CELL_MIN_INVERSION -3

class TransformCell : public settings::SettingsBase<TransformCell, CELL_SETTING_LAST> {
public:
  TransformCell() { }

  tonnetz::ETransformType transform() const {
    return static_cast<tonnetz::ETransformType>(values_[CELL_SETTING_TRANSFORM]);
  }

  int transpose() const {
    return values_[CELL_SETTING_TRANSPOSE];
  }

  int inversion() const {
    return values_[CELL_SETTING_INVERSION];
  }

  CellEventMasks event_masks() const {
    return static_cast<CellEventMasks>(values_[CELL_SETTING_EVENT]);
  }

  void apply_event_masks() {
    int masks = values_[CELL_SETTING_EVENT];
    if (masks & CELL_EVENT_RAND_TRANFORM)
      apply_value(CELL_SETTING_TRANSFORM, random(tonnetz::TRANSFORM_LAST + 1));
    if (masks & CELL_EVENT_RAND_TRANSPOSE)
      apply_value(CELL_SETTING_TRANSPOSE, -12 + random(24 + 1));
    if (masks & CELL_EVENT_RAND_INVERSION)
      apply_value(CELL_SETTING_INVERSION, CELL_MIN_INVERSION + random(2*CELL_MAX_INVERSION + 1));
  }
};

const char *cell_event_masks[] = {
  "none",
  "rT__", // 0x1
  "r_O_", // 0x2
  "rTO_", // 0x1 + 0x2
  "r__I", // 0x4
  "rT_I", // 0x4 + 0x1
  "r_OI", // 0x4 + 0x2
  "rTOI", // 0x4 + 0x2 + 0x1
};

SETTINGS_DECLARE(TransformCell, CELL_SETTING_LAST) {
  {0, tonnetz::TRANSFORM_NONE, tonnetz::TRANSFORM_LAST, "Trfm", tonnetz::transform_names_str, settings::STORAGE_TYPE_U8},
  {0, -12, 12, "Offs", NULL, settings::STORAGE_TYPE_I8},
  {0, CELL_MIN_INVERSION, CELL_MAX_INVERSION, "Inv", NULL, settings::STORAGE_TYPE_I8},
  {0, CELL_EVENT_NONE, CELL_EVENT_ALL, "Muta", cell_event_masks, settings::STORAGE_TYPE_U8}
};

enum GridSettings {
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
enum ClearMode {
  CLEAR_MODE_ZERO, // empty cells
  CLEAR_MODE_RAND_TRANSFORM, // random transform
  CLEAR_MODE_RAND_TRANSFORM_EV, // random transform event
  CLEAR_MODE_LAST
};

enum UserAction {
  USER_ACTION_RESET,
  USER_ACTION_CLOCK,
};

class AutomatonnetzState : public settings::SettingsBase<AutomatonnetzState, GRID_SETTING_LAST> {
public:

  void Init() {
    InitDefaults();
    memset(cells_, 0, sizeof(cells_));
    grid.Init(cells_);
    memset(&ui, 0, sizeof(ui));
    ui.cell_cursor.Init(CELL_SETTING_TRANSFORM, CELL_SETTING_LAST - 1);
    ui.grid_cursor.Init(GRID_SETTING_DX, GRID_SETTING_LAST - 1);

    history_ = 0;
    cell_transpose_ = cell_inversion_ = 0;
    user_actions_.Init();
    critical_section_.Init();
  }

  void ClearGrid() {
    util::Lock<CRITICAL_SECTION_ID_MAIN> lock(critical_section_);
    ClearGrid(clear_mode());
  }

  void ClearGrid(ClearMode mode) {
    memset(cells_, 0, sizeof(cells_));
    switch (mode) {
      case CLEAR_MODE_RAND_TRANSFORM:
        for (auto &cell : cells_)
          cell.apply_value(CELL_SETTING_TRANSFORM, random(tonnetz::TRANSFORM_LAST + 1));
        break;
      case CLEAR_MODE_RAND_TRANSFORM_EV:
        for (auto &cell : cells_)
          cell.apply_value(CELL_SETTING_EVENT, CELL_EVENT_RAND_TRANFORM);
        break;
      case CLEAR_MODE_ZERO:
      default:
      break;
    }
  }

  // Settings wrappers

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

  ClearMode clear_mode() const {
    return static_cast<ClearMode>(values_[GRID_SETTING_CLEARMODE]);
  }

  // End of settings

  void ISR();
  void Reset();

  inline void AddUserAction(UserAction action) {
    user_actions_.Write(action);
  }

  // history length is fixed since it's kept as 4xuint8_t
  static const size_t HISTORY_LENGTH = 4;

  uint32_t history() const {
    return history_;
  }

  TransformCell cells_[GRID_CELLS];
  CellGrid<TransformCell, GRID_DIMENSION, FRACTIONAL_BITS, GRID_EPSILON> grid;
  TonnetzState tonnetz_state;

  struct {
    int selected_cell;
    bool edit_cell;

    menu::ScreenCursor<menu::kScreenLines> grid_cursor;
    menu::ScreenCursor<menu::kScreenLines> cell_cursor;
  } ui;

private:

  enum CriticalSectionIDs {
    CRITICAL_SECTION_ID_MAIN,
    CRITICAL_SECTION_ID_ISR
  };

  uint32_t trigger_out_ticks_;
  uint_fast8_t arp_index_;
  int cell_transpose_, cell_inversion_;
  uint32_t history_;

  util::RingBuffer<uint32_t, 4> user_actions_;
  util::CriticalSection critical_section_;

  void update_trigger_out();
  void update_outputs(bool chord_changed, int transpose, int inversion);
  void push_history(uint32_t pos) {
    history_ = (history_ << 8) | (pos & 0xff);
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

SETTINGS_DECLARE(AutomatonnetzState, GRID_SETTING_LAST) {
  {8, 0, 8*GRID_DIMENSION - 1, "dx", NULL, settings::STORAGE_TYPE_I8},
  {4, 0, 8*GRID_DIMENSION - 1, "dy", NULL, settings::STORAGE_TYPE_I8},
  {MODE_MAJOR, 0, MODE_LAST-1, "Mode", mode_names, settings::STORAGE_TYPE_U8},
  {0, -3, 3, "Oct", NULL, settings::STORAGE_TYPE_I8},
  {OUTPUTA_MODE_ROOT, OUTPUTA_MODE_ROOT, OUTPUTA_MODE_LAST - 1, "OutA", outputa_mode_names, settings::STORAGE_TYPE_U4},
  {CLEAR_MODE_ZERO, CLEAR_MODE_ZERO, CLEAR_MODE_LAST - 1, "Clr", clear_mode_names, settings::STORAGE_TYPE_U4},
};

AutomatonnetzState automatonnetz_state;

void Automatonnetz_init() {
  automatonnetz_state.Init();
  automatonnetz_state.ClearGrid(CLEAR_MODE_RAND_TRANSFORM);
  automatonnetz_state.Reset();
}

size_t Automatonnetz_storageSize() {
  return AutomatonnetzState::storageSize() +
    GRID_CELLS * TransformCell::storageSize();
}

void FASTRUN AutomatonnetzState::ISR() {
  update_trigger_out();

  uint32_t triggers = OC::DigitalInputs::clocked();

  bool reset = false;
  while (user_actions_.readable()) {
    switch (user_actions_.Read()) {
      case USER_ACTION_RESET:
        reset = true;
        break;
      case USER_ACTION_CLOCK:
        triggers |= TRIGGER_MASK_GRID;
        break;
    }
  }

  if ((triggers & TRIGGER_MASK_GRID) && OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_3>())
    reset = true;

  bool update = false;
  if (reset) {
    grid.MoveToOrigin();
    arp_index_ = 0;
    update = true;
  } else if (triggers & TRIGGER_MASK_GRID) {
    update = grid.move(dx(), dy());
  }

  bool chord_changed = false;
  {
    util::TryLock<CRITICAL_SECTION_ID_ISR> lock(critical_section_);
    // Minimal safeguard against update while ClearGrid is active.
    // We can't use a blocking Lock here since the ISR interrupts the main
    // loop, so the lock would never be relinquished at things implode.
    // A user-triggered clear will force a reset anyway and update next ISR,
    // so we can just skip it.

    if (update && lock.locked()) {
      TransformCell &current_cell = grid.mutable_current_cell();
      push_history(grid.current_pos_index());
      tonnetz::ETransformType transform = current_cell.transform();
      if (reset || transform >= tonnetz::TRANSFORM_LAST) {
        tonnetz_state.reset(mode());
        chord_changed = true;
      } else if (transform != tonnetz::TRANSFORM_NONE) {
        tonnetz_state.apply_transformation(transform);
        chord_changed = true;
      }

      cell_transpose_ = current_cell.transpose();
      cell_inversion_ = current_cell.inversion();
      current_cell.apply_event_masks();
    }
  }

  // Arp/strum
  if (chord_changed && OUTPUTA_MODE_STRUM == output_mode()) {
    arp_index_ = 0;
  } else if ((triggers & TRIGGER_MASK_ARP) &&
             !reset &&
             !OC::DigitalInputs::read_immediate<OC::DIGITAL_INPUT_4>()) {
    ++arp_index_;
    if (arp_index_ >= 3)
      arp_index_ = 0;
  }

  update_outputs(chord_changed, cell_transpose_, cell_inversion_);
}

void AutomatonnetzState::Reset() {
  // Assumed to be called w/o ISR active!
  grid.MoveToOrigin();
  push_history(grid.current_pos_index());
  tonnetz_state.reset(mode());
  arp_index_ = 0;
  const TransformCell &current_cell = grid.current_cell();
  cell_transpose_ = current_cell.transpose();
  cell_inversion_ = current_cell.inversion();
  update_outputs(true, cell_transpose_, cell_inversion_);
}

// TODO Essentially shared with H1200 
#define AT_OUTPUT_NOTE(i,dac) \
do { \
  int32_t note = tonnetz_state.outputs(i) << 7; \
  note += 3 * 12 << 7; \
  DAC::set<dac>(DAC::pitch_to_dac(note, octave())); \
} while (0)

void AutomatonnetzState::update_outputs(bool chord_changed, int transpose, int inversion) {

  int32_t root = (OC::ADC::pitch_value(ADC_CHANNEL_1) >> 7) + transpose;

  inversion += (OC::ADC::value<ADC_CHANNEL_4>() >> 9);
  CONSTRAIN(inversion, CELL_MIN_INVERSION * 2, CELL_MAX_INVERSION * 2);

  tonnetz_state.render(root, inversion);

  switch (output_mode()) {
    case OUTPUTA_MODE_ROOT:
      AT_OUTPUT_NOTE(0,DAC_CHANNEL_A);
      break;
    case OUTPUTA_MODE_TRIG:
      if (chord_changed) {
        trigger_out_ticks_ = kTriggerOutTicks;
        DAC::set<DAC_CHANNEL_A>(OC::calibration_data.dac.octaves[_ZERO + 5]);
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
  if (trigger_out_ticks_) {
    uint32_t ticks = trigger_out_ticks_;
    --ticks;
    if (!ticks)
      DAC::set<DAC_CHANNEL_A>(OC::calibration_data.dac.octaves[_ZERO]);
    trigger_out_ticks_ = ticks;
  }
}

void Automatonnetz_loop() {
}

void FASTRUN Automatonnetz_isr() {
  // All user actions, etc. handled in ::Update
  automatonnetz_state.ISR();
}

static const weegfx::coord_t kGridXStart = 0;
static const weegfx::coord_t kGridYStart = 2;
static const weegfx::coord_t kGridH = 12;
static const weegfx::coord_t kGridW = 12;
static const weegfx::coord_t kMenuStartX = 62;
static const weegfx::coord_t kLineHeight = 11;

namespace automatonnetz {

void draw_cell_menu() {

  menu::TitleBar<kMenuStartX, 1, 2>::Draw();
  graphics.print("CELL ");
  graphics.print(automatonnetz_state.ui.selected_cell / 5 + 1);
  graphics.print(',');
  graphics.print(automatonnetz_state.ui.selected_cell % 5 + 1);

  const TransformCell &cell = automatonnetz_state.grid.at(automatonnetz_state.ui.selected_cell);

  menu::SettingsList<menu::kScreenLines, kMenuStartX, menu::kDefaultMenuEndX - 25> settings_list(automatonnetz_state.ui.cell_cursor);
  menu::SettingsListItem list_item;
  while (settings_list.available()) {
    const int current = settings_list.Next(list_item);
    list_item.DrawDefault(cell.get_value(current), TransformCell::value_attr(current));
  }
}

void draw_grid_menu() {
  EMode mode = automatonnetz_state.tonnetz_state.current_chord().mode();
  int outputs[4];
  automatonnetz_state.tonnetz_state.get_outputs(outputs);

  menu::TitleBar<kMenuStartX, 1, 2>::Draw();
  for (size_t i=1; i < 4; ++i) {
    graphics.print(note_name(outputs[i]));
    graphics.movePrintPos(weegfx::Graphics::kFixedFontW/2, 0);
  }
  if (MODE_MAJOR == mode)
    graphics.print('+');
  else
    graphics.print('-');

  menu::SettingsList<menu::kScreenLines, kMenuStartX, menu::kDefaultMenuEndX - 25> settings_list(automatonnetz_state.ui.grid_cursor);
  menu::SettingsListItem list_item;
  while (settings_list.available()) {
    const int current = settings_list.Next(list_item);
    const int value = automatonnetz_state.get_value(current);
    const settings::value_attr &attr = AutomatonnetzState::value_attr(current);

    if (current <= GRID_SETTING_DY) {
      const int integral = value / 8;
      const int fraction = value % 8;
      char value_str[6];
      memcpy(value_str, clock_fraction_names[fraction], 6);
      if (integral || !fraction)
        value_str[0] = (char)('0' + integral);

      list_item.valuex = list_item.endx - 30;
      list_item.DrawDefault(value_str, value, attr);
    } else {
      list_item.DrawDefault(value, attr);
    }
  }
}

}; // namespace automatonnetz

void Automatonnetz_menu() {
  uint16_t row = 0, col = 0;
  for (int i = 0; i < GRID_CELLS; ++i) {

    const TransformCell &cell = automatonnetz_state.grid.at(i);
    weegfx::coord_t x = kGridXStart + col * kGridW;
    weegfx::coord_t y = kGridYStart + row * kGridH;

    graphics.setPrintPos(x + 3, y + 3);
    graphics.print(tonnetz::transform_names[cell.transform()]);

    if (i == automatonnetz_state.ui.selected_cell)
      graphics.drawFrame(x, y, kGridW, kGridH);

    if (col < GRID_DIMENSION - 1) {
      ++col;
    } else {
      ++row;
      col = 0;
    }
  }

  const vec2<size_t> current_pos = automatonnetz_state.grid.current_pos();
  graphics.invertRect(kGridXStart + current_pos.x * kGridW + 1, 
                      kGridYStart + current_pos.y * kGridH + 1,
                      kGridW - 2, kGridH - 2);

  if (automatonnetz_state.ui.edit_cell)
    automatonnetz::draw_cell_menu();
  else
    automatonnetz::draw_grid_menu();
}

static const weegfx::coord_t kScreenSaverGridX = kGridXStart + kGridW / 2;
static const weegfx::coord_t kScreenSaverGridY = kGridYStart + kGridH / 2;
static const weegfx::coord_t kScreenSaverGridW = kGridW;
static const weegfx::coord_t kScreenSaverGridH = kGridH;

static constexpr weegfx::coord_t kNoteCircleX = 96;
static constexpr weegfx::coord_t kNoteCircleY = 32;

inline vec2<size_t> extract_pos(uint32_t history) {
  return vec2<size_t>((history & 0xff) / GRID_DIMENSION, (history & 0xff) % GRID_DIMENSION);
}

void Automatonnetz_screensaver() {
  int outputs[4];
  automatonnetz_state.tonnetz_state.get_outputs(outputs);
  uint32_t cell_history = automatonnetz_state.history();

  uint8_t normalized[3];
  for (size_t i=0; i < 3; ++i)
    normalized[i] = (outputs[i + 1] + 120) % 12;
  OC::visualize_pitch_classes(normalized, kNoteCircleX, kNoteCircleY);

  vec2<size_t> last_pos = extract_pos(cell_history);
  cell_history >>= 8;
  graphics.drawBitmap8(kScreenSaverGridX + last_pos.x * kScreenSaverGridW - 3,
                       kScreenSaverGridY + last_pos.y * kScreenSaverGridH - 3,
                       8, OC::circle_disk_bitmap_8x8);
  for (size_t i = 1; i < AutomatonnetzState::HISTORY_LENGTH; ++i, cell_history >>= 8) {
    const vec2<size_t> current = extract_pos(cell_history);
    graphics.drawLine(kScreenSaverGridX + last_pos.x * kScreenSaverGridW, kScreenSaverGridY + last_pos.y * kScreenSaverGridH,
                      kScreenSaverGridX + current.x * kScreenSaverGridW, kScreenSaverGridY + current.y * kScreenSaverGridH);

    graphics.drawBitmap8(kScreenSaverGridX + current.x * kScreenSaverGridW - 3,
                         kScreenSaverGridY + current.y * kScreenSaverGridH - 3,
                         8, OC::circle_bitmap_8x8);
    last_pos = current;
  }
}

size_t Automatonnetz_save(void *dest) {
  char *storage = static_cast<char *>(dest);
  size_t used = automatonnetz_state.Save(storage);
  for (size_t cell = 0; cell < GRID_CELLS; ++cell)
    used += automatonnetz_state.cells_[cell].Save(storage + used);

  return used;
}

size_t Automatonnetz_restore(const void *dest) {
  const char *storage = static_cast<const char *>(dest);
  size_t used = automatonnetz_state.Restore(storage);
  for (size_t cell = 0; cell < GRID_CELLS; ++cell)
    used += automatonnetz_state.cells_[cell].Restore(storage + used);

  return used;
}

void Automatonnetz_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      OC::ui.encoder_enable_acceleration(OC::CONTROL_ENCODER_L, false);
      automatonnetz_state.AddUserAction(USER_ACTION_RESET);
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
      break;
  }   
}

void Automatonnetz_rightButton() {
}

void Automatonnetz_handleButtonEvent(const UI::Event &event) {
  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        automatonnetz_state.AddUserAction(USER_ACTION_RESET);
        break;
      case OC::CONTROL_BUTTON_DOWN:
        automatonnetz_state.AddUserAction(USER_ACTION_CLOCK);
        break;
      case OC::CONTROL_BUTTON_L:
        automatonnetz_state.ui.edit_cell = !automatonnetz_state.ui.edit_cell;       
        break;
      case OC::CONTROL_BUTTON_R:
        if (automatonnetz_state.ui.edit_cell)
          automatonnetz_state.ui.cell_cursor.toggle_editing();
        else
          automatonnetz_state.ui.grid_cursor.toggle_editing();
        break;
    }
  } else {
    if (OC::CONTROL_BUTTON_L == event.control) {
      automatonnetz_state.ClearGrid();
      // Forcing reset might make critical section even less necesary...
      automatonnetz_state.AddUserAction(USER_ACTION_RESET);
    }
  }
}

void Automatonnetz_handleEncoderEvent(const UI::Event &event) {

  if (OC::CONTROL_ENCODER_L == event.control) {
    int selected = automatonnetz_state.ui.selected_cell + event.value;
    CONSTRAIN(selected, 0, GRID_CELLS - 1);
    automatonnetz_state.ui.selected_cell = selected;
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    if (automatonnetz_state.ui.edit_cell) {
      if (automatonnetz_state.ui.cell_cursor.editing()) {
        TransformCell &cell = automatonnetz_state.grid.mutable_cell(automatonnetz_state.ui.selected_cell);
        cell.change_value(automatonnetz_state.ui.cell_cursor.cursor_pos(), event.value);
      } else {
        automatonnetz_state.ui.cell_cursor.Scroll(event.value);
      }
    } else {
      if (automatonnetz_state.ui.grid_cursor.editing()) {
        automatonnetz_state.change_value(automatonnetz_state.ui.grid_cursor.cursor_pos(), event.value);
      } else {
        automatonnetz_state.ui.grid_cursor.Scroll(event.value);
      }
    }
  }
}
