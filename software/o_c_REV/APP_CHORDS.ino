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
#include "OC_chords.h"
#include "OC_chords_edit.h"

enum CHORDS_SETTINGS {
  CHORDS_SETTING_SCALE,
  CHORDS_SETTING_ROOT,
  CHORDS_SETTING_MASK,
  CHORDS_SETTING_CV_SOURCE,
  CHORDS_SETTING_CHORDS_ADVANCE_TRIGGER_SOURCE,
  CHORDS_SETTING_PLAYMODES,
  CHORDS_SETTING_TRIGGER_DELAY,
  CHORDS_SETTING_TRANSPOSE,
  CHORDS_SETTING_OCTAVE,
  CHORDS_SETTING_CHORD_SLOT,
  CHORDS_SETTING_NUM_CHORDS,
  CHORDS_SETTING_CHORD_EDIT,
  // CV sources
  CHORDS_SETTING_ROOT_CV,
  CHORDS_SETTING_MASK_CV,
  CHORDS_SETTING_TRANSPOSE_CV,
  CHORDS_SETTING_OCTAVE_CV,
  CHORDS_SETTING_QUALITY_CV,
  CHORDS_SETTING_VOICING_CV,
  CHORDS_SETTING_INVERSION_CV,
  CHORDS_SETTING_DUMMY,
  CHORDS_SETTING_MORE_DUMMY,
  CHORDS_SETTING_LAST
};

enum CHORDS_CV_SOURCES {
  CHORDS_CV_SOURCE_CV1,
  CHORDS_CV_SOURCE_NONE,
  // todo
  CHORDS_CV_SOURCE_LAST
};

enum CHORDS_ADVANCE_TRIGGER_SOURCE {
  CHORDS_ADVANCE_TRIGGER_SOURCE_TR1,
  CHORDS_ADVANCE_TRIGGER_SOURCE_TR2,
  CHORDS_ADVANCE_TRIGGER_SOURCE_LAST
};

enum CHORDS_CV_DESTINATIONS {
  CHORDS_CV_DEST_NONE,
  CHORDS_CV_DEST_ROOT,
  CHORDS_CV_DEST_OCTAVE,
  CHORDS_CV_DEST_TRANSPOSE,
  CHORDS_CV_DEST_MASK,
  CHORDS_CV_DEST_LAST
};

enum CHORDS_MENU_PAGES {
  MENU_PARAMETERS,
  MENU_CV_MAPPING,
  MENU_PAGES_LAST  
};

class Chords : public settings::SettingsBase<Chords, CHORDS_SETTING_LAST> {
public:

  int get_scale(uint8_t selected_scale_slot_) const {
    return values_[CHORDS_SETTING_SCALE];
  } 

  void set_scale(int scale) {

    if (scale != get_scale(DUMMY)) {
      const OC::Scale &scale_def = OC::Scales::GetScale(scale);
      uint16_t mask = get_mask();
      if (0 == (mask & ~(0xffff << scale_def.num_notes)))
        mask |= 0x1;
      apply_value(CHORDS_SETTING_MASK, mask);
      apply_value(CHORDS_SETTING_SCALE, scale);
    }
  }
  
  // dummy
  int get_scale_select() const {
    return 0;
  }
  // dummy
  void set_scale_at_slot(int scale, uint16_t mask, uint8_t scale_slot) {
  }

  int get_chord_slot() const {
    return values_[CHORDS_SETTING_CHORD_SLOT];
  }

  void set_chord_slot(int8_t slot) {
    apply_value(CHORDS_SETTING_CHORD_SLOT, slot);
  }

  int get_num_chords() const {
    return values_[CHORDS_SETTING_NUM_CHORDS];
  }

  void set_num_chords(int8_t num_chords) {
    apply_value(CHORDS_SETTING_NUM_CHORDS, num_chords);
  }

  int active_chord() const {
    return active_chord_;
  }
  
  int get_root() const {
    return values_[CHORDS_SETTING_ROOT];
  }

  uint8_t get_root_cv() const {
    return values_[CHORDS_SETTING_ROOT_CV];
  }
  
