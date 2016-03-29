/*
 * Quad quantizer mode
 */

#include "OC_apps.h"
#include "OC_strings.h"
#include "util/util_settings.h"
#include "util/util_turing.h"
#include "util/util_logistic_map.h"
#include "braids_quantizer.h"
#include "braids_quantizer_scales.h"
#include "OC_menus.h"
#include "OC_scales.h"
#include "OC_scale_edit.h"
#include "OC_strings.h"

enum ChannelSetting {
  CHANNEL_SETTING_SCALE,
  CHANNEL_SETTING_ROOT,
  CHANNEL_SETTING_MASK,
  CHANNEL_SETTING_SOURCE,
  CHANNEL_SETTING_TRIGGER,
  CHANNEL_SETTING_CLKDIV,
  CHANNEL_SETTING_TRANSPOSE,
  CHANNEL_SETTING_OCTAVE,
  CHANNEL_SETTING_FINE,
  CHANNEL_SETTING_TURING_LENGTH,
  CHANNEL_SETTING_TURING_PROB,
  CHANNEL_SETTING_TURING_RANGE,
  CHANNEL_SETTING_LOGISTIC_MAP_R,
  CHANNEL_SETTING_LOGISTIC_MAP_RANGE,
  CHANNEL_SETTING_LOGISTIC_MAP_SEED,
  CHANNEL_SETTING_LAST
};

enum ChannelTriggerSource {
  CHANNEL_TRIGGER_TR1,
  CHANNEL_TRIGGER_TR2,
  CHANNEL_TRIGGER_TR3,
  CHANNEL_TRIGGER_TR4,
  CHANNEL_TRIGGER_CONTINUOUS,
  CHANNEL_TRIGGER_LAST
};

enum ChannelSource {
  CHANNEL_SOURCE_CV1,
  CHANNEL_SOURCE_CV2,
  CHANNEL_SOURCE_CV3,
  CHANNEL_SOURCE_CV4,
  CHANNEL_SOURCE_TURING,
  CHANNEL_SOURCE_LOGISTIC_MAP,
  CHANNEL_SOURCE_LAST
};

class QuantizerChannel : public settings::SettingsBase<QuantizerChannel, CHANNEL_SETTING_LAST> {
public:

  int get_scale() const {
    return values_[CHANNEL_SETTING_SCALE];
  }

  void set_scale(int scale) {
    if (scale != get_scale()) {
      const OC::Scale &scale_def = OC::Scales::GetScale(scale);
      uint16_t mask = get_mask();
      if (0 == (mask & ~(0xffff << scale_def.num_notes)))
        mask |= 0x1;
      apply_value(CHANNEL_SETTING_MASK, mask);
      apply_value(CHANNEL_SETTING_SCALE, scale);
    }
  }

  int get_root() const {
    return values_[CHANNEL_SETTING_ROOT];
  }

  uint16_t get_mask() const {
    return values_[CHANNEL_SETTING_MASK];
  }

  ChannelSource get_source() const {
    return static_cast<ChannelSource>(values_[CHANNEL_SETTING_SOURCE]);
  }

  ChannelTriggerSource get_trigger_source() const {
    return static_cast<ChannelTriggerSource>(values_[CHANNEL_SETTING_TRIGGER]);
  }

  uint8_t get_clkdiv() const {
    return values_[CHANNEL_SETTING_CLKDIV];
  }

  int get_transpose() const {
    return values_[CHANNEL_SETTING_TRANSPOSE];
  }

  int get_octave() const {
    return values_[CHANNEL_SETTING_OCTAVE];
  }

  int get_fine() const {
    return values_[CHANNEL_SETTING_FINE];
  }

  uint8_t get_turing_length() const {
    return values_[CHANNEL_SETTING_TURING_LENGTH];
  }

  uint8_t get_turing_prob() const {
    return values_[CHANNEL_SETTING_TURING_PROB];
  }

  uint8_t get_turing_range() const {
    return values_[CHANNEL_SETTING_TURING_RANGE];
  }

  uint8_t get_logistic_map_r() const {
    return values_[CHANNEL_SETTING_LOGISTIC_MAP_R];
  }

  uint8_t get_logistic_map_range() const {
    return values_[CHANNEL_SETTING_LOGISTIC_MAP_RANGE];
  }

  uint8_t get_logistic_map_seed() const {
    return values_[CHANNEL_SETTING_LOGISTIC_MAP_SEED];
  }

