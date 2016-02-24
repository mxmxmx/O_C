/*
 * Quad quantizer mode
 */

#include "OC_apps.h"
#include "OC_strings.h"
#include "util/util_settings.h"
#include "braids_quantizer.h"
#include "braids_quantizer_scales.h"
#include "OC_scales.h"
#include "OC_scale_edit.h"
#include "util_ui.h"

// TODO Extend calibration to get exact octave spacing for inputs?

enum EChannelSettings {
  CHANNEL_SETTING_SCALE,
  CHANNEL_SETTING_ROOT,
  CHANNEL_SETTING_MASK,
  CHANNEL_SETTING_UPDATEMODE,
  CHANNEL_SETTING_TRANSPOSE,
  CHANNEL_SETTING_OCTAVE,
  CHANNEL_SETTING_SOURCE,
  CHANNEL_SETTING_FINE,
  CHANNEL_SETTING_LAST
};

enum EChannelUpdateMode {
  CHANNEL_UPDATE_TRIGGERED,
  CHANNEL_UPDATE_CONTINUOUS,
  CHANNEL_UPDATE_LAST
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

  EChannelUpdateMode get_update_mode() const {
    return static_cast<EChannelUpdateMode>(values_[CHANNEL_SETTING_UPDATEMODE]);
  }

  int get_transpose() const {
    return values_[CHANNEL_SETTING_TRANSPOSE];
  }

  int get_octave() const {
    return values_[CHANNEL_SETTING_OCTAVE];
  }

  ADC_CHANNEL get_source() const {
    return static_cast<ADC_CHANNEL>(values_[CHANNEL_SETTING_SOURCE]);
  }

  int get_fine() const {
    return values_[CHANNEL_SETTING_FINE];
  }

  void Init() {
    force_update_ = false;
    last_scale_ = -1;
    last_mask_ = 0;
    last_output_ = 0;
    quantizer_.Init();
    update_scale(true);
  }

  void force_update() {
    force_update_ = true;
  }

  template <size_t index, DAC_CHANNEL dac_channel>
  inline bool Update() {
    bool forced_update = force_update_;
    force_update_ = false;
    bool triggered = OC::DigitalInputs::clocked<static_cast<OC::DigitalInput>(index)>();
    bool continous = CHANNEL_UPDATE_CONTINUOUS == get_update_mode();

    bool update = forced_update || continous || triggered;
    if (update_scale(forced_update))
      update = true;

    if (update) {
      int32_t transpose = get_transpose();
      ADC_CHANNEL source = get_source();
      int32_t pitch = OC::ADC::value(source);
      if (index != source) {
        transpose += (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) * 12) >> 12;
      }
      if (transpose > 12) transpose = 12;
      else if (transpose < -12) transpose = -12;

      pitch = (pitch * 120 << 7) >> 12; // Convert to range with 128 steps per semitone
      pitch += 3 * 12 << 7; // offset for LUT range

      int32_t quantized = quantizer_.Process(pitch, (get_root() + 60) << 7, transpose);
      quantized += get_octave() * 12 << 7;

      if (quantized > (120 << 7))
        quantized = 120 << 7;
      else if (quantized < 0)
        quantized = 0;

      const int32_t octave = quantized / (12 << 7);
      const int32_t fractional = quantized - octave * (12 << 7);

      int32_t sample = OC::calibration_data.octaves[octave];
      if (fractional)
        sample += (fractional * (OC::calibration_data.octaves[octave + 1] - OC::calibration_data.octaves[octave])) / (12 << 7);

      if (last_output_ != sample) {
        MENU_REDRAW = 1;
        last_output_ = sample;

        DAC::set<dac_channel>(last_output_ + get_fine());
        if (continous)
          return true;
      }
    }
    return !continous && triggered;
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

private:
  bool force_update_;
  int last_scale_;
  uint16_t last_mask_;
  int32_t last_output_;
  braids::Quantizer quantizer_;

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

const char* const update_modes[CHANNEL_UPDATE_LAST] = {
  "trig",
  "cont"
};

const char* const channel_source[ADC_CHANNEL_LAST] = {
  "CV1", "CV2", "CV3", "CV4"
};

SETTINGS_DECLARE(QuantizerChannel, CHANNEL_SETTING_LAST) {
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { 0, 0, 11, "root", OC::Strings::note_names, settings::STORAGE_TYPE_U8 },
  { 65535, 1, 65535, "active notes", NULL, settings::STORAGE_TYPE_U16 },
  { CHANNEL_UPDATE_CONTINUOUS, 0, CHANNEL_UPDATE_LAST - 1, "update", update_modes, settings::STORAGE_TYPE_U8 },
  { 0, -5, 7, "transpose", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -4, 4, "octave", NULL, settings::STORAGE_TYPE_I8 },
  { ADC_CHANNEL_1, ADC_CHANNEL_1, ADC_CHANNEL_LAST - 1, "source", channel_source, settings::STORAGE_TYPE_U8},
  { 0, -999, 999, "fine", NULL, settings::STORAGE_TYPE_I16 },
};

enum EMenuMode {
  MODE_SELECT_CHANNEL,
  MODE_EDIT_CHANNEL
};

struct QuadQuantizerState {
  int selected_channel;
  EMenuMode left_encoder_mode;
  int left_encoder_value;
  int selected_param;