  int get_display_root() const {
    return display_root_;
  }

  uint16_t get_mask() const {
    return values_[CHORDS_SETTING_MASK];
  }

  uint16_t get_rotated_mask() const {
    return last_mask_;
  }

  uint8_t get_mask_cv() const {
    return values_[CHORDS_SETTING_MASK_CV];
  }

  int16_t get_cv_source() const {
    return values_[CHORDS_SETTING_CV_SOURCE];
  }

  int8_t get_chords_trigger_source() const {
    return values_[CHORDS_SETTING_CHORDS_ADVANCE_TRIGGER_SOURCE];
  }

  uint16_t get_trigger_delay() const {
    return values_[CHORDS_SETTING_TRIGGER_DELAY];
  }

  int get_transpose() const {
    return values_[CHORDS_SETTING_TRANSPOSE];
  }

  uint8_t get_transpose_cv() const {
    return values_[CHORDS_SETTING_TRANSPOSE_CV];
  }

  int get_octave() const {
    return values_[CHORDS_SETTING_OCTAVE];
  }

  uint8_t get_octave_cv() const {
    return values_[CHORDS_SETTING_OCTAVE_CV];
  }

  uint8_t get_quality_cv() const {
    return values_[CHORDS_SETTING_QUALITY_CV];
  }

  uint8_t get_inversion_cv() const {
    return values_[CHORDS_SETTING_INVERSION_CV];
  }

  uint8_t get_voicing_cv() const {
    return values_[CHORDS_SETTING_VOICING_CV];
  }

  uint8_t clockState() const {
    return clock_display_.getState();
  }

  uint8_t get_menu_page() const {
    return menu_page_;  
  }

  void set_menu_page(uint8_t _menu_page) {
    menu_page_ = _menu_page;  
  }

  void clear_CV_mapping() {
    // clear all ... 
    apply_value(CHORDS_SETTING_ROOT_CV, 0);
    apply_value(CHORDS_SETTING_MASK_CV, 0);
    apply_value(CHORDS_SETTING_TRANSPOSE_CV, 0);
    apply_value(CHORDS_SETTING_OCTAVE_CV, 0);
    apply_value(CHORDS_SETTING_QUALITY_CV, 0);
    apply_value(CHORDS_SETTING_VOICING_CV, 0); 
    apply_value(CHORDS_SETTING_INVERSION_CV, 0); 
  }

  void Init() {
    
    InitDefaults();
    menu_page_ = PARAMETERS;
    apply_value(CHORDS_SETTING_CV_SOURCE, 0x0);
    set_scale(OC::Scales::SCALE_SEMI);
    force_update_ = true;
    last_scale_= -1;
    last_mask_ = 0;
    last_sample_ = 0;
    schedule_mask_rotate_ = false;
    chord_advance_last_ = true;
    active_chord_ = 0;
    prev_inversion_cv_ = 0;
    prev_root_cv_ = 0;
    prev_octave_cv_ = 0;
    prev_transpose_cv_ = 0;
   
    trigger_delay_.Init();
    quantizer_.Init();
    chords_.Init();
    update_scale(true, false);
    clock_display_.Init();
    update_enabled_settings();

    scrolling_history_.Init(OC::DAC::kOctaveZero * 12 << 7);
  }

  void force_update() {
    force_update_ = true;
  }
 
