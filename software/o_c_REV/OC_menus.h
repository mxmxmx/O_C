// Copyright (c) 2016 Patrick Dowling
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// 
// Menu drawing helpers

#ifndef OC_MENUS_H
#define OC_MENUS_H

#include "src/drivers/display.h"
#include "OC_bitmaps.h"
#include "util/util_macros.h"
#include "util/util_misc.h"
#include "util/util_settings.h"
#include "OC_chords.h"
#include "OC_DAC.h"

namespace OC {

void visualize_pitch_classes(uint8_t *normalized, weegfx::coord_t centerx, weegfx::coord_t centery);

void screensaver();
void scope_render();
void vectorscope_render();

namespace menu {

void Init();

static constexpr weegfx::coord_t kDisplayWidth = weegfx::Graphics::kWidth;
static constexpr weegfx::coord_t kDisplayHeight = weegfx::Graphics::kHeight;
static constexpr weegfx::coord_t kMenuLineH = 12;
static constexpr weegfx::coord_t kFontHeight = 8;
static constexpr weegfx::coord_t kDefaultMenuStartX = 0;
static constexpr weegfx::coord_t kDefaultValueX = 96;
static constexpr weegfx::coord_t kDefaultMenuEndX = kDisplayWidth - 2;
static constexpr weegfx::coord_t kIndentDx = 2;
static constexpr weegfx::coord_t kTextDy = 2;

static constexpr int kScreenLines = 4;

static inline weegfx::coord_t CalcLineY(int line) {
  return (line + 1) * kMenuLineH + 2 + 1;
}

// Helper to manage cursor position in settings-type menus.
// The "fancy" mode tries to indicate that there may be more items by not
// moving to line 0 until the first item is selected (and vice versa for end
// of list).
// Currently assumes there are at least 4 items!
template <int screen_lines, bool fancy = true>
class ScreenCursor {
public:
  ScreenCursor() { }
  ~ScreenCursor() { }

  void Init(int start, int end) {
    editing_ = false;
    start_ = start;
    end_ = end;
    cursor_pos_ = start;
    screen_line_ = 0;
  }

  void AdjustEnd(int end) {
    // WARN This has a specific use case where we don't have to adjust screen line!
    end_ = end;
  }

  void Scroll(int amount) {
    int pos = cursor_pos_ + amount;
    CONSTRAIN(pos, start_, end_);
    cursor_pos_ = pos;

    int screen_line = screen_line_ + amount;
    if (fancy) {
      if (amount < 0) {
        if (screen_line < 2) {
          if (pos >= start_ + 1)
            screen_line = 1;
          else
            screen_line = 0;
        }
      } else {
        if (screen_line >= screen_lines - 2) {
          if (pos <= end_ - 1)
            screen_line = screen_lines - 2;
          else
            screen_line = screen_lines - 1;
        }
      }
    } else {
      CONSTRAIN(screen_line, 0, screen_lines - 1);
    }
    screen_line_ = screen_line;
  }

  inline int cursor_pos() const {
    return cursor_pos_;
  }

  inline int first_visible() const {
    return cursor_pos_ - screen_line_;
  }

  inline int last_visible() const {
    return cursor_pos_ - screen_line_ + screen_lines - 1;
  }

  inline bool editing() const {
    return editing_;
  }

  inline void toggle_editing() {
    editing_ = !editing_;
  }

  inline void set_editing(bool enable) {
    editing_ = enable;
  }

private:
  bool editing_;