  void Init(ChannelSource source, ChannelTriggerSource trigger_source) {
    InitDefaults();
    apply_value(CHANNEL_SETTING_SOURCE, source);
    apply_value(CHANNEL_SETTING_TRIGGER, trigger_source);

    force_update_ = false;
    last_scale_ = -1;
    last_mask_ = 0;
    last_output_ = 0;
    clock_ = 0;

    turing_machine_.Init();
    logistic_map_.Init();
    quantizer_.Init();
    update_scale(true);
    trigger_display_.Init();
    update_enabled_settings();
  }

  void force_update() {
    force_update_ = true;
  }

  inline void Update(uint32_t triggers, size_t index, DAC_CHANNEL dac_channel) {
    bool forced_update = force_update_;
    force_update_ = false;

    ChannelSource source = get_source();
    ChannelTriggerSource trigger_source = get_trigger_source();
    bool continous = CHANNEL_TRIGGER_CONTINUOUS == trigger_source;
    bool triggered = !continous &&
      (triggers & DIGITAL_INPUT_MASK(trigger_source - CHANNEL_TRIGGER_TR1));
    if (triggered) {
      ++clock_;
      if (clock_ >= get_clkdiv()) {
        clock_ = 0;
      } else {
        triggered = false;
      }
    }

    bool update = forced_update || continous || triggered;
    if (update_scale(forced_update))
      update = true;

    int32_t sample = last_output_;

    switch (source) {
      case CHANNEL_SOURCE_TURING: {
          turing_machine_.set_length(get_turing_length());
          int32_t probability = get_turing_prob() + (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) >> 4);
          CONSTRAIN(probability, 0, 255);
          turing_machine_.set_probability(probability);  
          if (triggered) {
            uint32_t shift_register = turing_machine_.Clock();
            uint8_t range = get_turing_range();
            if (quantizer_.enabled()) {
    
              // To use full range of bits is something like:
              // uint32_t scaled = (static_cast<uint64_t>(shift_register) * static_cast<uint64_t>(range)) >> turing_length;
              // Since our range is limited anyway, just grab the last byte
              uint32_t scaled = ((shift_register & 0xff) * range) >> 8;
    
              // TODO This is just a bodge to get things working;
              // I think we can convert the quantizer codebook to work in a better range
              // The same things happen in all apps that use it, so can be simplified/unified.
              int32_t pitch = quantizer_.Lookup(64 + range / 2 - scaled);
              pitch += (get_root() + 60) << 7;
              //pitch += 3 * 12 << 7; // offset for LUT range
              sample = DAC::pitch_to_dac(pitch, get_octave());
            } else {
              // We dont' need a calibrated value here, really
              int octave = get_octave() + 3;
              CONSTRAIN(octave, 0, 6);
              sample = OC::calibration_data.dac.octaves[octave] + (get_transpose() << 7); 
              // range is actually 120 (10 oct) but 65535 / 128 is close enough
              sample += multiply_u32xu32_rshift32((static_cast<uint32_t>(range) * 65535U) >> 7, shift_register << (32 - get_turing_length()));
              sample = USAT16(sample);
            }  
          }
        }
        break;
      case CHANNEL_SOURCE_LOGISTIC_MAP: {
          logistic_map_.set_seed(get_logistic_map_seed());
          int32_t logistic_map_r = get_logistic_map_r() + (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) >> 4);
          CONSTRAIN(logistic_map_r, 0, 255);
          logistic_map_.set_r(logistic_map_r);  
          if (triggered) {
            int64_t logistic_map_x = logistic_map_.Clock();
            uint8_t range = get_logistic_map_range();
            if (quantizer_.enabled()) {   
              uint32_t logistic_scaled = (logistic_map_x * range) >> 24;

              // See above, may need tweaking    
              int32_t pitch = quantizer_.Lookup(64 + range / 2 - logistic_scaled);
              pitch += (get_root() + 60) << 7;
              //pitch += 3 * 12 << 7; // offset for LUT range
              sample = DAC::pitch_to_dac(pitch, get_octave());
            } else {
              int octave = get_octave() + 3;
              CONSTRAIN(octave, 0, 6);
              sample = OC::calibration_data.dac.octaves[octave] + (get_transpose() << 7);
              sample += multiply_u32xu32_rshift24((static_cast<uint32_t>(range) * 65535U) >> 7, logistic_map_x);
              sample = USAT16(sample);
            }
          }
        }
        break;
      default: {
          if (update) {
            int32_t transpose = get_transpose();
            int32_t pitch = OC::ADC::pitch_value(static_cast<ADC_CHANNEL>(source));
            if (index != source) {
              transpose += (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) * 12) >> 12;
            }
            CONSTRAIN(transpose, -12, 12); 
            pitch += 3 * 12 << 7; // offset for LUT range
            sample = DAC::pitch_to_dac(quantizer_.Process(pitch, (get_root() + 60) << 7, transpose), get_octave());
          }
        }
    } // end switch  

    bool changed = last_output_ != sample;
    if (changed) {
      MENU_REDRAW = 1;
      last_output_ = sample;
      DAC::set(dac_channel, sample + get_fine());
    }
    trigger_display_.Update(1, continous ? changed : triggered);
  }

  // Wrappers for ScaleEdit
  void scale_changed() {
    force_update_ = true;
  }

  uint16_t get_scale_mask() const {
    return get_mask();
  }

  void update_scale_mask(uint16_t mask) {
    apply_value(CHANNEL_SETTING_MASK, mask); // Should automatically be updated
  }
  //

  uint8_t getTriggerState() const {
    return trigger_display_.getState();
  }

  uint32_t get_shift_register() const {
    return turing_machine_.get_shift_register();
  }

  // Maintain an internal list of currently available settings, since some are
  // dependent on others. It's kind of brute force, but eh, works :) If other
  // apps have a similar need, it can be moved to a common wrapper

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  ChannelSetting enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  void update_enabled_settings() {
    ChannelSetting *settings = enabled_settings_;
    *settings++ = CHANNEL_SETTING_SCALE;
    if (OC::Scales::SCALE_NONE != get_scale()) {
      *settings++ = CHANNEL_SETTING_ROOT;
      *settings++ = CHANNEL_SETTING_MASK;
    }
    *settings++ = CHANNEL_SETTING_SOURCE;
    switch (get_source()) {
      case CHANNEL_SOURCE_TURING:
        *settings++ = CHANNEL_SETTING_TURING_LENGTH;
        *settings++ = CHANNEL_SETTING_TURING_RANGE;
        *settings++ = CHANNEL_SETTING_TURING_PROB;
      break;
      case CHANNEL_SOURCE_LOGISTIC_MAP:
        *settings++ = CHANNEL_SETTING_LOGISTIC_MAP_R;
        *settings++ = CHANNEL_SETTING_LOGISTIC_MAP_RANGE;
        *settings++ = CHANNEL_SETTING_LOGISTIC_MAP_SEED;
      default:
      break;
    }
    *settings++ = CHANNEL_SETTING_TRIGGER;
    if (CHANNEL_TRIGGER_CONTINUOUS != get_trigger_source())
      *settings++ = CHANNEL_SETTING_CLKDIV;

    *settings++ = CHANNEL_SETTING_OCTAVE;
    *settings++ = CHANNEL_SETTING_TRANSPOSE;
    *settings++ = CHANNEL_SETTING_FINE;

    num_enabled_settings_ = settings - enabled_settings_;
  }

  static bool indentSetting(ChannelSetting s) {
    switch (s) {
      case CHANNEL_SETTING_TURING_LENGTH:
      case CHANNEL_SETTING_TURING_RANGE:
      case CHANNEL_SETTING_TURING_PROB:
      case CHANNEL_SETTING_LOGISTIC_MAP_R:
      case CHANNEL_SETTING_LOGISTIC_MAP_RANGE:
      case CHANNEL_SETTING_LOGISTIC_MAP_SEED:
      case CHANNEL_SETTING_CLKDIV:
        return true;
      default: break;
    }
    return false;
  }