  inline void Update(uint32_t triggers) {

    bool triggered = triggers & DIGITAL_INPUT_MASK(0x0);

    trigger_delay_.Update();
    if (triggered)
      trigger_delay_.Push(OC::trigger_delay_ticks[get_trigger_delay()]);
    triggered = trigger_delay_.triggered();
    
    if (triggered) 
      update_scale(force_update_, schedule_mask_rotate_);
       
    int32_t sample_a = last_sample_;
    int32_t temp_sample = 0;
    int32_t history_sample = 0;

    if (triggered) {
        
      int32_t pitch, cv_source, transpose, octave, root;
      
      cv_source = get_cv_source();
      transpose = get_transpose();
      octave = get_octave();
      root = get_root();

      
      // next chord via trigger?
      uint8_t _advance_trig = get_chords_trigger_source();
      
      if (_advance_trig == CHORDS_ADVANCE_TRIGGER_SOURCE_TR2) 
        _advance_trig = digitalReadFast(TR2);
      else if (triggered) {
        _advance_trig = 0x0;
        chord_advance_last_ = 0x1;
      }
        
      if (_advance_trig < chord_advance_last_) 
        active_chord_ = active_chord_++ > (get_num_chords() - 1) ? 0x0 : active_chord_; // increment/reset
      else if (!get_num_chords()) 
        active_chord_ = 0x0; 
      chord_advance_last_ = _advance_trig;
      
      // active chord:
      OC::Chord *active_chord = &OC::user_chords[active_chord_];
      
      int8_t _base_note = active_chord->base_note;
      int8_t _octave = active_chord->octave;
      int8_t _quality = active_chord->quality;
      int8_t _voicing = active_chord->voicing;
      int8_t _inversion = active_chord->inversion;

      octave += _octave;
      CONSTRAIN(octave, -6, 6);

      if (_base_note) {
        // we don't use the incoming CV pitch value
        // to do: re-purpose CV1 (?), though can be mapped to misc parameters anyways 
        pitch = 0x0;
        transpose += (_base_note - 0x1);
      }     
      else {
        pitch = quantizer_.enabled()
                  ? OC::ADC::raw_pitch_value(static_cast<ADC_CHANNEL>(cv_source))
                  : OC::ADC::pitch_value(static_cast<ADC_CHANNEL>(cv_source));
      }
              
      switch (cv_source) {

        case CHORDS_CV_SOURCE_CV1:
        break;
        // todo
        default:
        break;
      }
       
    // S/H mode
      if (get_root_cv()) {
          root += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_root_cv() - 1)) + 127) >> 8;
          CONSTRAIN(root, 0, 11);
      }

      if (get_octave_cv()) {
        octave += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_octave_cv() - 1)) + 255) >> 9;
        CONSTRAIN(octave, -4, 4);
      }
      
      if (get_transpose_cv()) {
        transpose += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_transpose_cv() - 1)) + 63) >> 7;
        CONSTRAIN(transpose, -12, 12);
      }

      if (get_quality_cv()) {
        _quality += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_quality_cv() - 1)) + 255) >> 9;
        CONSTRAIN(_quality, 0,  OC::Chords::CHORDS_QUALITY_LAST - 1);
      }

      if (get_inversion_cv()) {
        _inversion += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_inversion_cv() - 1)) + 511) >> 10;
        CONSTRAIN(_inversion, 0,  OC::Chords::CHORDS_INVERSION_LAST - 1);
      }

      if (get_voicing_cv()) {
        _voicing += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_voicing_cv() - 1)) + 255) >> 9;
        CONSTRAIN(_voicing, 0,  OC::Chords::CHORDS_VOICING_LAST - 1);
      }

      if (get_mask_cv()) {
        // to do
      }
      

      display_root_ = root;
      update_scale(true, schedule_mask_rotate_);
      
      int32_t quantized = quantizer_.Process(pitch, root << 7, transpose);
      // main sample, S/H:
      sample_a = temp_sample = OC::DAC::pitch_to_dac(DAC_CHANNEL_A, quantized, octave + OC::inversion[_inversion][0]);

      history_sample = quantized + ((OC::DAC::kOctaveZero + octave) * 12 << 7);

      // now derive chords ...
      
      int32_t sample_b  = quantizer_.Process(pitch, root << 7, transpose + OC::qualities[_quality][1]);
      int32_t sample_c  = quantizer_.Process(pitch, root << 7, transpose + OC::qualities[_quality][2]);
      int32_t sample_d  = quantizer_.Process(pitch, root << 7, transpose + OC::qualities[_quality][3]);

      //todo voicing for root note
      sample_b = OC::DAC::pitch_to_dac(DAC_CHANNEL_B, sample_b, octave + OC::voicing[_voicing][1] + OC::inversion[_inversion][1]);
      sample_c = OC::DAC::pitch_to_dac(DAC_CHANNEL_C, sample_c, octave + OC::voicing[_voicing][2] + OC::inversion[_inversion][2]);
      sample_d = OC::DAC::pitch_to_dac(DAC_CHANNEL_D, sample_d, octave + OC::voicing[_voicing][3] + OC::inversion[_inversion][3]);
      
      OC::DAC::set<DAC_CHANNEL_A>(sample_a);
      OC::DAC::set<DAC_CHANNEL_B>(sample_b);
      OC::DAC::set<DAC_CHANNEL_C>(sample_c);
      OC::DAC::set<DAC_CHANNEL_D>(sample_d);
    }

    bool changed = (last_sample_ != sample_a);
     
    if (changed) {
      MENU_REDRAW = 1;
      last_sample_ = sample_a;
    }

    if (triggered) {
      scrolling_history_.Push(history_sample);
      clock_display_.Update(1, true);
    } else {
      clock_display_.Update(1, false);
    }
    scrolling_history_.Update();
  }

  // Wrappers for ScaleEdit
  void scale_changed() {
    force_update_ = true;
  }

  uint16_t get_scale_mask(uint8_t scale_select) const {
    return get_mask();
  }

  void update_scale_mask(uint16_t mask, uint8_t scale_select) {
    apply_value(CHORDS_SETTING_MASK, mask);
    last_mask_ = mask;
    force_update_ = true;
  }

  // Maintain an internal list of currently available settings, since some are
  // dependent on others. It's kind of brute force, but eh, works :) If other
  // apps have a similar need, it can be moved to a common wrapper

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  CHORDS_SETTINGS enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  void update_enabled_settings() {
 
    CHORDS_SETTINGS *settings = enabled_settings_;

    switch(get_menu_page()) {

      case MENU_PARAMETERS: { 
        
        *settings++ = CHORDS_SETTING_MASK;
        // hide root ?
        if (get_scale(DUMMY) != OC::Scales::SCALE_NONE)  
          *settings++ = CHORDS_SETTING_ROOT;
        else
           *settings++ = CHORDS_SETTING_MORE_DUMMY;
        
        *settings++ = CHORDS_SETTING_CHORD_EDIT;
        *settings++ = CHORDS_SETTING_PLAYMODES;    
        *settings++ = CHORDS_SETTING_TRANSPOSE;
        *settings++ = CHORDS_SETTING_OCTAVE;
        *settings++ = CHORDS_SETTING_CV_SOURCE;
        *settings++ = CHORDS_SETTING_CHORDS_ADVANCE_TRIGGER_SOURCE;
        *settings++ = CHORDS_SETTING_MORE_DUMMY; // ? 
      }
      break;
      case MENU_CV_MAPPING: {

        *settings++ = CHORDS_SETTING_MASK;
        // destinations:
        // hide root CV?
        if (get_scale(DUMMY) != OC::Scales::SCALE_NONE)  
          *settings++ = CHORDS_SETTING_ROOT_CV;
        else
           *settings++ = CHORDS_SETTING_MORE_DUMMY;
        
        *settings++ = CHORDS_SETTING_CHORD_EDIT; // todo: CV
        *settings++ = CHORDS_SETTING_PLAYMODES;  // todo: CV ?
        *settings++ = CHORDS_SETTING_TRANSPOSE_CV;
        *settings++ = CHORDS_SETTING_OCTAVE_CV;
        *settings++ = CHORDS_SETTING_QUALITY_CV;
        *settings++ = CHORDS_SETTING_INVERSION_CV;
        *settings++ = CHORDS_SETTING_VOICING_CV;
      }
      break;
      default:
      break;
    }
    // end switch
    num_enabled_settings_ = settings - enabled_settings_;
  }
  
  void RenderScreensaver(weegfx::coord_t x) const;

