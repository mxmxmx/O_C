// Copyright (c) 2015, 2016, 2017 Patrick Dowling, Tim Churches, Max Stadler
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
#include "OC_input_map.h"
#include "OC_input_maps.h"

enum CHORDS_SETTINGS {
  CHORDS_SETTING_SCALE,
  CHORDS_SETTING_ROOT,
  CHORDS_SETTING_PROGRESSION,
  CHORDS_SETTING_MASK,
  CHORDS_SETTING_CV_SOURCE,
  CHORDS_SETTING_CHORDS_ADVANCE_TRIGGER_SOURCE,
  CHORDS_SETTING_PLAYMODES,
  CHORDS_SETTING_DIRECTION,
  CHORDS_SETTING_BROWNIAN_PROBABILITY,
  CHORDS_SETTING_TRIGGER_DELAY,
  CHORDS_SETTING_TRANSPOSE,
  CHORDS_SETTING_OCTAVE,
  CHORDS_SETTING_CHORD_SLOT,
  CHORDS_SETTING_NUM_CHORDS_0,
  CHORDS_SETTING_NUM_CHORDS_1,
  CHORDS_SETTING_NUM_CHORDS_2,
  CHORDS_SETTING_NUM_CHORDS_3,
  CHORDS_SETTING_CHORD_EDIT,
  // CV sources
  CHORDS_SETTING_ROOT_CV,
  CHORDS_SETTING_MASK_CV,
  CHORDS_SETTING_TRANSPOSE_CV,
  CHORDS_SETTING_OCTAVE_CV,
  CHORDS_SETTING_QUALITY_CV,
  CHORDS_SETTING_VOICING_CV,
  CHORDS_SETTING_INVERSION_CV,
  CHORDS_SETTING_PROGRESSION_CV,
  CHORDS_SETTING_DIRECTION_CV,
  CHORDS_SETTING_BROWNIAN_CV,
  CHORDS_SETTING_NUM_CHORDS_CV,
  CHORDS_SETTING_DUMMY,
  CHORDS_SETTING_MORE_DUMMY,
  CHORDS_SETTING_LAST
};

enum CHORDS_CV_SOURCES {
  CHORDS_CV_SOURCE_CV1,
  CHORDS_CV_SOURCE_CV2,
  CHORDS_CV_SOURCE_CV3,
  CHORDS_CV_SOURCE_CV4,
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

enum CHORDS_PLAYMODES {
  _NONE,
  _SEQ1,
  _SEQ2,
  _SEQ3,
  _TR1,
  _TR2,
  _TR3,
  _SH1,
  _SH2,
  _SH3,
  _SH4,
  _CV1,
  _CV2,
  _CV3,
  _CV4,
  CHORDS_PLAYMODES_LAST
};

enum CHORDS_DIRECTIONS {
  CHORDS_FORWARD,
  CHORDS_REVERSE,
  CHORDS_PENDULUM1,
  CHORDS_PENDULUM2,
  CHORDS_RANDOM,
  CHORDS_BROWNIAN,
  CHORDS_DIRECTIONS_LAST
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
  void set_scale_at_slot(int scale, uint16_t mask, int root, int transpose, uint8_t scale_slot) {
    
  }

  // dummy
  int get_transpose(uint8_t DUMMY) const {
    return 0;
  }

  bool octave_toggle() {
    _octave_toggle = (~_octave_toggle) & 1u;
    return _octave_toggle;
  }
  
  bool poke_octave_toggle() const {
    return _octave_toggle;
  }
  
  int get_progression() const {
    return values_[CHORDS_SETTING_PROGRESSION];
  }

  int get_progression_cv() const {
    return values_[CHORDS_SETTING_PROGRESSION_CV];
  }
  
  int get_active_progression() const {
    return active_progression_;
  }

  int get_chord_slot() const {
    return values_[CHORDS_SETTING_CHORD_SLOT];
  }

  void set_chord_slot(int8_t slot) {
    apply_value(CHORDS_SETTING_CHORD_SLOT, slot);
  }

