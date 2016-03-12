/*
 * Quad quantizer mode
 */

#include "OC_apps.h"
#include "OC_strings.h"
#include "util/util_settings.h"
#include "util/util_turing.h"
#include "braids_quantizer.h"
#include "braids_quantizer_scales.h"
#include "OC_scales.h"
#include "OC_scale_edit.h"
#include "OC_strings.h"
#include "util_ui.h"

// TODO Extend calibration to get exact octave spacing for inputs?

enum ChannelSettings {
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

  void Init() {
    InitDefaults();

    force_update_ = false;
    last_scale_ = -1;
    last_mask_ = 0;
    last_output_ = 0;
    clock_ = 0;

    turing_machine_.Init();
    quantizer_.Init();
    update_scale(true);
    trigger_display_.Init();
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

    if (CHANNEL_SOURCE_TURING != source) {
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
    } else {
      uint8_t turing_length = get_turing_length();
      turing_machine_.set_length(turing_length);
      int32_t probability = get_turing_prob() + (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) >> 4);
      CONSTRAIN(probability, 0, 255);
      turing_machine_.set_probability(probability);
 
      if (triggered) {
        uint32_t shift_register = turing_machine_.Clock();
        int32_t pitch;
        if (quantizer_.enabled()) {
          uint8_t range = get_turing_range();

          // To use full range of bits is something like:
          // uint32_t scaled = (static_cast<uint64_t>(shift_register) * static_cast<uint64_t>(range)) >> turing_length;
          // Since our range is limited anyway, just grab the last byte
          uint32_t scaled = ((shift_register & 0xff) * range) >> 8;

          // TODO This is just a bodge to get things working;
          // I think we can convert the quantizer codebook to work in a better range
          // The same things happen in all apps that use it, so can be simplified/unified.
          pitch = quantizer_.Lookup(64 + range / 2 - scaled);
          pitch += (get_root() + 60) << 7;
        } else {
          pitch = shift_register;
        }

        //pitch += 3 * 12 << 7; // offset for LUT range
        sample = DAC::pitch_to_dac(pitch, get_octave());
      }
    }

    bool changed = last_output_ != sample;
    if (changed) {
      MENU_REDRAW = 1;
      last_output_ = sample;
      DAC::set(dac_channel, last_output_ + get_fine());
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

  ChannelSettings visible_params() const {
    return CHANNEL_SOURCE_TURING == get_source()
      ? CHANNEL_SETTING_LAST
      : CHANNEL_SETTING_TURING_LENGTH;
  }

private:
  bool force_update_;
  int last_scale_;
  uint16_t last_mask_;
  int32_t last_output_;
  uint8_t clock_;

  util::TuringShiftRegister turing_machine_;
  braids::Quantizer quantizer_;
  OC::DigitalInputDisplay trigger_display_;

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
  "CV1", "CV2", "CV3", "CV4", "TURING"
};

SETTINGS_DECLARE(QuantizerChannel, CHANNEL_SETTING_LAST) {
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { 0, 0, 11, "root", OC::Strings::note_names, settings::STORAGE_TYPE_U8 },
  { 65535, 1, 65535, "active notes", NULL, settings::STORAGE_TYPE_U16 },
  { CHANNEL_SOURCE_CV1, CHANNEL_SOURCE_CV1, CHANNEL_SOURCE_LAST - 1, "source", channel_input_sources, settings::STORAGE_TYPE_U8},
  { CHANNEL_TRIGGER_CONTINUOUS, 0, CHANNEL_TRIGGER_LAST - 1, "trigger", channel_trigger_sources, settings::STORAGE_TYPE_U8 },
  { 1, 1, 16, "clock div", NULL, settings::STORAGE_TYPE_U8 },
  { 0, -5, 7, "transpose", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -4, 4, "octave", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -999, 999, "fine", NULL, settings::STORAGE_TYPE_I16 },
  { 16, 0, 32, "SR length", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "SR probability", NULL, settings::STORAGE_TYPE_U8 },
  { 24, 0, 120, "SR range", NULL, settings::STORAGE_TYPE_U8 }
};

struct QuadQuantizerState {
  int selected_channel;
  int selected_param;
  bool editing;

  OC::ScaleEditor<QuantizerChannel> scale_editor;
};

QuadQuantizerState qq_state;
QuantizerChannel quantizer_channels[4];

void QQ_init() {
  for (size_t i = 0; i < 4; ++i) {
    quantizer_channels[i].Init();
    quantizer_channels[i].apply_value(CHANNEL_SETTING_SOURCE, (int)i); // override
  }

  qq_state.selected_channel = 0;
  qq_state.selected_param = CHANNEL_SETTING_SCALE;
  qq_state.editing = false;
  qq_state.scale_editor.Init();
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
  }
  return used;
}

void QQ_handleEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      encoder[LEFT].setPos(0);
      encoder[RIGHT].setPos(0);
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER:
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
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  buttons(BUTTON_TOP);
  buttons(BUTTON_BOTTOM);
  buttons(BUTTON_LEFT);
  buttons(BUTTON_RIGHT);
}

template <bool rtl>
void draw_mask(weegfx::coord_t y, uint32_t mask, size_t count) {
  weegfx::coord_t x, dx;
  if (count > 16) count = 16;
  if (rtl) {
    x = kUiDisplayWidth - 3;
    dx = -3;
  } else {
    x = kUiDisplayWidth - count * 3;
    dx = 3;
  }

  for (size_t i = 0; i < count; ++i, mask >>= 1, x += dx) {
    if (mask & 0x1)
      graphics.drawRect(x, y + 1, 2, 8);
    else
      graphics.drawRect(x, y + 8, 2, 1);
  }
}