private:
  bool force_update_;
  int last_scale_;
  uint16_t last_mask_;
  int32_t schedule_mask_rotate_;
  int32_t last_sample_;
  uint8_t display_root_;
  int8_t prev_octave_cv_;
  int8_t prev_transpose_cv_;
  int8_t prev_root_cv_;
  int8_t prev_inversion_cv_;
  bool chord_advance_last_;
  int8_t active_chord_;
  int8_t menu_page_;

  util::TriggerDelay<OC::kMaxTriggerDelayTicks> trigger_delay_;
  braids::Quantizer quantizer_;
  OC::DigitalInputDisplay clock_display_;
  OC::Chords chords_;
  
  int num_enabled_settings_;
  CHORDS_SETTINGS enabled_settings_[CHORDS_SETTING_LAST];

  OC::vfx::ScrollingHistory<int32_t, 5> scrolling_history_;

  bool update_scale(bool force, int32_t mask_rotate) {

    force_update_ = false;  
    const int scale = get_scale(DUMMY);
    uint16_t mask = get_mask();
    
    if (mask_rotate)
      mask = OC::ScaleEditor<Chords>::RotateMask(mask, OC::Scales::GetScale(scale).num_notes, mask_rotate);
      
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

const char* const chords_advance_trigger_sources[] = {
  "TR1", "TR2"
};

const char* const chords_cv_main_source[] = {
  "CV1", "-"
};

const char* const chords_cv_sources[] = {
  "-", "CV1", "CV2", "CV3", "CV4"
};

const char* const chords_slots[] = {
  "#1", "#2", "#3", "#4", "#5", "#6", "#7", "#8"
};
  
SETTINGS_DECLARE(Chords, CHORDS_SETTING_LAST) {
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { 0, 0, 11, "root", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 }, 
  { 65535, 1, 65535, "scale  -->", NULL, settings::STORAGE_TYPE_U16 }, // mask
  { 0, 0, CHORDS_CV_SOURCE_LAST - 1, "CV source", chords_cv_main_source, settings::STORAGE_TYPE_U4 }, /// to do ..
  { CHORDS_ADVANCE_TRIGGER_SOURCE_TR2, 0, CHORDS_ADVANCE_TRIGGER_SOURCE_LAST - 1, "chords trg src", chords_advance_trigger_sources, settings::STORAGE_TYPE_U8 },
  { 0, 0, PM_LAST - 5, "playmode", OC::Strings::seq_playmodes, settings::STORAGE_TYPE_U8 },
  { 0, 0, OC::kNumDelayTimes - 1, "TR1 delay", OC::Strings::trigger_delay_times, settings::STORAGE_TYPE_U8 },
  { 0, -5, 7, "transpose", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -4, 4, "octave", NULL, settings::STORAGE_TYPE_I8 },
  { 0, 0, OC::Chords::CHORDS_USER_LAST - 1, "chord:", chords_slots, settings::STORAGE_TYPE_U8 },
  { 0, 0, OC::Chords::CHORDS_USER_LAST - 1, "num.chords", NULL, settings::STORAGE_TYPE_U8 },
  {0, 0, 0, "chords -->", NULL, settings::STORAGE_TYPE_U4 }, // = chord editor
  // CV
  {0, 0, 4, "root CV      >", chords_cv_sources, settings::STORAGE_TYPE_U4 },
  {0, 0, 4, "mask CV      >", chords_cv_sources, settings::STORAGE_TYPE_U4 },
  {0, 0, 4, "transpose CV >", chords_cv_sources, settings::STORAGE_TYPE_U4 },
  {0, 0, 4, "octave CV    >", chords_cv_sources, settings::STORAGE_TYPE_U4 },
  {0, 0, 4, "quality CV   >", chords_cv_sources, settings::STORAGE_TYPE_U4 },
  {0, 0, 4, "voicing CV   >", chords_cv_sources, settings::STORAGE_TYPE_U4 },
  {0, 0, 4, "inversion CV >", chords_cv_sources, settings::STORAGE_TYPE_U4 },
  {0, 0, 0, "-", NULL, settings::STORAGE_TYPE_U4 }, // DUMMY 
  {0, 0, 0, " ", NULL, settings::STORAGE_TYPE_U4 }  // MORE DUMMY  
};


class ChordQuantizer {
public:
  void Init() {
    cursor.Init(CHORDS_SETTING_SCALE, CHORDS_SETTING_LAST - 1);
    scale_editor.Init();
    chord_editor.Init();
    left_encoder_value = OC::Scales::SCALE_SEMI;
  }

  inline bool editing() const {
    return cursor.editing();
  }

  inline int cursor_pos() const {
    return cursor.cursor_pos();
  }

  menu::ScreenCursor<menu::kScreenLines> cursor;
  OC::ScaleEditor<Chords> scale_editor;
  OC::ChordEditor<Chords> chord_editor;
  int left_encoder_value;
};

ChordQuantizer chords_state;
Chords chords;

void CHORDS_init() {

  chords.InitDefaults();
  chords.Init();
  chords_state.Init();
  chords.update_enabled_settings();
  chords_state.cursor.AdjustEnd(chords.num_enabled_settings() - 1);
}

size_t CHORDS_storageSize() {
  return Chords::storageSize();
}

size_t CHORDS_save(void *storage) {
  return chords.Save(storage);
}

size_t CHORDS_restore(const void *storage) {
  
  size_t storage_size = chords.Restore(storage);
  chords.update_enabled_settings();
  chords_state.left_encoder_value = chords.get_scale(DUMMY); 
  chords.set_scale(chords_state.left_encoder_value);
  chords_state.cursor.AdjustEnd(chords.num_enabled_settings() - 1);
  return storage_size;
}

void CHORDS_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      chords_state.cursor.set_editing(false);
      chords_state.scale_editor.Close();
      chords_state.chord_editor.Close();
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
      break;
  }
}