private:
  bool force_update_;
  int last_scale_;
  uint16_t last_mask_;
  int32_t last_output_;
  uint8_t clock_;

  util::TuringShiftRegister turing_machine_;
  util::LogisticMap logistic_map_;
  braids::Quantizer quantizer_;
  OC::DigitalInputDisplay trigger_display_;

  int num_enabled_settings_;
  ChannelSetting enabled_settings_[CHANNEL_SETTING_LAST];

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

const char* const channel_trigger_sources[CHANNEL_TRIGGER_LAST] = {
  "TR1", "TR2", "TR3", "TR4", "cont"
};

const char* const channel_input_sources[CHANNEL_SOURCE_LAST] = {
  "CV1", "CV2", "CV3", "CV4", "Turing", "Lgstc"
};

SETTINGS_DECLARE(QuantizerChannel, CHANNEL_SETTING_LAST) {
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "Scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { 0, 0, 11, "Root", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 },
  { 65535, 1, 65535, "Active notes", NULL, settings::STORAGE_TYPE_U16 },
  { CHANNEL_SOURCE_CV1, CHANNEL_SOURCE_CV1, CHANNEL_SOURCE_LAST - 1, "CV Source", channel_input_sources, settings::STORAGE_TYPE_U4 },
  { CHANNEL_TRIGGER_CONTINUOUS, 0, CHANNEL_TRIGGER_LAST - 1, "Trigger source", channel_trigger_sources, settings::STORAGE_TYPE_U4 },
  { 1, 1, 16, "Clock div", NULL, settings::STORAGE_TYPE_U8 },
  { 0, -5, 7, "Transpose", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -4, 4, "Octave", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -999, 999, "Fine", NULL, settings::STORAGE_TYPE_I16 },
  { 16, 1, 32, "LFSR length", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "LFSR p", NULL, settings::STORAGE_TYPE_U8 },
  { 24, 1, 120, "LFSR range", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 1, 255, "Logistic r", NULL, settings::STORAGE_TYPE_U8 },
  { 24, 1, 120, "Logistic range", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 1, 255, "Logistic seed", NULL, settings::STORAGE_TYPE_U8 }
};