void QQ_menu() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem

  graphics.setFont(UI_DEFAULT_FONT);

  static const weegfx::coord_t kStartX = 0;

  UI_DRAW_TITLE(kStartX);

  for (int i = 0, x = 0; i < 4; ++i, x += 32) {
    const QuantizerChannel &channel = quantizer_channels[i];
    const uint8_t trigger_state = (channel.getTriggerState() + 3) >> 2;
    if (trigger_state)
      graphics.drawBitmap8(x + 1, 2, 4, OC::bitmap_gate_indicators_8 + (trigger_state << 2));

    graphics.setPrintPos(x + 6, 2);
    graphics.print((char)('A' + i));
    graphics.setPrintPos(x + 14, 2);
    int octave = channel.get_octave();
    if (octave)
      graphics.pretty_print(octave);

    if (i == qq_state.selected_channel) {
      graphics.invertRect(x, 0, 32, 11);
    }
  }

  const QuantizerChannel &channel = quantizer_channels[qq_state.selected_channel];

  UI_START_MENU(kStartX);

  int first_visible_param = qq_state.selected_param - 2;
  int last_visible_param = channel.visible_params() - 1;
  if (first_visible_param < CHANNEL_SETTING_SCALE)
    first_visible_param = CHANNEL_SETTING_SCALE;
  else if (first_visible_param + kUiVisibleItems > last_visible_param)
    first_visible_param = last_visible_param - kUiVisibleItems;

  // TODO "Smarter" listing, e.g. hide clkdiv if continuous mode

  UI_BEGIN_ITEMS_LOOP(kStartX, first_visible_param, last_visible_param, qq_state.selected_param, 0)
    if (__selected && qq_state.editing)
      graphics.print(">");
    const settings::value_attr &attr = QuantizerChannel::value_attr(current_item);
    switch (current_item) {
      case CHANNEL_SETTING_SCALE:
        graphics.print(OC::scale_names[channel.get_scale()]);
        UI_END_ITEM();
        break;
      case CHANNEL_SETTING_MASK:
        graphics.print(attr.name);
        draw_mask<false>(y, channel.get_mask(), OC::Scales::GetScale(channel.get_scale()).num_notes);
        UI_END_ITEM();
        break;
      case CHANNEL_SETTING_SOURCE:
        if (CHANNEL_SOURCE_TURING == channel.get_source()) {
          graphics.print(attr.name);
          draw_mask<true>(y, channel.get_shift_register(), channel.get_turing_length());
          UI_END_ITEM();
          break;
        }
      default:
        UI_DRAW_SETTING(attr, channel.get_value(current_item), kUiWideMenuCol1X);
        break;
    }
  UI_END_ITEMS_LOOP();

  if (qq_state.scale_editor.active())
    qq_state.scale_editor.Draw();

  GRAPHICS_END_FRAME();
}

bool QQ_encoders() {
  if (qq_state.scale_editor.active())
    return qq_state.scale_editor.handle_encoders();

  bool changed = false;

  int value = encoder[LEFT].pos();
  encoder[LEFT].setPos(0);
  if (value) {
    int selected_channel = qq_state.selected_channel + value;
    CONSTRAIN(selected_channel, 0, 3);
    qq_state.selected_channel = selected_channel;
    changed = true;
  }

  value = encoder[RIGHT].pos();
  encoder[RIGHT].setPos(0);
  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  if (value) {
    if (qq_state.editing) {
      if (CHANNEL_SETTING_MASK != qq_state.selected_param) {
        if (selected.change_value(qq_state.selected_param, value)) {
          selected.force_update();
          changed = true;
        }
      }
    } else {
      int selected_param = qq_state.selected_param + value;
      CONSTRAIN(selected_param, 0, selected.visible_params() - 2);
      qq_state.selected_param = selected_param;
    }
  }

  return changed;
}

void QQ_topButton() {
  if (qq_state.scale_editor.active()) {
    qq_state.scale_editor.handle_topButton();
    return;
  }

  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  if (selected.change_value(CHANNEL_SETTING_OCTAVE, 1)) {
    if (qq_state.selected_param == CHANNEL_SETTING_OCTAVE)
      encoder[RIGHT].setPos(selected.get_octave());
    selected.force_update();
  }
}

void QQ_lowerButton() {
  if (qq_state.scale_editor.active()) {
    qq_state.scale_editor.handle_bottomButton();
    return;
  }

  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  if (selected.change_value(CHANNEL_SETTING_OCTAVE, -1)) {
    if (qq_state.selected_param == CHANNEL_SETTING_OCTAVE)
      encoder[RIGHT].setPos(selected.get_octave());
    selected.force_update();
  }
}

void QQ_rightButton() {
  if (qq_state.scale_editor.active()) {
    qq_state.scale_editor.handle_rightButton();
    return;
  }

  if (qq_state.editing) {
    qq_state.editing = false;
  } else {
    switch (qq_state.selected_param) {
      case CHANNEL_SETTING_MASK: {
        QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
        int scale = selected.get_scale();
        if (OC::Scales::SCALE_NONE != scale) {
          qq_state.scale_editor.Edit(&selected, scale);
        }
      }
      break;
      default:
      qq_state.editing = true;
    }
  }
}

void QQ_leftButton() {
  if (qq_state.scale_editor.active()) {
    qq_state.scale_editor.handle_leftButton();
    return;
  }

  qq_state.selected_channel = (qq_state.selected_channel + 1) & 3;
}

void QQ_leftButtonLong() {
  if (qq_state.scale_editor.active()) {
    qq_state.scale_editor.handle_leftButtonLong();
    return;
  }

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