void CHORDS_isr() {
  
  uint32_t triggers = OC::DigitalInputs::clocked();
  chords.Update(triggers);
}

void CHORDS_loop() {
}

void CHORDS_menu() {

  menu::TitleBar<0, 4, 0>::Draw();

  // print scale
  int scale = chords_state.left_encoder_value;
  graphics.movePrintPos(5, 0);
  graphics.print(OC::scale_names[scale]);
  if (chords.get_scale(DUMMY) == scale)
    graphics.drawBitmap8(1, menu::QuadTitleBar::kTextY, 4, OC::bitmap_indicator_4x8);

  uint8_t clock_state = (chords.clockState() + 3) >> 2;
  if (clock_state && !chords_state.chord_editor.active())
    graphics.drawBitmap8(121, 2, 4, OC::bitmap_gate_indicators_8 + (clock_state << 2));
  
  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(chords_state.cursor);
  menu::SettingsListItem list_item;

  while (settings_list.available()) {

    const int setting = chords.enabled_setting_at(settings_list.Next(list_item));
    const int value = chords.get_value(setting);
    const settings::value_attr &attr = Chords::value_attr(setting); 

    switch(setting) {

      case CHORDS_SETTING_MASK:
        menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, chords.get_rotated_mask(), OC::Scales::GetScale(chords.get_scale(DUMMY)).num_notes);
        list_item.DrawNoValue<false>(value, attr);
        break;
      case CHORDS_SETTING_DUMMY:
      case CHORDS_SETTING_CHORD_EDIT: 
        // to do: draw something that makes sense, presumably some pre-made icons would work best.
        menu::DrawMiniChord(menu::kDisplayWidth, list_item.y, chords.get_num_chords(), chords.active_chord());
        list_item.DrawNoValue<false>(value, attr);
        break;
      case CHORDS_SETTING_MORE_DUMMY:
        list_item.DrawNoValue<false>(value, attr);
        break;  
      case CHORDS_SETTING_CHORD_SLOT: 
        //special case:
        list_item.DrawValueMax(value, attr, chords.get_num_chords());
        break;    
      default:
        list_item.DrawDefault(value, attr);
        break;
      }
      
   if (chords_state.scale_editor.active())
     chords_state.scale_editor.Draw();
   else if (chords_state.chord_editor.active())
     chords_state.chord_editor.Draw();  
  }
}