  int start_, end_;
  int cursor_pos_;
  int screen_line_;
};

inline void DrawEditIcon(weegfx::coord_t x, weegfx::coord_t y, int value, int min_value, int max_value) {
  const uint8_t *src = OC::bitmap_edit_indicators_8;
  if (value == max_value)
    src += OC::kBitmapEditIndicatorW * 2;
  else if (value == min_value)
    src += OC::kBitmapEditIndicatorW;

  graphics.drawBitmap8(x - 5, y + 1, OC::kBitmapEditIndicatorW, src);
}

inline void DrawEditIcon(weegfx::coord_t x, weegfx::coord_t y, int value, const settings::value_attr &attr) {
  const uint8_t *src = OC::bitmap_edit_indicators_8;
  if (value == attr.max_)
    src += OC::kBitmapEditIndicatorW * 2;
  else if (value == attr.min_)
    src += OC::kBitmapEditIndicatorW;

  graphics.drawBitmap8(x - 5, y + 1, OC::kBitmapEditIndicatorW, src);
}

inline void DrawChord(weegfx::coord_t x, weegfx::coord_t y, int width, int value, int mem_offset) {
  
   OC::Chord *active_chord = &OC::user_chords[value + mem_offset * OC::Chords::NUM_CHORDS];
   int8_t _quality = active_chord->quality;
   int8_t _voicing = active_chord->voicing;
   int y_pos;

   // draw #A / base note:
   y -= active_chord->base_note * 2;
   graphics.drawRect(x, y, width, width);
   // draw #B:
   y -= OC::qualities[_quality][1] * 3;
   y_pos = y + OC::voicing[_voicing][1] * 16;
   CONSTRAIN(y_pos, 8, 64);
   graphics.drawFrame(x, y_pos, width, width);
   // draw #C:
   y -= OC::qualities[_quality][2] * 3;
   y_pos = y + OC::voicing[_voicing][2] * 16;
   CONSTRAIN(y_pos, 8, 64);
   graphics.drawFrame(x, y_pos, width, width);
   // draw #D:
   y -= OC::qualities[_quality][3] * 3;
   y_pos = y + OC::voicing[_voicing][3] * 16;
   CONSTRAIN(y_pos, 8, 64);
   graphics.drawFrame(x, y_pos, width, width);
}

inline void DrawMiniChord(weegfx::coord_t x, weegfx::coord_t y, uint8_t count, uint8_t indicator) {
  int8_t _x = x - count * 3 - 4;
  int8_t _y = y + 4;
  int8_t _w = 2;
  for (int i = 0; i < count + 1; i++, _x += 3) {
    graphics.drawRect(_x, _y, _w, _w);
    if (i == indicator)
      graphics.drawRect(_x, _y + 3, _w, _w);
  }
}

template <bool rtl, size_t max_bits, weegfx::coord_t height, weegfx::coord_t padding>
void DrawMask(weegfx::coord_t x, weegfx::coord_t y, uint32_t mask, size_t count) {
  weegfx::coord_t dx;
  if (count > max_bits) count = max_bits;
  if (rtl) {
    x -= 3;
    dx = -(2 + padding);
  } else {
    x -= count * 3;
    dx = (2 + padding );
  }

  for (size_t i = 0; i < count; ++i, mask >>= 1, x += dx) {
    if (mask & 0x1)
      graphics.drawRect(x, y + 1, 2, height);
    else
      graphics.drawRect(x, y + height, 2, 1);
  }
}

template <bool rtl, size_t max_bits, weegfx::coord_t height, weegfx::coord_t padding>
void DrawMask(weegfx::coord_t x, weegfx::coord_t y, uint32_t mask, size_t count, uint8_t clock_indicator) {
  weegfx::coord_t dx;
  if (count > max_bits) count = max_bits;
  if (rtl) {
    x -= 3;
    dx = -(2 + padding);
  } else {
    x -= count * 3;
    dx = (2 + padding );
  }

  for (size_t i = 0; i < count; ++i, mask >>= 1, x += dx) {
    if (mask & 0x1)
      graphics.drawRect(x, y + 1, 2, height);
    else
      graphics.drawRect(x, y + height, 2, 1);
      
    if (clock_indicator == i)  
      graphics.drawRect(x, y + height + 2, 2, 2);
  }
}

inline static void DrawGateIndicator(weegfx::coord_t x, weegfx::coord_t y, uint8_t state) {
  state = (state + 3) >> 2;
  if (state)
    graphics.drawBitmap8(x, y, 4, OC::bitmap_gate_indicators_8 + (state << 2));
}

// Templated title bar that can have multiple columns
template <weegfx::coord_t start_x, int columns, weegfx::coord_t text_dx>
class TitleBar {
public:
  static constexpr weegfx::coord_t kColumnWidth = kDisplayWidth / columns;
  static constexpr weegfx::coord_t kTextX = text_dx;
  static constexpr weegfx::coord_t kTextY = 2;

