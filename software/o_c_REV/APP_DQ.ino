// Copyright (c) 2015, 2016 Patrick Dowling, Tim Churches, Max Stadler
//
// Initial app implementation: Patrick Dowling (pld@gurkenkiste.com)
// Modifications by: Tim Churches (tim.churches@gmail.com)
// Yet more Modifications by: mxmxmx 
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
// Quad quantizer app, based around the the quantizer/scales implementation from
// from Braids by Olivier Gillet (see braids_quantizer.h/cc et al.). It has since
// grown a little bit...

#include "OC_apps.h"
#include "util/util_settings.h"
#include "util/util_trigger_delay.h"
#include "braids_quantizer.h"
#include "braids_quantizer_scales.h"
#include "OC_menus.h"
#include "OC_scales.h"
#include "OC_scale_edit.h"
#include "OC_strings.h"

const uint8_t NUMCHANNELS = 2;

enum DQ_ChannelSetting {
  DQ_CHANNEL_SETTING_SCALE,
  DQ_CHANNEL_SETTING_ROOT,
  DQ_CHANNEL_SETTING_SCALE_SEQ,
  DQ_CHANNEL_SETTING_MASK1,
  DQ_CHANNEL_SETTING_MASK2,
  DQ_CHANNEL_SETTING_MASK3,
  DQ_CHANNEL_SETTING_MASK4,
  DQ_CHANNEL_SETTING_SEQ_MODE,
  DQ_CHANNEL_SETTING_SOURCE,
  DQ_CHANNEL_SETTING_TRIGGER,
  DQ_CHANNEL_SETTING_DELAY,
  DQ_CHANNEL_SETTING_TRANSPOSE,
  DQ_CHANNEL_SETTING_OCTAVE,
  DQ_CHANNEL_SETTING_FINE,
  DQ_CHANNEL_SETTING_AUX_OUTPUT,
  DQ_CHANNEL_SETTING_PULSEWIDTH,
  DQ_CHANNEL_SETTING_AUX_OCTAVE,
  DQ_CHANNEL_SETTING_LAST
};

enum DQ_ChannelTriggerSource {
  DQ_CHANNEL_TRIGGER_TR1,
  DQ_CHANNEL_TRIGGER_TR2,
  DQ_CHANNEL_TRIGGER_CONTINUOUS,
  DQ_CHANNEL_TRIGGER_LAST
};

enum DQ_ChannelSource {
  DQ_CHANNEL_SOURCE_CV1,
  DQ_CHANNEL_SOURCE_CV2,
  DQ_CHANNEL_SOURCE_LAST
};

enum DQ_AUX_MODE {
  DQ_GATE,
  DQ_COPY,
  DQ_ASR,
  DQ_AUX_MODE_LAST
};

class DQ_QuantizerChannel : public settings::SettingsBase<DQ_QuantizerChannel, DQ_CHANNEL_SETTING_LAST> {
public:

  int get_scale() const {
    return values_[DQ_CHANNEL_SETTING_SCALE];
  }

  void set_scale(int scale) {
    if (scale != get_scale()) {
      const OC::Scale &scale_def = OC::Scales::GetScale(scale);
      uint16_t mask = get_mask();
      if (0 == (mask & ~(0xffff << scale_def.num_notes)))
        mask |= 0x1;
      apply_value(DQ_CHANNEL_SETTING_MASK1, mask); // to do
      apply_value(DQ_CHANNEL_SETTING_SCALE, scale);
    }
  }

  int get_root() const {
    return values_[DQ_CHANNEL_SETTING_ROOT];
  }

  uint16_t get_mask() const { // to do
    return values_[DQ_CHANNEL_SETTING_MASK1];
  }

  DQ_ChannelSource get_source() const {
    return static_cast<DQ_ChannelSource>(values_[DQ_CHANNEL_SETTING_SOURCE]);
  }

  DQ_ChannelTriggerSource get_trigger_source() const {
    return static_cast<DQ_ChannelTriggerSource>(values_[DQ_CHANNEL_SETTING_TRIGGER]);
  }

  uint16_t get_trigger_delay() const {
    return values_[DQ_CHANNEL_SETTING_DELAY];
  }