void CHORDS_handleButtonEvent(const UI::Event &event) {

  if (UI::EVENT_BUTTON_LONG_PRESS == event.type) {
     switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
         CHORDS_upButtonLong();
        break;
      case OC::CONTROL_BUTTON_DOWN:
        CHORDS_downButtonLong();
        break;
       case OC::CONTROL_BUTTON_L:
        if (!(chords_state.chord_editor.active()))
          CHORDS_leftButtonLong();
        break;  
      default:
        break;
     }
  }
      
  if (chords_state.scale_editor.active()) {
    chords_state.scale_editor.HandleButtonEvent(event);
    return;
  }
  else if (chords_state.chord_editor.active()) {
    chords_state.chord_editor.HandleButtonEvent(event);
    return;
  }

  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        CHORDS_topButton();
        break;
      case OC::CONTROL_BUTTON_DOWN:
        CHORDS_lowerButton();
        break;
      case OC::CONTROL_BUTTON_L:
        CHORDS_leftButton();
        break;
      case OC::CONTROL_BUTTON_R:
        CHORDS_rightButton();
        break;
    }
  } 
}

void CHORDS_handleEncoderEvent(const UI::Event &event) {
  
  if (chords_state.scale_editor.active()) {
    chords_state.scale_editor.HandleEncoderEvent(event);
    return;
  }
  else if (chords_state.chord_editor.active()) {
    chords_state.chord_editor.HandleEncoderEvent(event);
    return;
  }

  if (OC::CONTROL_ENCODER_L == event.control) {
    
    int value = chords_state.left_encoder_value + event.value;
    CONSTRAIN(value, 0, OC::Scales::NUM_SCALES - 1);
    chords_state.left_encoder_value = value;
     
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    
    if (chords_state.editing()) {
      
      CHORDS_SETTINGS setting = chords.enabled_setting_at(chords_state.cursor_pos());
      
      if (CHORDS_SETTING_MASK != setting) { 
        
        if (chords.change_value(setting, event.value))
          chords.force_update();

        switch (setting) {
          case CHORDS_SETTING_CHORD_SLOT:
            // special case, slot shouldn't be > num.chords
            if (chords.get_chord_slot() > chords.get_num_chords())
              chords.set_chord_slot(chords.get_num_chords());
            break;  
          default:
            break;
        }
      }
    } else {
      chords_state.cursor.Scroll(event.value);
    }
  }
}