// WIP refactoring to better encapsulate and for possible app interface change
class QuadQuantizer {
public:
  void Init() {
    selected_channel = 0;
    cursor.Init(CHANNEL_SETTING_SCALE, CHANNEL_SETTING_LAST - 1);
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
  OC::ScaleEditor<QuantizerChannel> scale_editor;
};

QuadQuantizer qq_state;
QuantizerChannel quantizer_channels[4];

void QQ_init() {

  qq_state.Init();
  for (size_t i = 0; i < 4; ++i) {
    quantizer_channels[i].Init(static_cast<ChannelSource>(CHANNEL_SOURCE_CV1 + i),
                               static_cast<ChannelTriggerSource>(CHANNEL_TRIGGER_TR1 + i));
  }

  qq_state.cursor.AdjustEnd(quantizer_channels[0].num_enabled_settings() - 1);
}

size_t QQ_storageSize() {
  return 4 * QuantizerChannel::storageSize();
}

size_t QQ_save(void *storage) {
  size_t used = 0;
  for (size_t i = 0; i < 4; ++i) {
    used += quantizer_channels[i].Save(static_cast<char*>(storage) + used);
  }
  return used;
}

size_t QQ_restore(const void *storage) {
  size_t used = 0;
  for (size_t i = 0; i < 4; ++i) {
    used += quantizer_channels[i].Restore(static_cast<const char*>(storage) + used);
    quantizer_channels[i].update_enabled_settings();
  }
  qq_state.cursor.AdjustEnd(quantizer_channels[0].num_enabled_settings() - 1);
  return used;
}

void QQ_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      qq_state.cursor.set_editing(false);
      qq_state.scale_editor.Close();
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
      break;
  }
}

void QQ_isr() {
  uint32_t triggers = OC::DigitalInputs::clocked();
  quantizer_channels[0].Update(triggers, 0, DAC_CHANNEL_A);
  quantizer_channels[1].Update(triggers, 1, DAC_CHANNEL_B);
  quantizer_channels[2].Update(triggers, 2, DAC_CHANNEL_C);
  quantizer_channels[3].Update(triggers, 3, DAC_CHANNEL_D);
}

void QQ_loop() {
}