  int get_transpose() const {
    return values_[DQ_CHANNEL_SETTING_TRANSPOSE];
  }

  int get_octave() const {
    return values_[DQ_CHANNEL_SETTING_OCTAVE];
  }

  int get_fine() const {
    return values_[DQ_CHANNEL_SETTING_FINE];
  }

  int get_aux_mode() const {
    return values_[DQ_CHANNEL_SETTING_AUX_OUTPUT];
  }

  void Init(DQ_ChannelSource source, DQ_ChannelTriggerSource trigger_source) {
    InitDefaults();
    apply_value(DQ_CHANNEL_SETTING_SOURCE, source);
    apply_value(DQ_CHANNEL_SETTING_TRIGGER, trigger_source);

    force_update_ = true;
    instant_update_ = false;
    last_scale_ = -1;
    last_mask_ = 0;
    last_sample_ = 0;
    clock_ = 0;

    trigger_delay_.Init();
    quantizer_.Init();
    update_scale(true);
    trigger_display_.Init();
    update_enabled_settings();

    scrolling_history_.Init(OC::DAC::kOctaveZero * 12 << 7);
  }

  void force_update() {
    force_update_ = true;
  }

  void instant_update() {
    instant_update_ = (~instant_update_) & 1u;
  }

  inline void Update(uint32_t triggers, size_t index, DAC_CHANNEL dac_channel) {
    bool forced_update = force_update_;
    force_update_ = false;

    DQ_ChannelSource source = get_source();
    DQ_ChannelTriggerSource trigger_source = get_trigger_source();
    bool continous = DQ_CHANNEL_TRIGGER_CONTINUOUS == trigger_source;
    bool triggered = !continous &&
      (triggers & DIGITAL_INPUT_MASK(trigger_source - DQ_CHANNEL_TRIGGER_TR1));

    trigger_delay_.Update();
    if (triggered)
      trigger_delay_.Push(OC::trigger_delay_ticks[get_trigger_delay()]);
    triggered = trigger_delay_.triggered();

    if (triggered)
      ++clock_;

    bool update = continous || triggered;
    if (update_scale(forced_update) && instant_update_ == true)
       update = true;
       
    int32_t sample = last_sample_;
    int32_t history_sample = 0;

    switch (source) {
      
      default: {
          if (update) {
            int32_t transpose = get_transpose();
            int32_t pitch = quantizer_.enabled()
                ? OC::ADC::raw_pitch_value(static_cast<ADC_CHANNEL>(source))
                : OC::ADC::pitch_value(static_cast<ADC_CHANNEL>(source));
            if (index != source) {
              transpose += (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) * 12 + 2047) >> 12;
            }
            CONSTRAIN(transpose, -12, 12); 
            const int32_t quantized = quantizer_.Process(pitch, get_root() << 7, transpose);
            sample = OC::DAC::pitch_to_dac(dac_channel, quantized, get_octave());
            history_sample = quantized + ((OC::DAC::kOctaveZero + get_octave()) * 12 << 7);
          }
        }
    } // end switch  

    bool changed = last_sample_ != sample;
    if (changed) {
      MENU_REDRAW = 1;
      last_sample_ = sample;
    }
    OC::DAC::set(dac_channel, sample + get_fine());