void CHORDS_topButton() {
  
  if (chords.get_menu_page() == MENU_PARAMETERS) 
    chords.change_value(CHORDS_SETTING_OCTAVE, 1); 
  else  {
    chords.set_menu_page(MENU_PARAMETERS);
    chords.update_enabled_settings();
    chords_state.cursor.set_editing(false);
  }
}

void CHORDS_lowerButton() {
  
  if (chords.get_menu_page() == MENU_PARAMETERS) 
    chords.change_value(CHORDS_SETTING_OCTAVE, -1); 
  else {
    chords.set_menu_page(MENU_PARAMETERS);
    chords.update_enabled_settings();
    chords_state.cursor.set_editing(false);
  }
}

void CHORDS_rightButton() {
  
  switch (chords.enabled_setting_at(chords_state.cursor_pos())) {

    case CHORDS_SETTING_MASK: {
      int scale = chords.get_scale(DUMMY);
      if (OC::Scales::SCALE_NONE != scale)
        chords_state.scale_editor.Edit(&chords, scale);
      }
    break;
    case CHORDS_SETTING_CHORD_EDIT:
      chords_state.chord_editor.Edit(&chords, chords.get_chord_slot(), chords.get_num_chords());
    break;
    case CHORDS_SETTING_DUMMY:
    case CHORDS_SETTING_MORE_DUMMY:
      chords.set_menu_page(MENU_PARAMETERS);
      chords.update_enabled_settings();
    break;
    default:
      chords_state.cursor.toggle_editing();
    break;    
  }
}

void CHORDS_leftButton() {

  if (chords_state.left_encoder_value != asr.get_scale(DUMMY) || chords_state.left_encoder_value == OC::Scales::SCALE_SEMI) { 
    chords.set_scale(chords_state.left_encoder_value);
    // hide/show root
    chords.update_enabled_settings();
  }
}

void CHORDS_leftButtonLong() {
  // todo
}