  inline static void SetColumn(int column) {
    graphics.setPrintPos(start_x + kColumnWidth * column + kTextX, kTextY);
  }

  inline static void Draw() {
    
    if (OC::DAC::get_voltage_scaling(DAC_CHANNEL_A) || OC::DAC::get_voltage_scaling(DAC_CHANNEL_B) || OC::DAC::get_voltage_scaling(DAC_CHANNEL_C) || OC::DAC::get_voltage_scaling(DAC_CHANNEL_D))
      graphics.drawHLineDots(start_x, kMenuLineH, kDisplayWidth - start_x);
    else
      graphics.drawHLine(start_x, kMenuLineH, kDisplayWidth - start_x);
    SetColumn(0);
  }

  inline static void Selected(int column) {
    graphics.invertRect(start_x + kColumnWidth * column, 0, kColumnWidth, kMenuLineH - 1);
  }

  inline static void DrawGateIndicator(int column, uint8_t state) {
    menu::DrawGateIndicator(start_x + kColumnWidth * column + 1, 2, state);
  }

  inline static weegfx::coord_t ColumnStartX(int column) {
    return start_x + kColumnWidth * column;
  }
};

// Common, default types
typedef TitleBar<kDefaultMenuStartX, 1, 2> DefaultTitleBar;
typedef TitleBar<kDefaultMenuStartX, 2, 2> DualTitleBar;
typedef TitleBar<kDefaultMenuStartX, 4, 6> QuadTitleBar;

// Essentially all O&C apps are built around a list of settings; these two
// wrappers and the cursor wrapper replace the original macro-based drawing.
// start_x : Left edge of list (setting name column)
// value_x : Left edge of value column (edit cursor placed left of this)
// The value column is assumed to end at kDisplayWidth
//
struct SettingsListItem {
  bool selected, editing;
  weegfx::coord_t x, y;
  weegfx::coord_t valuex, endx;

  SettingsListItem() { }
  ~SettingsListItem() { }

  inline void DrawName(const settings::value_attr &attr) const {
    graphics.setPrintPos(x + kIndentDx, y + kTextDy);
    graphics.print(attr.name);
  }

  inline void DrawCharName(const char* name_string) const {
    graphics.setPrintPos(x + kIndentDx, y + kTextDy);
    graphics.print(name_string);
  }

  inline void DrawDefault(int value, const settings::value_attr &attr) const {
    DrawName(attr);

    graphics.setPrintPos(endx, y + kTextDy);
    if(attr.value_names)
      graphics.print_right(attr.value_names[value]);
    else
      graphics.pretty_print_right(value);

    if (editing)
      menu::DrawEditIcon(valuex, y, value, attr);
    if (selected)
      graphics.invertRect(x, y, kDisplayWidth - x, kMenuLineH - 1);
  }

  inline void Draw_PW_Value_Char(int value, const settings::value_attr &attr, const char* name_string) const {
    DrawCharName(name_string);

    graphics.setPrintPos(endx, y + kTextDy);
    
    if(attr.value_names)
      graphics.print_right(attr.value_names[value]);
    else {
      if (value == 0x0) // echo
        graphics.print_right("echo");
      else if (value == 0xFF) // 50%
        graphics.print_right("50%");
      else 
        graphics.pretty_print_right(value);
    }

    if (editing)
      menu::DrawEditIcon(valuex, y, value, attr);
    if (selected)
      graphics.invertRect(x, y, kDisplayWidth - x, kMenuLineH - 1);
  }
  