    if (triggered || (continous && changed)) {
      scrolling_history_.Push(history_sample);
      trigger_display_.Update(1, true);
    } else {
      trigger_display_.Update(1, false);
    }
    scrolling_history_.Update();
  }

  // Wrappers for ScaleEdit
  void scale_changed() {
    force_update_ = true;
  }

  uint16_t get_scale_mask() const {
    return get_mask();
  }

  void update_scale_mask(uint16_t mask) {
    apply_value(DQ_CHANNEL_SETTING_MASK1, mask); // to do .. Should automatically be updated
  }
  //

  uint8_t getTriggerState() const {
    return trigger_display_.getState();
  }

  // Maintain an internal list of currently available settings, since some are
  // dependent on others. It's kind of brute force, but eh, works :) If other
  // apps have a similar need, it can be moved to a common wrapper

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  DQ_ChannelSetting enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  void update_enabled_settings() {
    DQ_ChannelSetting *settings = enabled_settings_;
    *settings++ = DQ_CHANNEL_SETTING_SCALE;
    if (OC::Scales::SCALE_NONE != get_scale()) {
      *settings++ = DQ_CHANNEL_SETTING_ROOT;
      *settings++ = DQ_CHANNEL_SETTING_SCALE_SEQ;
      *settings++ = DQ_CHANNEL_SETTING_MASK1; // to do
      *settings++ = DQ_CHANNEL_SETTING_SEQ_MODE;
    }
    *settings++ = DQ_CHANNEL_SETTING_SOURCE;
    *settings++ = DQ_CHANNEL_SETTING_TRIGGER;
    if (DQ_CHANNEL_TRIGGER_CONTINUOUS != get_trigger_source()) 
      *settings++ = DQ_CHANNEL_SETTING_DELAY;

    *settings++ = DQ_CHANNEL_SETTING_OCTAVE;
    *settings++ = DQ_CHANNEL_SETTING_TRANSPOSE;
    *settings++ = DQ_CHANNEL_SETTING_FINE;
    *settings++ = DQ_CHANNEL_SETTING_AUX_OUTPUT;
    
    switch(get_aux_mode()) {
      
      case DQ_GATE:
        *settings++ = DQ_CHANNEL_SETTING_PULSEWIDTH;
      break;
      case DQ_COPY:
        *settings++ = DQ_CHANNEL_SETTING_AUX_OCTAVE;
      break;
      default:
      break;
    }

    num_enabled_settings_ = settings - enabled_settings_;
  }

  //

  void RenderScreensaver(weegfx::coord_t x) const;

private:
  bool force_update_;
  bool instant_update_;
  int last_scale_;
  uint16_t last_mask_;
  int32_t last_sample_;
  uint8_t clock_;

  util::TriggerDelay<OC::kMaxTriggerDelayTicks> trigger_delay_;
  braids::Quantizer quantizer_;
  OC::DigitalInputDisplay trigger_display_;

  int num_enabled_settings_;
  DQ_ChannelSetting enabled_settings_[DQ_CHANNEL_SETTING_LAST];

  OC::vfx::ScrollingHistory<int32_t, 5> scrolling_history_;

  bool update_scale(bool force) {
    const int scale = get_scale();
    const uint16_t mask = get_mask();
    if (force || (last_scale_ != scale || last_mask_ != mask)) {
      last_scale_ = scale;
      last_mask_ = mask;
      quantizer_.Configure(OC::Scales::GetScale(scale), mask);
      return true;
    } else {
      return false;
    }
  }
};

const char* const dq_channel_trigger_sources[DQ_CHANNEL_TRIGGER_LAST] = {
  "TR1", "TR3", "cont"
};

const char* const dq_channel_input_sources[DQ_CHANNEL_SOURCE_LAST] = {
  "CV1", "CV3"
};

const char* const dq_seq_scales[] = {
  "s#1", "s#2", "s#3", "s#4"
};

const char* const dq_seq_modes[] = {
  "-", "TR+1", "TR+2", "TR+3"
};

const char* const dq_aux_outputs[] = {
  "gate", "copy"
};

  
SETTINGS_DECLARE(DQ_QuantizerChannel, DQ_CHANNEL_SETTING_LAST) {
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "Scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { 0, 0, 11, "Root", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 },
  { 0, 0, 3, "scale #", dq_seq_scales, settings::STORAGE_TYPE_U4  },
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 },
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 },
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 },
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 },
  { 0, 0, 3, "seq_mode", dq_seq_modes, settings::STORAGE_TYPE_U4 },
  { DQ_CHANNEL_SOURCE_CV1, DQ_CHANNEL_SOURCE_CV1, DQ_CHANNEL_SOURCE_LAST - 1, "CV Source", dq_channel_input_sources, settings::STORAGE_TYPE_U4 },
  { DQ_CHANNEL_TRIGGER_CONTINUOUS, 0, DQ_CHANNEL_TRIGGER_LAST - 1, "Trigger source", dq_channel_trigger_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, OC::kNumDelayTimes - 1, "Trigger delay", OC::Strings::trigger_delay_times, settings::STORAGE_TYPE_U4 },
  { 0, -5, 7, "transpose", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -4, 4, "octave", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -999, 999, "fine", NULL, settings::STORAGE_TYPE_I16 },
  { 0, 0, 1, "aux.output", dq_aux_outputs, settings::STORAGE_TYPE_U4 },
  { 25, 0, PULSEW_MAX, "--> pw", OC::Strings::pulsewidth_ms, settings::STORAGE_TYPE_U8 },
  { 0, -5, 5, "--> aux +/-", NULL, settings::STORAGE_TYPE_I8 }, // aux octave
};