void QQ_menu() {

  menu::QuadTitleBar::Draw();
  for (int i = 0, x = 0; i < 4; ++i, x += 32) {
    const QuantizerChannel &channel = quantizer_channels[i];
    menu::QuadTitleBar::SetColumn(i);
    graphics.print((char)('A' + i));
    graphics.movePrintPos(2, 0);
    int octave = channel.get_octave();
    if (octave)
      graphics.pretty_print(octave);

    const uint8_t trigger_state = (channel.getTriggerState() + 3) >> 2;
    if (trigger_state)
      graphics.drawBitmap8(x + 1, 2, 4, OC::bitmap_gate_indicators_8 + (trigger_state << 2));
  }
  menu::QuadTitleBar::Selected(qq_state.selected_channel);


  const QuantizerChannel &channel = quantizer_channels[qq_state.selected_channel];

  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(qq_state.cursor);
  menu::SettingsListItem list_item;
  while (settings_list.available()) {
    const int setting =
        channel.enabled_setting_at(settings_list.Next(list_item));
    const int value = channel.get_value(setting);
    const settings::value_attr &attr = QuantizerChannel::value_attr(setting);

    switch (setting) {
      case CHANNEL_SETTING_SCALE:
        list_item.SetPrintPos();
        if (list_item.editing) {
          menu::DrawEditIcon(6, list_item.y, value, attr);
          graphics.movePrintPos(6, 0);
        }
        graphics.print(OC::scale_names[value]);
        list_item.DrawCustom();
        break;
      case CHANNEL_SETTING_MASK:
        menu::DrawMask<false, 16>(list_item.y, channel.get_mask(), OC::Scales::GetScale(channel.get_scale()).num_notes);
        list_item.DrawNoValue<false>(value, attr);
        break;
      case CHANNEL_SETTING_SOURCE:
        if (CHANNEL_SOURCE_TURING == channel.get_source()) {
          int turing_length = channel.get_turing_length();
          int w = turing_length >= 16 ? 16 * 3 : turing_length * 3;

          menu::DrawMask<true, 16>(list_item.y, channel.get_shift_register(), turing_length);
          list_item.valuex = menu::kDisplayWidth - w - 1;
          list_item.DrawNoValue<true>(value, attr);
          break;
          // Fall through if not Turing
        }
      default:
        if (QuantizerChannel::indentSetting(static_cast<ChannelSetting>(setting)))
          list_item.x += menu::kIndentDx;
        list_item.DrawDefault(value, attr);
    }
  }

  if (qq_state.scale_editor.active())
    qq_state.scale_editor.Draw();
}

void QQ_handleButtonEvent(const UI::Event &event) {
  if (qq_state.scale_editor.active()) {
    qq_state.scale_editor.HandleButtonEvent(event);
    return;
  }

  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        QQ_topButton();
        break;
      case OC::CONTROL_BUTTON_DOWN:
        QQ_lowerButton();
        break;
      case OC::CONTROL_BUTTON_L:
        QQ_leftButton();
        break;
      case OC::CONTROL_BUTTON_R:
        QQ_rightButton();
        break;
    }
  } else {
    if (OC::CONTROL_BUTTON_L == event.control)
      QQ_leftButtonLong();
  }
}

void QQ_handleEncoderEvent(const UI::Event &event) {
  if (qq_state.scale_editor.active()) {
    qq_state.scale_editor.HandleEncoderEvent(event);
    return;
  }

  if (OC::CONTROL_ENCODER_L == event.control) {
    int selected_channel = qq_state.selected_channel + event.value;
    CONSTRAIN(selected_channel, 0, 3);
    qq_state.selected_channel = selected_channel;

    QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
    qq_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
    if (qq_state.editing()) {
      ChannelSetting setting = selected.enabled_setting_at(qq_state.cursor_pos());
      if (CHANNEL_SETTING_MASK != setting) {
        if (selected.change_value(setting, event.value))
          selected.force_update();

        switch (setting) {
          case CHANNEL_SETTING_SCALE:
          case CHANNEL_SETTING_TRIGGER:
          case CHANNEL_SETTING_SOURCE:
            selected.update_enabled_settings();
            qq_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
          break;
          default:
          break;
        }
      }
    } else {
      qq_state.cursor.Scroll(event.value);
    }
  }
}

void QQ_topButton() {
  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  if (selected.change_value(CHANNEL_SETTING_OCTAVE, 1)) {
    selected.force_update();
  }
}

void QQ_lowerButton() {
  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  if (selected.change_value(CHANNEL_SETTING_OCTAVE, -1)) {
    selected.force_update();
  }
}

void QQ_rightButton() {
  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  switch (selected.enabled_setting_at(qq_state.cursor_pos())) {
    case CHANNEL_SETTING_MASK: {
      int scale = selected.get_scale();
      if (OC::Scales::SCALE_NONE != scale) {
        qq_state.scale_editor.Edit(&selected, scale);
      }
    }
    break;
    default:
      qq_state.cursor.toggle_editing();
      break;
  }
}

void QQ_leftButton() {
  qq_state.selected_channel = (qq_state.selected_channel + 1) & 3;
  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  qq_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
}

void QQ_leftButtonLong() {
  QuantizerChannel &selected_channel = quantizer_channels[qq_state.selected_channel];
  int scale = selected_channel.get_scale();
  int root = selected_channel.get_root();
  for (int i = 0; i < 4; ++i) {
    if (i != qq_state.selected_channel) {
      quantizer_channels[i].apply_value(CHANNEL_SETTING_ROOT, root);
      quantizer_channels[i].set_scale(scale);
    }
  }
}