  OC::ScaleEditor<QuantizerChannel> scale_editor;
  OC::DigitalInputDisplay trigger_displays[4];
};

QuadQuantizerState qq_state;
QuantizerChannel quantizer_channels[4];

void QQ_init() {
  for (size_t i = 0; i < 4; ++i) {
    quantizer_channels[i].InitDefaults();
    quantizer_channels[i].Init();
    quantizer_channels[i].apply_value(CHANNEL_SETTING_SOURCE, (int)i); // override
  }

  qq_state.selected_channel = 0;
  qq_state.left_encoder_mode = MODE_SELECT_CHANNEL;
  qq_state.left_encoder_value = 0;
  qq_state.selected_param = CHANNEL_SETTING_ROOT;
  qq_state.scale_editor.Init();

  for (auto &did : qq_state.trigger_displays)  
    did.Init();
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
      switch (qq_state.left_encoder_mode) {
        case MODE_EDIT_CHANNEL:
          encoder[LEFT].setPos(qq_state.left_encoder_value);
          break;
        case MODE_SELECT_CHANNEL:
          encoder[LEFT].setPos(qq_state.selected_channel);
          break;
      }
      if (CHANNEL_SETTING_MASK != qq_state.selected_param)
        encoder[RIGHT].setPos(quantizer_channels[qq_state.selected_channel].get_value(qq_state.selected_param));
      else
        encoder[RIGHT].setPos(0);
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER:
      break;
  }
}

void QQ_isr() {
  bool clocked;

  clocked = quantizer_channels[0].Update<0, DAC_CHANNEL_A>();
  qq_state.trigger_displays[0].Update(1, clocked);

  clocked = quantizer_channels[1].Update<1, DAC_CHANNEL_B>();
  qq_state.trigger_displays[1].Update(1, clocked);

  clocked = quantizer_channels[2].Update<2, DAC_CHANNEL_C>();
  qq_state.trigger_displays[2].Update(1, clocked);

  clocked = quantizer_channels[3].Update<3, DAC_CHANNEL_D>();
  qq_state.trigger_displays[3].Update(1, clocked);
}

void QQ_loop() {
  if (_ENC && (millis() - _BUTTONS_TIMESTAMP > DEBOUNCE)) encoders();
  buttons(BUTTON_TOP);
  buttons(BUTTON_BOTTOM);
  buttons(BUTTON_LEFT);
  buttons(BUTTON_RIGHT);
}