// WIP refactoring to better encapsulate and for possible app interface change
class DualQuantizer {
public:
  void Init() {
    selected_channel = 0;
    cursor.Init(DQ_CHANNEL_SETTING_SCALE, DQ_CHANNEL_SETTING_LAST - 1);
    scale_editor.Init();
  }

  inline bool editing() const {
    return cursor.editing();
  }

  inline int cursor_pos() const {
    return cursor.cursor_pos();
  }

  int selected_channel;
  menu::ScreenCursor<menu::kScreenLines> cursor;
  OC::ScaleEditor<DQ_QuantizerChannel> scale_editor;
};

DualQuantizer dq_state;
DQ_QuantizerChannel dq_quantizer_channels[NUMCHANNELS];

void DQ_init() {

  dq_state.Init();
  for (size_t i = 0; i < NUMCHANNELS; ++i) {
    dq_quantizer_channels[i].Init(static_cast<DQ_ChannelSource>(DQ_CHANNEL_SOURCE_CV1 + i),
                               static_cast<DQ_ChannelTriggerSource>(DQ_CHANNEL_TRIGGER_TR1 + i));
  }

  dq_state.cursor.AdjustEnd(dq_quantizer_channels[0].num_enabled_settings() - 1);
}

size_t DQ_storageSize() {
  return NUMCHANNELS * DQ_QuantizerChannel::storageSize();
}

size_t DQ_save(void *storage) {
  size_t used = 0;
  for (size_t i = 0; i < NUMCHANNELS; ++i) {
    used += dq_quantizer_channels[i].Save(static_cast<char*>(storage) + used);
  }
  return used;
}

size_t DQ_restore(const void *storage) {
  size_t used = 0;
  for (size_t i = 0; i < NUMCHANNELS; ++i) {
    used += dq_quantizer_channels[i].Restore(static_cast<const char*>(storage) + used);
    dq_quantizer_channels[i].update_enabled_settings();
  }
  dq_state.cursor.AdjustEnd(dq_quantizer_channels[0].num_enabled_settings() - 1);
  return used;
}

void DQ_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      dq_state.cursor.set_editing(false);
      dq_state.scale_editor.Close();
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
      break;
  }
}

void DQ_isr() {
  uint32_t triggers = OC::DigitalInputs::clocked();
  // to do
  dq_quantizer_channels[0].Update(triggers, 0, DAC_CHANNEL_A);
  dq_quantizer_channels[1].Update(triggers, 1, DAC_CHANNEL_C);
}

void DQ_loop() {
}

void DQ_menu() {

  menu::DualTitleBar::Draw();
  for (int i = 0, x = 0; i < NUMCHANNELS; ++i, x += 21) {
    const DQ_QuantizerChannel &channel = dq_quantizer_channels[i];
    menu::DualTitleBar::SetColumn(i);
    graphics.print((char)('A' + i));
    graphics.movePrintPos(2, 0);
    int octave = channel.get_octave();
    if (octave)
      graphics.pretty_print(octave);

    menu::DualTitleBar::DrawGateIndicator(i, channel.getTriggerState());
  }
  menu::DualTitleBar::Selected(dq_state.selected_channel);


  const DQ_QuantizerChannel &channel = dq_quantizer_channels[dq_state.selected_channel];

  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(dq_state.cursor);
  menu::SettingsListItem list_item;
  while (settings_list.available()) {
    const int setting =
        channel.enabled_setting_at(settings_list.Next(list_item));
    const int value = channel.get_value(setting);
    const settings::value_attr &attr = DQ_QuantizerChannel::value_attr(setting);

    switch (setting) {
      case DQ_CHANNEL_SETTING_SCALE:
        list_item.SetPrintPos();
        if (list_item.editing) {
          menu::DrawEditIcon(6, list_item.y, value, attr);
          graphics.movePrintPos(6, 0);
        }
        graphics.print(OC::scale_names[value]);
        list_item.DrawCustom();
        break;
      case DQ_CHANNEL_SETTING_MASK1:
      case DQ_CHANNEL_SETTING_MASK2: 
      case DQ_CHANNEL_SETTING_MASK3: 
      case DQ_CHANNEL_SETTING_MASK4:  
        menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, channel.get_mask(), OC::Scales::GetScale(channel.get_scale()).num_notes);
        list_item.DrawNoValue<false>(value, attr);
        break;
      default:
        list_item.DrawDefault(value, attr);
    }
  }

  if (dq_state.scale_editor.active())
    dq_state.scale_editor.Draw();
}