  inline void Draw_PW_Value(int value, const settings::value_attr &attr) const {
    DrawName(attr);

    graphics.setPrintPos(endx, y + kTextDy);
    
    if(attr.value_names)
      graphics.print_right(attr.value_names[value]);
    else {
      if (value == 0x0) // echo
        graphics.print_right("echo");
      else if (value == 0xFF) // 50%
        graphics.print_right("50%");
      else 
        graphics.pretty_print_right(value);
    }

    if (editing)
      menu::DrawEditIcon(valuex, y, value, attr);
    if (selected)
      graphics.invertRect(x, y, kDisplayWidth - x, kMenuLineH - 1);
  }

  inline void DrawValueMax(int value, const settings::value_attr &attr, int16_t _max) const {
    DrawName(attr);

    graphics.setPrintPos(endx, y + kTextDy);
    if(attr.value_names)
      graphics.print_right(attr.value_names[value]);
    else
      graphics.pretty_print_right(value);

    if (editing)
      menu::DrawEditIcon(valuex, y, value, attr.min_, _max);
    if (selected)
      graphics.invertRect(x, y, kDisplayWidth - x, kMenuLineH - 1);
  }

  inline void DrawDefault(const char *str, int value, const settings::value_attr &attr) const {
    DrawName(attr);

    graphics.setPrintPos(endx, y + kTextDy);
    graphics.print_right(str);

    if (editing)
      menu::DrawEditIcon(valuex, y, value, attr);
    if (selected)
      graphics.invertRect(x, y, kDisplayWidth - x, kMenuLineH - 1);
  }

  template <bool editable>
  inline void DrawNoValue(int value, const settings::value_attr &attr) const {
    DrawName(attr);

    if (editable && editing)
      menu::DrawEditIcon(valuex, y, value, attr);
    if (selected)
      graphics.invertRect(x, y, kDisplayWidth - x, kMenuLineH - 1);
  }

  inline void DrawCustom() const {
    if (selected)
      graphics.invertRect(x, y, kDisplayWidth - x, kMenuLineH - 1);
  }

  inline void SetPrintPos() const {
    graphics.setPrintPos(x + kIndentDx, y + kTextDy);
  }

  DISALLOW_COPY_AND_ASSIGN(SettingsListItem);
};

template <int screen_lines, weegfx::coord_t start_x, weegfx::coord_t value_x, weegfx::coord_t end_x = kDefaultMenuEndX>
class SettingsList {
public:

  SettingsList(const ScreenCursor<screen_lines> &cursor)
  : cursor_(cursor)
  , current_item_(cursor.first_visible())
  , last_item_(cursor.last_visible())
  , y_(CalcLineY(kScreenLines - screen_lines))
  { }

  bool available() const {
    return current_item_ <= last_item_;
  }

  int Next(SettingsListItem &item) {
    item.selected = current_item_ == cursor_.cursor_pos();
    item.editing = item.selected && cursor_.editing();
    item.x = start_x;
    item.y = y_;
    item.valuex = value_x;
    item.endx = end_x;
    y_ += kMenuLineH;
    return current_item_++;
  }

  static void AbsoluteLine(int line, SettingsListItem &item) {
    item.x = start_x;
    item.y = CalcLineY(line);
    item.valuex = value_x;
    item.endx = end_x;
  }

private:
  const ScreenCursor<screen_lines> cursor_;
  int current_item_, last_item_;
  weegfx::coord_t y_;

  DISALLOW_COPY_AND_ASSIGN(SettingsList);
};

}; // namespace menu

}; // namespace OC

#endif // OC_MENUS_H