  int get_num_chords(uint8_t progression) const {

     int len = 0x0;
     switch (progression) {
      
        case 0:
          len = values_[CHORDS_SETTING_NUM_CHORDS_0];
        break;
        case 1:
          len = values_[CHORDS_SETTING_NUM_CHORDS_1];
        break;
        case 2:
          len = values_[CHORDS_SETTING_NUM_CHORDS_2];
        break;
        case 3:
          len =  values_[CHORDS_SETTING_NUM_CHORDS_3];
        break;
        default:
        break;
    }
    return len;
  }

  void set_num_chords(int8_t num_chords, uint8_t progression) {

    // set progression length:
    switch (progression) {
      case 0:
      apply_value(CHORDS_SETTING_NUM_CHORDS_0, num_chords);
      break;
      case 1:
      apply_value(CHORDS_SETTING_NUM_CHORDS_1, num_chords);
      break;
      case 2:
      apply_value(CHORDS_SETTING_NUM_CHORDS_2, num_chords);
      break;
      case 3:
      apply_value(CHORDS_SETTING_NUM_CHORDS_3, num_chords);
      break;
      default:
      break;
    }
  }

  int get_num_chords_cv() const {
    return values_[CHORDS_SETTING_NUM_CHORDS_CV];
  }

  int active_chord() const {
    return active_chord_;
  }

  int get_playmode() const {
    return values_[CHORDS_SETTING_PLAYMODES];
  }

  int get_direction() const {
    return values_[CHORDS_SETTING_DIRECTION];
  }

  uint8_t get_direction_cv() const {
    return values_[CHORDS_SETTING_DIRECTION_CV];
  }

  uint8_t get_brownian_probability() const {
    return values_[CHORDS_SETTING_BROWNIAN_PROBABILITY];
  }

  int8_t get_brownian_probability_cv() const {
    return values_[CHORDS_SETTING_BROWNIAN_CV];
  }
  
  int get_root() const {
    return values_[CHORDS_SETTING_ROOT];
  }

  int get_root(uint8_t DUMMY) const {
    return 0x0;
  }

  uint8_t get_root_cv() const {
    return values_[CHORDS_SETTING_ROOT_CV];
  }
  
  int get_display_num_chords() const {
    return display_num_chords_;
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

  void update_inputmap(int num_slots, uint8_t range) {
    input_map_.Configure(OC::InputMaps::GetInputMap(num_slots), range);
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
    apply_value(CHORDS_SETTING_BROWNIAN_CV, 0);  
    apply_value(CHORDS_SETTING_DIRECTION_CV, 0);
    apply_value(CHORDS_SETTING_PROGRESSION_CV, 0);   
    apply_value(CHORDS_SETTING_NUM_CHORDS_CV, 0); 
  }
  
  void Init() {
    
    InitDefaults();
    menu_page_ = PARAMETERS;
    apply_value(CHORDS_SETTING_CV_SOURCE, 0x0);
    set_scale(OC::Scales::SCALE_SEMI);
    force_update_ = true;
    _octave_toggle = false;
    last_scale_= -1;
    last_mask_ = 0;
    last_sample_ = 0;
    chord_advance_last_ = true;
    progression_advance_last_ = true;
    active_chord_ = 0;
    chord_repeat_ = false;
    progression_cnt_ = 0;
    active_progression_ = 0;
    playmode_last_ = 0;
    progression_last_ = 0;
    progression_EoP_ = 0;
    num_chords_last_ = 0;
    chords_direction_ = true;
    display_num_chords_ = 0x1;
   
    trigger_delay_.Init();
    input_map_.Init();
    quantizer_.Init();
    chords_.Init();
    update_scale(true, false);
    clock_display_.Init();
    update_enabled_settings();
  }

  void force_update() {
    //force_update_ = true;
  }

  int8_t _clock(uint8_t sequence_length, uint8_t sequence_count, uint8_t sequence_max, bool _reset) {

        int8_t EoP = 0x0, _clk_cnt, _direction;
        bool reset = !digitalReadFast(TR4) | _reset;
        
        _clk_cnt = active_chord_;
        _direction = get_direction();

        if (get_direction_cv()) {
           _direction += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_direction_cv() - 1)) + 255) >> 9;
           CONSTRAIN(_direction, 0, CHORDS_DIRECTIONS_LAST - 0x1);
        }
        