void DQ_handleButtonEvent(const UI::Event &event) {

  if (UI::EVENT_BUTTON_LONG_PRESS == event.type && OC::CONTROL_BUTTON_DOWN == event.control)   
    DQ_downButtonLong(); 
      
  if (dq_state.scale_editor.active()) {
    dq_state.scale_editor.HandleButtonEvent(event);
    return;
  }

  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        DQ_topButton();
        break;
      case OC::CONTROL_BUTTON_DOWN:
        DQ_lowerButton();
        break;
      case OC::CONTROL_BUTTON_L:
        DQ_leftButton();
        break;
      case OC::CONTROL_BUTTON_R:
        DQ_rightButton();
        break;
    }
  } else {
    if (OC::CONTROL_BUTTON_L == event.control)
      DQ_leftButtonLong();       
  }
}

void DQ_handleEncoderEvent(const UI::Event &event) {
  if (dq_state.scale_editor.active()) {
    dq_state.scale_editor.HandleEncoderEvent(event);
    return;
  }

  if (OC::CONTROL_ENCODER_L == event.control) {
    int selected_channel = dq_state.selected_channel + event.value;
    CONSTRAIN(selected_channel, 0, NUMCHANNELS-1);
    dq_state.selected_channel = selected_channel;

    DQ_QuantizerChannel &selected = dq_quantizer_channels[dq_state.selected_channel];
    dq_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    DQ_QuantizerChannel &selected = dq_quantizer_channels[dq_state.selected_channel];
    if (dq_state.editing()) {
      DQ_ChannelSetting setting = selected.enabled_setting_at(dq_state.cursor_pos());
      if (DQ_CHANNEL_SETTING_MASK1 != setting) { // to do
        if (selected.change_value(setting, event.value))
          selected.force_update();

        switch (setting) {
          case DQ_CHANNEL_SETTING_SCALE:
          case DQ_CHANNEL_SETTING_TRIGGER:
          case DQ_CHANNEL_SETTING_SOURCE:
          case DQ_CHANNEL_SETTING_AUX_OUTPUT:
            selected.update_enabled_settings();
            dq_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
          break;
          default:
          break;
        }
      }
    } else {
      dq_state.cursor.Scroll(event.value);
    }
  }
}

void DQ_topButton() {
  DQ_QuantizerChannel &selected = dq_quantizer_channels[dq_state.selected_channel];
  if (selected.change_value(DQ_CHANNEL_SETTING_OCTAVE, 1)) {
    selected.force_update();
  }
}

void DQ_lowerButton() {
  DQ_QuantizerChannel &selected = dq_quantizer_channels[dq_state.selected_channel];
  if (selected.change_value(DQ_CHANNEL_SETTING_OCTAVE, -1)) {
    selected.force_update();
  }
}

void DQ_rightButton() {
  DQ_QuantizerChannel &selected = dq_quantizer_channels[dq_state.selected_channel];
  switch (selected.enabled_setting_at(dq_state.cursor_pos())) {
    case DQ_CHANNEL_SETTING_MASK1:
    case DQ_CHANNEL_SETTING_MASK2:
    case DQ_CHANNEL_SETTING_MASK3:
    case DQ_CHANNEL_SETTING_MASK4: {
      int scale = selected.get_scale();
      if (OC::Scales::SCALE_NONE != scale) {
        dq_state.scale_editor.Edit(&selected, scale);
      }
    }
    break;
    default:
      dq_state.cursor.toggle_editing();
      break;
  }
}