void QQ_menu() {
  GRAPHICS_BEGIN_FRAME(false); // no frame, no problem

  graphics.setFont(UI_DEFAULT_FONT);

  static const weegfx::coord_t kStartX = 0;

  UI_DRAW_TITLE(kStartX);

  for (int i = 0, x = 0; i < 4; ++i, x += 32) {
    uint8_t input_state = (qq_state.trigger_displays[i].getState() + 3) >> 2;
    if (input_state) {
      graphics.drawBitmap8(x + 1, 2, 4, OC::bitmap_gate_indicators_8 + (input_state << 2));
    }

    graphics.setPrintPos(x + 6, 2);
    graphics.print((char)('A' + i));
    graphics.setPrintPos(x + 14, 2);
    int octave = quantizer_channels[i].get_octave();
    if (octave)
      graphics.pretty_print(octave);

    if (i == qq_state.selected_channel) {
      graphics.invertRect(x, 0, 32, 11);
    }
  }

  const QuantizerChannel &channel = quantizer_channels[qq_state.selected_channel];

  UI_START_MENU(kStartX);

  int scale;
  if (MODE_EDIT_CHANNEL == qq_state.left_encoder_mode) {
    scale = qq_state.left_encoder_value;
    if (channel.get_scale() == scale)
      graphics.print(">");
    else
      graphics.print('-');
  } else {
    scale = channel.get_scale();
  }
  graphics.print(OC::scale_names[scale]);

  int first_visible_param = qq_state.selected_param - 2;
  if (first_visible_param < CHANNEL_SETTING_ROOT)
    first_visible_param = CHANNEL_SETTING_ROOT;

  UI_BEGIN_ITEMS_LOOP(kStartX, first_visible_param, CHANNEL_SETTING_LAST, qq_state.selected_param, 1)
    const settings::value_attr &attr = QuantizerChannel::value_attr(current_item);
    if (CHANNEL_SETTING_MASK != current_item) {
      UI_DRAW_SETTING(attr, channel.get_value(current_item), kUiWideMenuCol1X);
    } else {
      graphics.print(attr.name);
      uint16_t mask = channel.get_mask();
      size_t num_notes = OC::Scales::GetScale(channel.get_scale()).num_notes;
      weegfx::coord_t x = kUiDisplayWidth - num_notes * 3;
      for (size_t i = 0; i < num_notes; ++i, mask >>= 1, x+=3) {
        if (mask & 0x1)
          graphics.drawRect(x, y + 1, 2, 8);
      }
      UI_END_ITEM();
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

  switch (qq_state.left_encoder_mode) {
    case MODE_EDIT_CHANNEL:
      if (value != qq_state.left_encoder_value) {
        CONSTRAIN(value, 0, (int)OC::Scales::NUM_SCALES - 1);
        qq_state.left_encoder_value = value;
        encoder[LEFT].setPos(value);
        changed = true;
      }
      break;
    case MODE_SELECT_CHANNEL:
      if (value != qq_state.selected_channel) {
        CONSTRAIN(value, 0, 3);
        qq_state.selected_channel = value;
        encoder[LEFT].setPos(value);
        if (CHANNEL_SETTING_MASK != qq_state.selected_param)
          value = quantizer_channels[qq_state.selected_channel].get_value(qq_state.selected_param);
        else
          value = 0;
        encoder[RIGHT].setPos(value);
        changed = true;
      }
      break;
  }

  QuantizerChannel &selected = quantizer_channels[qq_state.selected_channel];
  value = encoder[RIGHT].pos();
  if (CHANNEL_SETTING_MASK != qq_state.selected_param) {
    if (value != selected.get_value(qq_state.selected_param)) {
      if (selected.apply_value(qq_state.selected_param, value))
        selected.force_update();
      changed = true;
    }
    encoder[RIGHT].setPos(selected.get_value(qq_state.selected_param));
  } else {
    encoder[RIGHT].setPos(0);
    int scale = selected.get_scale();
    if (value && OC::Scales::SCALE_NONE != scale) {
      qq_state.scale_editor.Edit(&selected, scale);
      changed = true;
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

  QuantizerChannel &selected_channel = quantizer_channels[qq_state.selected_channel];
  ++qq_state.selected_param;
  if (qq_state.selected_param >= CHANNEL_SETTING_LAST)
    qq_state.selected_param = CHANNEL_SETTING_ROOT;
  if (CHANNEL_SETTING_MASK != qq_state.selected_param) {
    encoder[RIGHT].setPos(selected_channel.get_value(qq_state.selected_param));
  } else {
    encoder[RIGHT].setPos(0);
  }
}

void QQ_leftButton() {
  if (qq_state.scale_editor.active()) {
    qq_state.scale_editor.handle_leftButton();
    return;
  }

  QuantizerChannel &selected_channel = quantizer_channels[qq_state.selected_channel];
  if (MODE_EDIT_CHANNEL == qq_state.left_encoder_mode) {
    selected_channel.set_scale(qq_state.left_encoder_value);
    qq_state.left_encoder_mode = MODE_SELECT_CHANNEL;
    encoder[LEFT].setPos(qq_state.selected_channel);
  } else {
    qq_state.left_encoder_mode = MODE_EDIT_CHANNEL;
    qq_state.left_encoder_value = selected_channel.get_scale();
    encoder[LEFT].setPos(qq_state.left_encoder_value);
  }
}

void QQ_leftButtonLong() {
  if (qq_state.scale_editor.active()) {
    qq_state.scale_editor.handle_leftButtonLong();
    return;
  }

  QuantizerChannel &selected_channel = quantizer_channels[qq_state.selected_channel];

  if (MODE_SELECT_CHANNEL == qq_state.left_encoder_mode) {
    int scale = selected_channel.get_scale();
    int root = selected_channel.get_root();
    for (int i = 0; i < 4; ++i) {
      quantizer_channels[i].apply_value(CHANNEL_SETTING_ROOT, root);
      quantizer_channels[i].set_scale(scale);
    }
  } else {
    int scale = qq_state.left_encoder_value;
    selected_channel.apply_value(CHANNEL_SETTING_SCALE, scale);
    if (scale != OC::Scales::SCALE_NONE)
      qq_state.scale_editor.Edit(&selected_channel, scale);
  }
}