        switch (_direction) {

          case CHORDS_FORWARD:
          {
            _clk_cnt++;
            if (reset)
              _clk_cnt = 0x0;
            // end of sequence?  
            else if (_clk_cnt > sequence_length)
              _clk_cnt = 0x0;
            else if (_clk_cnt == sequence_length)
               EoP = 0x1;  
          }
          break;
          case CHORDS_REVERSE:
          {
            _clk_cnt--;
            if (reset)
              _clk_cnt = sequence_length;
            // end of sequence? 
            else if (_clk_cnt < 0) 
              _clk_cnt = sequence_length;
            else if (!_clk_cnt)  
               EoP = 0x1;
          }
          break;
          case CHORDS_PENDULUM1:
          case CHORDS_BROWNIAN:
          if (CHORDS_BROWNIAN == get_direction()) {
            // Compare Brownian probability and reverse direction if needed
            int16_t brown_prb = get_brownian_probability();

            if (get_brownian_probability_cv()) {
              brown_prb += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_brownian_probability_cv() - 1)) + 8) >> 3;
              CONSTRAIN(brown_prb, 0, 256);
            }
            if (random(0,256) < brown_prb) 
              chords_direction_ = !chords_direction_; 
          }
          {
            if (chords_direction_) {
              _clk_cnt++;  
              if (reset)
                _clk_cnt = 0x0;
              else if (_clk_cnt >= sequence_length) {
                
                if (sequence_count >= sequence_max) {
                  chords_direction_ = false;
                  _clk_cnt = sequence_length; 
                }
                else EoP = 0x1;
              } 
            }
            // reverse direction:
            else {
              _clk_cnt--; 
              if (reset)  
                _clk_cnt = sequence_length;
              else if (_clk_cnt <= 0) {
                // end of sequence ? 
                if (sequence_count == 0x0) {
                  chords_direction_ = true;
                  _clk_cnt = 0x0; 
                }
                else EoP = -0x1;
              } 
            }
          }
          break;
          case CHORDS_PENDULUM2:
          {
            if (chords_direction_) {

              if (!chord_repeat_)
                _clk_cnt++;
              chord_repeat_ = false;
               
              if (reset)
                _clk_cnt = 0x0;
              else if (_clk_cnt >= sequence_length) {
                // end of sequence ? 
                if (sequence_count >= sequence_max) {
                  chords_direction_ = false;
                  _clk_cnt = sequence_length;  
                  chord_repeat_ = true; // repeat last step
                }
                else EoP = 0x1;  
              }
            }
            // reverse direction:
            else {
              
              if (!chord_repeat_)
                _clk_cnt--; 
              chord_repeat_ = false;
              
              if (reset)  
                _clk_cnt = sequence_length;
              else if (_clk_cnt <= 0x0) {
                // end of sequence ? 
                if (sequence_count == 0x0) {
                  chords_direction_ = true;
                  _clk_cnt = 0x0; 
                  chord_repeat_ = true; // repeat first step
                }
                else EoP = -0x1;  
              }
            }
          }
          break;
          case CHORDS_RANDOM:
          _clk_cnt = random(sequence_length + 0x1);
          if (reset)
            _clk_cnt = 0x0;
          // jump to next sequence if we happen to hit the last note:  
          else if (_clk_cnt >= sequence_length)
            EoP = random(0x2);  
          break;
          default:
          break;
        }
        active_chord_ = _clk_cnt;
        return EoP;
  }

  
  inline void Update(uint32_t triggers) {

    bool triggered = triggers & DIGITAL_INPUT_MASK(0x0);

    trigger_delay_.Update();
    if (triggered)
      trigger_delay_.Push(OC::trigger_delay_ticks[get_trigger_delay()]);
    triggered = trigger_delay_.triggered();
           
    int32_t sample_a = last_sample_;
    int32_t temp_sample = 0;

    if (triggered) {
        
      int32_t pitch, cv_source, transpose, octave, root, mask_rotate;
      int8_t num_progression, num_progression_cv, num_chords, num_chords_cv, progression_max, progression_cnt, playmode, reset;
      
      cv_source = get_cv_source();
      transpose = get_transpose();
      octave = get_octave();
      root = get_root();
      num_progression = get_progression();
      progression_max = 0;
      progression_cnt = 0;
      num_progression_cv = 0;
      num_chords = 0;
      num_chords_cv = 0;
      reset = 0;
      mask_rotate = 0;
      playmode = get_playmode();

      // update mask?
      if (get_mask_cv()) {
        mask_rotate = (OC::ADC::value(static_cast<ADC_CHANNEL>(get_mask_cv() - 0x1)) + 127) >> 8;
      }
              
      update_scale(force_update_, mask_rotate);
      
      if (num_progression != progression_last_ || playmode != playmode_last_) {
        // reset progression:
        progression_cnt_ = 0x0;
        active_progression_ = num_progression;
      }
      playmode_last_ = playmode;
      progression_last_ = num_progression;

      if (get_progression_cv()) {
        num_progression_cv = num_progression += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_progression_cv() - 1)) + 255) >> 9;
        CONSTRAIN(num_progression, 0, OC::Chords::NUM_CHORD_PROGRESSIONS - 0x1);
      }

      if (get_num_chords_cv()) 
        num_chords_cv = (OC::ADC::value(static_cast<ADC_CHANNEL>(get_num_chords_cv() - 1)) + 255) >> 9;

      switch (playmode) {
        
          case _NONE:
            active_progression_ = num_progression;
          break;
          case _SEQ1:
          case _SEQ2:
          case _SEQ3:
          {
              progression_max = playmode;
              
              if (progression_EoP_) {
                
                // increment progression #
                progression_cnt_ += progression_EoP_;
                // reset progression #
                progression_cnt_ = progression_cnt_ > progression_max ? 0x0 : progression_cnt_;
                // update progression 
                active_progression_ = num_progression + progression_cnt_;
                // wrap around:
                if (active_progression_ >= OC::Chords::NUM_CHORD_PROGRESSIONS)
                    active_progression_ -= OC::Chords::NUM_CHORD_PROGRESSIONS;
                // reset    
                _clock(get_num_chords(active_progression_), 0x0, progression_max, true); 
                reset = true;    
              }
              else if (num_progression_cv)  {
                active_progression_ += num_progression_cv;
                CONSTRAIN(active_progression_, 0, OC::Chords::NUM_CHORD_PROGRESSIONS - 0x1);
              }
              progression_cnt = progression_cnt_;
          }
          break;
          case _TR1:
          case _TR2:
          case _TR3:
          {
            // get trigger
            uint8_t _progression_advance_trig = digitalReadFast(TR3);
            progression_max = playmode - _SEQ3;
            
            if (_progression_advance_trig  < progression_advance_last_) {
              // increment progression #
              progression_cnt_++;
              // reset progression #
              progression_cnt_ = progression_cnt_ > progression_max ? 0x0 : progression_cnt_;
              // update progression 
              active_progression_ = num_progression + progression_cnt_;
              // + reset 
              reset = true;
              // wrap around:
              if (active_progression_ >= OC::Chords::NUM_CHORD_PROGRESSIONS)
                  active_progression_ -= OC::Chords::NUM_CHORD_PROGRESSIONS;
            }
            else if (num_progression_cv)  {
                active_progression_ += num_progression_cv;
                CONSTRAIN(active_progression_, 0, OC::Chords::NUM_CHORD_PROGRESSIONS - 0x1);
            }
            progression_advance_last_ = _progression_advance_trig;
            progression_max = 0x0; 
          }
          break;
          case _SH1:
          case _SH2:
          case _SH3:
          case _SH4:
          {
             // SH?
             uint8_t _progression_advance_trig = digitalReadFast(TR3);
             if (_progression_advance_trig  < progression_advance_last_) {
              
               num_chords = get_num_chords(num_progression) + num_chords_cv;
               if (num_chords_cv)
                  CONSTRAIN(num_chords, 0, OC::Chords::NUM_CHORDS - 0x1);         
               // length changed? 
               if (num_chords_last_ != num_chords) 
                  update_inputmap(num_chords + 0x1, 0x0); 
               // store values:  
               num_chords_last_ = num_chords;    
               active_progression_ = num_progression;
               // process input: 
               active_chord_ = input_map_.Process(OC::ADC::value(static_cast<ADC_CHANNEL>(playmode - _SH1)));
             }
             progression_advance_last_ = _progression_advance_trig;
          }
          break;
          case _CV1:
          case _CV2:
          case _CV3:
          case _CV4:
          {
             num_chords = get_num_chords(num_progression) + num_chords_cv;
             if (num_chords_cv) 
                CONSTRAIN(num_chords, 0, OC::Chords::NUM_CHORDS - 0x1);       
             // length changed ? 
             if (num_chords_last_ != num_chords) 
                update_inputmap(num_chords + 0x1, 0x0); 
             // store values:  
             num_chords_last_ = num_chords;    
             active_progression_ = num_progression;
             // process input: 
             active_chord_ = input_map_.Process(OC::ADC::value(static_cast<ADC_CHANNEL>(playmode - _CV1)));
          }
          break;
          default:
          break;
      }

      num_progression = active_progression_;

      if (playmode < _SH1) {
        // next chord via trigger?
        uint8_t _advance_trig = get_chords_trigger_source();
        
        if (_advance_trig == CHORDS_ADVANCE_TRIGGER_SOURCE_TR2) 
          _advance_trig = digitalReadFast(TR2);
        else if (triggered) {
          _advance_trig = 0x0;
          chord_advance_last_ = 0x1;
        }
  
        num_chords = get_num_chords(num_progression) + num_chords_cv;
        if (num_chords_cv) 
            CONSTRAIN(num_chords, 0, OC::Chords::NUM_CHORDS - 0x1);
             
        CONSTRAIN(active_chord_, 0x0, num_chords);
        
        if (num_chords && (_advance_trig < chord_advance_last_)) 
          progression_EoP_ = _clock(num_chords, progression_cnt, progression_max, reset); 
        chord_advance_last_ = _advance_trig;
      } 

      display_num_chords_ = num_chords;
      // active chord:
      OC::Chord *active_chord = &OC::user_chords[active_chord_ + num_progression * OC::Chords::NUM_CHORDS];
      
      int8_t _base_note = active_chord->base_note;
      int8_t _octave = active_chord->octave;
      int8_t _quality = active_chord->quality;
      int8_t _voicing = active_chord->voicing;
      int8_t _inversion = active_chord->inversion;

      octave += _octave;
      CONSTRAIN(octave, -6, 6);

      if (_base_note) {
        /* 
        *  we don't use the incoming CV pitch value — limit to valid base notes. 
        */
        int8_t _limit = OC::Scales::GetScale(get_scale(DUMMY)).num_notes;
        _base_note = _base_note > _limit ? _limit : _base_note;
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
          CONSTRAIN(root, 0, 15);
      }

      if (get_octave_cv()) {
        octave += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_octave_cv() - 1)) + 255) >> 9;
        CONSTRAIN(octave, -4, 4);
      }
      
      if (get_transpose_cv()) {
        transpose += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_transpose_cv() - 1)) + 63) >> 7;
        CONSTRAIN(transpose, -15, 15);
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
      
      int32_t quantized = quantizer_.Process(pitch, root << 7, transpose);
      // main sample, S/H:
      sample_a = temp_sample = OC::DAC::pitch_to_scaled_voltage_dac(DAC_CHANNEL_A, quantized, octave + OC::inversion[_inversion][0], OC::DAC::get_voltage_scaling(DAC_CHANNEL_A));
 
      // now derive chords ...
      transpose += OC::qualities[_quality][1];
      int32_t sample_b  = quantizer_.Process(pitch, root << 7, transpose);
      transpose += OC::qualities[_quality][2];
      int32_t sample_c  = quantizer_.Process(pitch, root << 7, transpose);
      transpose += OC::qualities[_quality][3];
      int32_t sample_d  = quantizer_.Process(pitch, root << 7, transpose);

      //todo voicing for root note
      sample_b = OC::DAC::pitch_to_scaled_voltage_dac(DAC_CHANNEL_B, sample_b, octave + OC::voicing[_voicing][1] + OC::inversion[_inversion][1], OC::DAC::get_voltage_scaling(DAC_CHANNEL_B));
      sample_c = OC::DAC::pitch_to_scaled_voltage_dac(DAC_CHANNEL_C, sample_c, octave + OC::voicing[_voicing][2] + OC::inversion[_inversion][2], OC::DAC::get_voltage_scaling(DAC_CHANNEL_C));
      sample_d = OC::DAC::pitch_to_scaled_voltage_dac(DAC_CHANNEL_D, sample_d, octave + OC::voicing[_voicing][3] + OC::inversion[_inversion][3], OC::DAC::get_voltage_scaling(DAC_CHANNEL_D));
      
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
      clock_display_.Update(1, true);
    } else {
      clock_display_.Update(1, false);
    }
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

        *settings++ = CHORDS_SETTING_PROGRESSION;
        *settings++ = CHORDS_SETTING_CHORD_EDIT;
        *settings++ = CHORDS_SETTING_PLAYMODES;
        if (get_playmode() < _SH1)
          *settings++ = CHORDS_SETTING_DIRECTION;
        if (get_direction() == CHORDS_BROWNIAN)       
          *settings++ = CHORDS_SETTING_BROWNIAN_PROBABILITY;       
        *settings++ = CHORDS_SETTING_TRANSPOSE;
        *settings++ = CHORDS_SETTING_OCTAVE;
        *settings++ = CHORDS_SETTING_CV_SOURCE;
        *settings++ = CHORDS_SETTING_CHORDS_ADVANCE_TRIGGER_SOURCE;
        *settings++ = CHORDS_SETTING_TRIGGER_DELAY;
      }
      break;
      case MENU_CV_MAPPING: {

        *settings++ = CHORDS_SETTING_MASK_CV;
        // destinations:
        // hide root CV?
        if (get_scale(DUMMY) != OC::Scales::SCALE_NONE)  
          *settings++ = CHORDS_SETTING_ROOT_CV;
        else
           *settings++ = CHORDS_SETTING_MORE_DUMMY;

        *settings++ = CHORDS_SETTING_PROGRESSION_CV; 
        *settings++ = CHORDS_SETTING_CHORD_EDIT; 
        *settings++ = CHORDS_SETTING_NUM_CHORDS_CV;
        if (get_playmode() < _SH1)
          *settings++ = CHORDS_SETTING_DIRECTION_CV; 
        if (get_direction() == CHORDS_BROWNIAN)       
          *settings++ = CHORDS_SETTING_BROWNIAN_CV; 
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
  bool _octave_toggle;
  int last_scale_;
  uint16_t last_mask_;
  int32_t last_sample_;
  uint8_t display_num_chords_;
  bool chord_advance_last_;
  bool progression_advance_last_;
  int8_t active_chord_;
  int8_t progression_cnt_;
  int8_t progression_EoP_;
  bool chord_repeat_;
  int8_t active_progression_;
  int8_t menu_page_;
  bool chords_direction_;
  int8_t playmode_last_;
  int8_t progression_last_;
  int8_t num_chords_last_;

  util::TriggerDelay<OC::kMaxTriggerDelayTicks> trigger_delay_;
  braids::Quantizer quantizer_;
  OC::Input_Map input_map_;
  OC::DigitalInputDisplay clock_display_;
  OC::Chords chords_;
  
  int num_enabled_settings_;
  CHORDS_SETTINGS enabled_settings_[CHORDS_SETTING_LAST];
  
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

const char* const chords_slots[] = {
  "#1", "#2", "#3", "#4", "#5", "#6", "#7", "#8"
};

const char* const chord_playmodes[] = {
  "-", "SEQ+1", "SEQ+2", "SEQ+3", "TR3+1", "TR3+2", "TR3+3", "S+H#1", "S+H#2", "S+H#3", "S+H#4", "CV#1", "CV#2", "CV#3", "CV#4" 
};

SETTINGS_DECLARE(Chords, CHORDS_SETTING_LAST) {
  { OC::Scales::SCALE_SEMI, OC::Scales::SCALE_SEMI, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { 0, 0, 11, "root", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 }, 
  { 0, 0, OC::Chords::NUM_CHORD_PROGRESSIONS - 1, "progression", chords_slots, settings::STORAGE_TYPE_U8 },
  { 65535, 1, 65535, "scale  -->", NULL, settings::STORAGE_TYPE_U16 }, // mask
  { 0, 0, CHORDS_CV_SOURCE_LAST - 1, "CV source", OC::Strings::cv_input_names, settings::STORAGE_TYPE_U8 }, /// to do ..
  { CHORDS_ADVANCE_TRIGGER_SOURCE_TR2, 0, CHORDS_ADVANCE_TRIGGER_SOURCE_LAST - 1, "chords trg src", chords_advance_trigger_sources, settings::STORAGE_TYPE_U8 },
  { 0, 0, CHORDS_PLAYMODES_LAST - 1, "playmode", chord_playmodes, settings::STORAGE_TYPE_U8 },
  { 0, 0, CHORDS_DIRECTIONS_LAST - 1, "direction", OC::Strings::seq_directions, settings::STORAGE_TYPE_U8 },
  { 64, 0, 255, "-->brown prob", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, OC::kNumDelayTimes - 1, "TR1 delay", OC::Strings::trigger_delay_times, settings::STORAGE_TYPE_U8 },
  { 0, -5, 7, "transpose", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -4, 4, "octave", NULL, settings::STORAGE_TYPE_I8 },
  { 0, 0, OC::Chords::CHORDS_USER_LAST - 1, "chord:", chords_slots, settings::STORAGE_TYPE_U8 },
  { 0, 0, OC::Chords::CHORDS_USER_LAST - 1, "num.chords", NULL, settings::STORAGE_TYPE_U8 }, // progression 1
  { 0, 0, OC::Chords::CHORDS_USER_LAST - 1, "num.chords", NULL, settings::STORAGE_TYPE_U8 }, // progression 2
  { 0, 0, OC::Chords::CHORDS_USER_LAST - 1, "num.chords", NULL, settings::STORAGE_TYPE_U8 }, // progression 3
  { 0, 0, OC::Chords::CHORDS_USER_LAST - 1, "num.chords", NULL, settings::STORAGE_TYPE_U8 }, // progression 4
  { 0, 0, 0, "chords -->", NULL, settings::STORAGE_TYPE_U4 }, // = chord editor
  // CV
  { 0, 0, 4, "root CV      >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "mask CV      >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "transpose CV >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "octave CV    >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "quality CV   >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "voicing CV   >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "inversion CV >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "prg.slot# CV >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "direction CV >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "-->br.prb CV >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "num.chrds CV >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 0, "-", NULL, settings::STORAGE_TYPE_U4 }, // DUMMY 
  { 0, 0, 0, " ", NULL, settings::STORAGE_TYPE_U4 }  // MORE DUMMY  
};

class ChordQuantizer {
public:
  void Init() {
    cursor.Init(CHORDS_SETTING_SCALE, CHORDS_SETTING_LAST - 1);
    scale_editor.Init(false);
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

  // active progression #
  graphics.setPrintPos(106, 2);
  if (chords.poke_octave_toggle())
    graphics.print("+"); 
  else 
    graphics.print("#");
  graphics.print(chords.get_active_progression() + 0x1);

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
        menu::DrawMiniChord(menu::kDisplayWidth, list_item.y, chords.get_display_num_chords(), chords.active_chord());
        list_item.DrawNoValue<false>(value, attr);
        break;
      case CHORDS_SETTING_MORE_DUMMY:
        list_item.DrawNoValue<false>(value, attr);
        break;  
      case CHORDS_SETTING_CHORD_SLOT: 
        //special case:
        list_item.DrawValueMax(value, attr, chords.get_num_chords(chords.get_progression()));
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
    CONSTRAIN(value, OC::Scales::SCALE_SEMI, OC::Scales::NUM_SCALES - 1);
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
            if (chords.get_chord_slot() > chords.get_num_chords(chords.get_progression()))
              chords.set_chord_slot(chords.get_num_chords(chords.get_progression()));
            break;
          case CHORDS_SETTING_DIRECTION:
          case CHORDS_SETTING_PLAYMODES:
          // show options, or don't:
            chords.update_enabled_settings();
            chords_state.cursor.AdjustEnd(chords.num_enabled_settings() - 1);
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
  
  if (chords.get_menu_page() == MENU_PARAMETERS) {

    if (chords.octave_toggle())
      chords.change_value(CHORDS_SETTING_OCTAVE, 1);
    else 
      chords.change_value(CHORDS_SETTING_OCTAVE, -1);
  } 
  else  {
    chords.set_menu_page(MENU_PARAMETERS);
    chords.update_enabled_settings();
    chords_state.cursor.set_editing(false);
  }
}

void CHORDS_lowerButton() {
  // go the CV mapping

  if (!chords_state.chord_editor.active() && !chords_state.scale_editor.active()) {  

    uint8_t _menu_page = chords.get_menu_page();
  
    switch (_menu_page) {
  
      case MENU_PARAMETERS:
        _menu_page = MENU_CV_MAPPING;
      break;
      default:
        _menu_page = MENU_PARAMETERS;
      break;
    }
    
    chords.set_menu_page(_menu_page);
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
      chords_state.chord_editor.Edit(&chords, chords.get_chord_slot(), chords.get_num_chords(chords.get_progression()), chords.get_progression());
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

  if (chords_state.left_encoder_value != chords.get_scale(DUMMY) || chords_state.left_encoder_value == OC::Scales::SCALE_SEMI) { 
    chords.set_scale(chords_state.left_encoder_value);
    // hide/show root
    chords.update_enabled_settings();
  }
}

void CHORDS_leftButtonLong() {
  // todo
}

void CHORDS_downButtonLong() {
  chords.clear_CV_mapping();
  chords_state.cursor.set_editing(false);
}

void CHORDS_upButtonLong() {
  // screensaver short cut
}

static const weegfx::coord_t chords_kBottom = 60;

inline int32_t chords_render_pitch(int32_t pitch, weegfx::coord_t x, weegfx::coord_t width) {
  
  CONSTRAIN(pitch, 0, 120 << 7);
  int32_t octave = pitch / (12 << 7);
  pitch -= (octave * 12 << 7);
  graphics.drawHLine(x, chords_kBottom - ((pitch * 4) >> 7), width << 1);
  return octave;
}

void Chords::RenderScreensaver(weegfx::coord_t start_x) const {

  int _active_chord = active_chord();
  int _num_progression = get_active_progression();
  int _num_chords = get_display_num_chords(); 
  int x = start_x + 4;
  int y = 42; 

  // todo: CV
  for (int j = 0; j <= _num_chords; j++) {

    if (j == _active_chord)
      menu::DrawChord(x + (j << 4) + 1, y, 6, j, _num_progression);
    else  
      menu::DrawChord(x + (j << 4) + 2, y, 4, j, _num_progression);
  }
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