void DQ_leftButton() {
  dq_state.selected_channel = (dq_state.selected_channel + 1) & 3;
  DQ_QuantizerChannel &selected = dq_quantizer_channels[dq_state.selected_channel];
  dq_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
}

void DQ_leftButtonLong() {
  DQ_QuantizerChannel &selected_channel = dq_quantizer_channels[dq_state.selected_channel];
  int scale = selected_channel.get_scale();
  int root = selected_channel.get_root();
  for (int i = 0; i < NUMCHANNELS; ++i) {
    if (i != dq_state.selected_channel) {
      dq_quantizer_channels[i].apply_value(DQ_CHANNEL_SETTING_ROOT, root);
      dq_quantizer_channels[i].set_scale(scale);
    }
  }
}

void DQ_downButtonLong() {

   for (int i = 0; i < NUMCHANNELS; ++i) 
      dq_quantizer_channels[i].instant_update();
}

int32_t dq_history[5];
static const weegfx::coord_t dq_kBottom = 60;

inline int32_t dq_render_pitch(int32_t pitch, weegfx::coord_t x, weegfx::coord_t width) {
  CONSTRAIN(pitch, 0, 120 << 7);
  int32_t octave = pitch / (12 << 7);
  pitch -= (octave * 12 << 7);
  graphics.drawHLine(x, dq_kBottom - ((pitch * 4) >> 7), width);
  return octave;
}

void DQ_QuantizerChannel::RenderScreensaver(weegfx::coord_t start_x) const {

  // History
  scrolling_history_.Read(dq_history);
  weegfx::coord_t scroll_pos = (scrolling_history_.get_scroll_pos() * 6) >> 8;

  // Top: Show gate & CV (or register bits)
  menu::DrawGateIndicator(start_x + 1, 2, getTriggerState());
  const DQ_ChannelSource source = get_source();
  switch (source) {
  
    default: {
      graphics.setPixel(start_x + 31 - 16, 4);
      int32_t cv = OC::ADC::value(static_cast<ADC_CHANNEL>(source));
      cv = (cv * 24 + 2047) >> 12;
      if (cv < 0)
        graphics.drawRect(start_x + 31 - 16 + cv, 6, -cv, 2);
      else if (cv > 0)
        graphics.drawRect(start_x + 31 - 16, 6, cv, 2);
      else
        graphics.drawRect(start_x + 31 - 16, 6, 1, 2);
    }
    break;
  }

#ifdef DQ_DEBUG_SCREENSAVER
  graphics.drawVLinePattern(start_x + 31, 0, 64, 0x55);
#endif

  // Draw semitone intervals, 4px apart
  weegfx::coord_t x = start_x + 26;
  weegfx::coord_t y = dq_kBottom;
  for (int i = 0; i < 12; ++i, y -= 4)
    graphics.setPixel(x, y);

  x = start_x + 1;
  dq_render_pitch(dq_history[0], x, scroll_pos); x += scroll_pos;
  dq_render_pitch(dq_history[1], x, 6); x += 6;
  dq_render_pitch(dq_history[2], x, 6); x += 6;
  dq_render_pitch(dq_history[3], x, 6); x += 6;

  int32_t octave = dq_render_pitch(dq_history[4], x, 6 - scroll_pos);
  graphics.drawBitmap8(start_x + 28, dq_kBottom - octave * 4 - 1, OC::kBitmapLoopMarkerW, OC::bitmap_loop_markers_8 + OC::kBitmapLoopMarkerW);
}

void DQ_screensaver() {
#ifdef DQ_DEBUG_SCREENSAVER
  debug::CycleMeasurement render_cycles;
#endif

  dq_quantizer_channels[0].RenderScreensaver(0);
  dq_quantizer_channels[1].RenderScreensaver(64);

#ifdef DQ_DEBUG_SCREENSAVER
  graphics.drawHLine(0, menu::kMenuLineH, menu::kDisplayWidth);
  uint32_t us = debug::cycles_to_us(render_cycles.read());
  graphics.setPrintPos(0, 32);
  graphics.printf("%u",  us);
#endif
}