void CHORDS_downButtonLong() {

  switch (chords.get_menu_page()) { 
     
    case MENU_PARAMETERS:  
    {
      if (!chords_state.chord_editor.active() && !chords_state.scale_editor.active()) {  
        chords.set_menu_page(MENU_CV_MAPPING);
        chords.update_enabled_settings();
        chords_state.cursor.set_editing(false);
      } 
    }
    break;
    case MENU_CV_MAPPING:
      chords.clear_CV_mapping();
      chords_state.cursor.set_editing(false);
    break;
    default:
    break;
  }
}

void CHORDS_upButtonLong() {
  // todo
}

int32_t chords_history[5];
static const weegfx::coord_t chords_kBottom = 60;

inline int32_t chords_render_pitch(int32_t pitch, weegfx::coord_t x, weegfx::coord_t width) {
  
  CONSTRAIN(pitch, 0, 120 << 7);
  int32_t octave = pitch / (12 << 7);
  pitch -= (octave * 12 << 7);
  graphics.drawHLine(x, chords_kBottom - ((pitch * 4) >> 7), width << 1);
  return octave;
}

void Chords::RenderScreensaver(weegfx::coord_t start_x) const {

  // History
  scrolling_history_.Read(chords_history);
  weegfx::coord_t scroll_pos = (scrolling_history_.get_scroll_pos() * 6) >> 8;

  // Top: Show gate & CV (or register bits)
  menu::DrawGateIndicator(start_x + 1, 2, clockState());
  const uint8_t source = get_cv_source();
  
  switch (source) {

    //to do: other sources
    default: {
      graphics.setPixel(start_x + 47 - 16, 4);
      int32_t cv = OC::ADC::value(static_cast<ADC_CHANNEL>(source));
      cv = (cv * 20 + 2047) >> 11;
      if (cv < 0)
        graphics.drawRect(start_x + 47 - 16 + cv, 6, -cv, 2);
      else if (cv > 0)
        graphics.drawRect(start_x + 47 - 16, 6, cv, 2);
      else
        graphics.drawRect(start_x + 47 - 16, 6, 1, 2);
    }
    break;
  }

#ifdef CHORDS_DEBUG_SCREENSAVER
  graphics.drawVLinePattern(start_x + 63, 0, 64, 0x55);
#endif

  // Draw semitone intervals, 4px apart
  weegfx::coord_t x = start_x + 56;
  weegfx::coord_t y = chords_kBottom;
  for (int i = 0; i < 12; ++i, y -= 4)
    graphics.setPixel(x, y);

  x = start_x + 1;
  chords_render_pitch(chords_history[0], x, scroll_pos); x += scroll_pos;
  chords_render_pitch(chords_history[1], x, 6); x += 6;
  chords_render_pitch(chords_history[2], x, 6); x += 6;
  chords_render_pitch(chords_history[3], x, 6); x += 6;

  int32_t octave = chords_render_pitch(chords_history[4], x, 6 - scroll_pos);
  graphics.drawBitmap8(start_x + 58, chords_kBottom - octave * 4 - 1, OC::kBitmapLoopMarkerW, OC::bitmap_loop_markers_8 + OC::kBitmapLoopMarkerW);

  graphics.setPrintPos(64, 0);
  OC::Chord *active_chord = &OC::user_chords[chords.active_chord()];
  graphics.print(OC::quality_names[active_chord->quality]);

  graphics.setPrintPos(64, 55);
  graphics.print(OC::voicing_names[active_chord->voicing]);

  menu::DrawChord(85, 40, 6, chords.active_chord());
}

void CHORDS_screensaver() {
#ifdef CHORDS_DEBUG_SCREENSAVER
  debug::CycleMeasurement render_cycles;
#endif

  chords.RenderScreensaver(0);
 
#ifdef CHORDS_DEBUG_SCREENSAVER
  graphics.drawHLine(0, menu::kMenuLineH, menu::kDisplayWidth);
  uint32_t us = debug::cycles_to_us(render_cycles.read());
  graphics.setPrintPos(0, 32);
  graphics.printf("%u",  us);
#endif
}
