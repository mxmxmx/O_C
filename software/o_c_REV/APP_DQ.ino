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
const uint8_t NUM_SCALE_SLOTS = 4;

enum DQ_ChannelSetting {
  DQ_CHANNEL_SETTING_SCALE1,
  DQ_CHANNEL_SETTING_SCALE2,
  DQ_CHANNEL_SETTING_SCALE3,
  DQ_CHANNEL_SETTING_SCALE4,
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
  DQ_CHANNEL_SETTING_AUX_OUTPUT,
  DQ_CHANNEL_SETTING_PULSEWIDTH,
  DQ_CHANNEL_SETTING_AUX_OCTAVE,
  DQ_CHANNEL_SETTING_AUX_CV_DEST,
  DQ_CHANNEL_SETTING_LAST
};

enum DQ_ChannelTriggerSource {
  DQ_CHANNEL_TRIGGER_TR1,
  DQ_CHANNEL_TRIGGER_TR2,
  DQ_CHANNEL_TRIGGER_TR3,
  DQ_CHANNEL_TRIGGER_TR4,
  DQ_CHANNEL_TRIGGER_CONTINUOUS,
  DQ_CHANNEL_TRIGGER_LAST
};

enum DQ_ChannelSource {
  DQ_CHANNEL_SOURCE_CV1,
  DQ_CHANNEL_SOURCE_CV2,
  DQ_CHANNEL_SOURCE_CV3,
  DQ_CHANNEL_SOURCE_CV4,
  DQ_CHANNEL_SOURCE_LAST
};

enum DQ_AUX_MODE {
  DQ_GATE,
  DQ_COPY,
  DQ_ASR,
  DQ_AUX_MODE_LAST
};

enum DQ_CV_DEST {
  DQ_DEST_NONE,
  DQ_DEST_SCALE_SLOT,
  DQ_DEST_ROOT,
  DQ_DEST_OCTAVE,
  DQ_DEST_TRANSPOSE,
  DQ_DEST_MASK,
  DQ_DEST_LAST
};

class DQ_QuantizerChannel : public settings::SettingsBase<DQ_QuantizerChannel, DQ_CHANNEL_SETTING_LAST> {
public:

  int get_scale(uint8_t selected_scale_slot_) const {

    switch(selected_scale_slot_) {
      
       case 0:
        return values_[DQ_CHANNEL_SETTING_SCALE1];
       break;
       case 1:
        return values_[DQ_CHANNEL_SETTING_SCALE2];
       break;
       case 2:
        return values_[DQ_CHANNEL_SETTING_SCALE3];
       break;
       case 3:
        return values_[DQ_CHANNEL_SETTING_SCALE4];
       break;
       default:
        return 0;
       break;        
    }
  } 

  int get_scale_select() const {
    return values_[DQ_CHANNEL_SETTING_SCALE_SEQ];
  }

  int get_scale_seq_mode() const {
    return values_[DQ_CHANNEL_SETTING_SEQ_MODE];
  }

  int get_display_scale() const {
    return display_scale_slot_;
  }

  int get_display_root() const {
    return display_root_;
  }

  void set_scale_at_slot(int scale, uint16_t mask, uint8_t scale_slot) {

    if (scale != get_scale(scale_slot) || mask != get_mask(scale_slot)) {
 
      const OC::Scale &scale_def = OC::Scales::GetScale(scale);
   
      if (0 == (mask & ~(0xffff << scale_def.num_notes)))
        mask |= 0x1;
      switch (scale_slot) {  
        case 1:
        apply_value(DQ_CHANNEL_SETTING_MASK2, mask); 
        apply_value(DQ_CHANNEL_SETTING_SCALE2, scale);
        break;
        case 2:
        apply_value(DQ_CHANNEL_SETTING_MASK3, mask); 
        apply_value(DQ_CHANNEL_SETTING_SCALE3, scale);
        break;
        case 3:
        apply_value(DQ_CHANNEL_SETTING_MASK4, mask); 
        apply_value(DQ_CHANNEL_SETTING_SCALE4, scale);
        break;
        default:
        apply_value(DQ_CHANNEL_SETTING_MASK1, mask); 
        apply_value(DQ_CHANNEL_SETTING_SCALE1, scale);
        break;
      }
    } 
  }

  void set_scale(int scale, uint16_t mask, uint8_t scale_slot) {

    if (scale != get_scale(scale_slot) || mask != get_mask(scale_slot)) {
 
      const OC::Scale &scale_def = OC::Scales::GetScale(scale);
   
      if (0 == (mask & ~(0xffff << scale_def.num_notes)))
        mask |= 0x1;
      switch (scale_slot) {  
        case 1:
        apply_value(DQ_CHANNEL_SETTING_MASK2, mask); 
        apply_value(DQ_CHANNEL_SETTING_SCALE2, scale);
        break;
        case 2:
        apply_value(DQ_CHANNEL_SETTING_MASK3, mask); 
        apply_value(DQ_CHANNEL_SETTING_SCALE3, scale);
        break;
        case 3:
        apply_value(DQ_CHANNEL_SETTING_MASK4, mask); 
        apply_value(DQ_CHANNEL_SETTING_SCALE4, scale);
        break;
        default:
        apply_value(DQ_CHANNEL_SETTING_MASK1, mask); 
        apply_value(DQ_CHANNEL_SETTING_SCALE1, scale);
        break;
      }
    }
  }

  int get_root() const {
    return values_[DQ_CHANNEL_SETTING_ROOT];
  }

  uint16_t get_mask(uint8_t selected_scale_slot_) const { 

    switch(selected_scale_slot_) {
      
      case 0:  
        return values_[DQ_CHANNEL_SETTING_MASK1];
      break;
      case 1:  
        return values_[DQ_CHANNEL_SETTING_MASK2];
      break;
      case 2:  
        return values_[DQ_CHANNEL_SETTING_MASK3];
      break;
      case 3:  
        return values_[DQ_CHANNEL_SETTING_MASK4];
      break;
      default:
        return 0;
      break;
    }
  }

  uint16_t get_rotated_mask(uint8_t selected_scale_slot_) const { 
      return last_mask_[selected_scale_slot_];
  }
  
  DQ_ChannelSource get_source() const {
    return static_cast<DQ_ChannelSource>(values_[DQ_CHANNEL_SETTING_SOURCE]);
  }

  DQ_ChannelTriggerSource get_trigger_source() const {
    return static_cast<DQ_ChannelTriggerSource>(values_[DQ_CHANNEL_SETTING_TRIGGER]);
  }

  OC::DigitalInput get_digital_input() const {
    return static_cast<OC::DigitalInput>(values_[DQ_CHANNEL_SETTING_TRIGGER]);
  }

  uint8_t get_aux_cv_dest() const {
    return values_[DQ_CHANNEL_SETTING_AUX_CV_DEST];
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

  int get_aux_mode() const {
    return values_[DQ_CHANNEL_SETTING_AUX_OUTPUT];
  }

  int get_aux_octave() const {
    return values_[DQ_CHANNEL_SETTING_AUX_OCTAVE];
  }

  int get_pulsewidth() const {
    return values_[DQ_CHANNEL_SETTING_PULSEWIDTH];
  }

  void Init(DQ_ChannelSource source, DQ_ChannelTriggerSource trigger_source) {
    InitDefaults();
    apply_value(DQ_CHANNEL_SETTING_SOURCE, source);
    apply_value(DQ_CHANNEL_SETTING_TRIGGER, trigger_source);

    force_update_ = true;
    instant_update_ = false;
    for (int i = 0; i < NUM_SCALE_SLOTS; i++) {
      last_scale_[i] = -1;
      last_mask_[i] = 0;
    }
    last_sample_ = 0;
    clock_ = 0;

    trigger_delay_.Init();
    quantizer_.Init();
    update_scale(true, 0, false);
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

  inline void Update(uint32_t triggers, DAC_CHANNEL dac_channel, DAC_CHANNEL aux_channel) {

    ticks_++;
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

    if (triggered) {
      channel_frequency_in_ticks_ = ticks_;
      ticks_ = 0x0;
      gate_state_ = ON; 
    }
   
    if (get_scale_seq_mode()) {
        // to do, don't hardcode .. 
      uint8_t _advance_trig = (dac_channel == DAC_CHANNEL_A) ? digitalReadFast(TR2) : digitalReadFast(TR4);
      if (_advance_trig < scale_advance_state_) 
        scale_advance_ = true;
          
      scale_advance_state_ = _advance_trig;     
    }
    else if (prev_scale_slot_ != get_scale_select()) {
      active_scale_slot_ = get_scale_select();
      prev_scale_slot_ = active_scale_slot_;
    }
          
    if (scale_advance_) {
      scale_sequence_cnt_++;
      active_scale_slot_ = get_scale_select() + (scale_sequence_cnt_ % (get_scale_seq_mode()+1));
      
      if (active_scale_slot_ >= NUM_SCALE_SLOTS)
        active_scale_slot_ -= NUM_SCALE_SLOTS;
      scale_advance_ = false;
      schedule_scale_update_ = true;
    }

    bool update = continous || triggered;
    if (get_aux_cv_dest() != DQ_DEST_MASK && update_scale(forced_update, active_scale_slot_, false) && instant_update_ == true)
      update = true;
       
    int32_t sample = last_sample_;
    int32_t history_sample = 0;
    uint8_t aux_mode = get_aux_mode();

    if (update) {
        
      int32_t transpose = get_transpose();
      int32_t pitch = quantizer_.enabled()
          ? OC::ADC::raw_pitch_value(static_cast<ADC_CHANNEL>(source))
          : OC::ADC::pitch_value(static_cast<ADC_CHANNEL>(source));

      int root = get_root();
      int octave = get_octave();
      int channel_id = (dac_channel == DAC_CHANNEL_A) ? 1 : 3; // hardcoded to use CV2, CV4, for now
      display_scale_slot_ = prev_scale_slot_ = active_scale_slot_;
      
      switch(get_aux_cv_dest()) {
      
        case DQ_DEST_NONE:
        break;
        case DQ_DEST_SCALE_SLOT:
          display_scale_slot_ += (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) + 255) >> 9;   
          CONSTRAIN(display_scale_slot_, 0, NUM_SCALE_SLOTS-1);
          schedule_scale_update_ = true;
        break;
        case DQ_DEST_ROOT:
            root += (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) * 12 + 2047) >> 12;
            CONSTRAIN(root, 0, 11);
        break;
        case DQ_DEST_MASK:
            update_scale(true, active_scale_slot_, (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) + 127) >> 8);
            schedule_scale_update_ = false;
        break;
        case DQ_DEST_OCTAVE:
          octave += (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) * 12 + 2047) >> 12;
          CONSTRAIN(octave, -4, 4);
        break;
        case DQ_DEST_TRANSPOSE:
          transpose += (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) * 12 + 2047) >> 12;
          CONSTRAIN(transpose, -12, 12);
        break;
        default:
        break;
      }
      // CV    
      display_root_ = root;
      
      if (schedule_scale_update_) {
        update_scale(true, display_scale_slot_, false);
        schedule_scale_update_ = false;
      }  
       
      const int32_t quantized = quantizer_.Process(pitch, root << 7, transpose);
      sample = OC::DAC::pitch_to_dac(dac_channel, quantized, octave);
      history_sample = quantized + ((OC::DAC::kOctaveZero + octave) * 12 << 7);  
     
      if (aux_mode == DQ_COPY) {
        int octave_offset = octave + get_aux_octave();
        gate_state_ = OC::DAC::pitch_to_dac(aux_channel, quantized, octave_offset);
      }
      else if (aux_mode == DQ_ASR) {
        // to do ... more settings
        int octave_offset = octave + get_aux_octave();
        const int32_t quantized_aux = quantizer_.Process(last_raw_sample_, root << 7, transpose);
        gate_state_ = OC::DAC::pitch_to_dac(aux_channel, quantized_aux, octave_offset);
        last_raw_sample_ = pitch;
      }
    }

    // 
    bool changed = last_sample_ != sample;
    
    if (changed) {
      
      MENU_REDRAW = 1;
      last_sample_ = sample;
      
      if (continous && aux_mode == DQ_GATE) {
        gate_state_ = ON;
        ticks_ = 0x0;
      } 
    }
    
    // pulsewidth / gate outputs: 

    switch (aux_mode) {

      case DQ_COPY: 
      case DQ_ASR:
      break;
      case DQ_GATE:
      { 
      if (gate_state_) { 
           
            // pulsewidth setting -- 
            int16_t _pulsewidth = get_pulsewidth();
    
            if (_pulsewidth || continous) { // don't echo
    
                bool _gates = false;
             
                if (_pulsewidth == PULSEW_MAX)
                  _gates = true;
                // we-can't-echo-hack  
                if (continous && !_pulsewidth)
                  _pulsewidth = 0x1;
                // CV?
                /*
                if (get_pulsewidth_cv_source()) {
    
                  _pulsewidth += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_pulsewidth_cv_source() - 1)) + 8) >> 3; 
                  if (!_gates)          
                    CONSTRAIN(_pulsewidth, 1, PULSEW_MAX);
                  else // CV for 50% duty cycle: 
                    CONSTRAIN(_pulsewidth, 1, (PULSEW_MAX<<1) - 55);  // incl margin, max < 2x mult. see below 
                }
                */
                // recalculate (in ticks), if new pulsewidth setting:
                if (prev_pulsewidth_ != _pulsewidth || ! ticks_) {
                  
                    if (!_gates) {
                      int32_t _fraction = signed_multiply_32x16b(TICKS_TO_MS, static_cast<int32_t>(_pulsewidth)); // = * 0.6667f
                      _fraction = signed_saturate_rshift(_fraction, 16, 0);
                      pulse_width_in_ticks_  = (_pulsewidth << 4) + _fraction;
                    }
                    else { // put out gates/half duty cycle:

                      pulse_width_in_ticks_ = channel_frequency_in_ticks_ >> 1;
                      
                      if (_pulsewidth != PULSEW_MAX) { // CV?
                        pulse_width_in_ticks_ = signed_multiply_32x16b(static_cast<int32_t>(_pulsewidth) << 8, pulse_width_in_ticks_); // 
                        pulse_width_in_ticks_ = signed_saturate_rshift(pulse_width_in_ticks_, 16, 0);
                      }
                    }
                }
                prev_pulsewidth_ = _pulsewidth;
                
                // limit pulsewidth, if approaching half duty cycle:
                if (!_gates && pulse_width_in_ticks_ >= channel_frequency_in_ticks_>>1) 
                  pulse_width_in_ticks_ = (channel_frequency_in_ticks_ >> 1) | 1u;
                  
                // turn off output? 
                if (ticks_ >= pulse_width_in_ticks_) 
                  gate_state_ = OFF;
                else // keep on 
                  gate_state_ = ON; 
             }
             else {
                // we simply echo the pulsewidth:
                 gate_state_ = OC::DigitalInputs::read_immediate(get_digital_input()) ? ON : OFF;
             }   
         }  
      } 
      break;
      default:
      break;
    }
    
    OC::DAC::set(dac_channel, sample);
    OC::DAC::set(aux_channel, gate_state_);

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

  uint16_t get_scale_mask(uint8_t scale_select) const {
    return get_mask(scale_select);
  }

  void update_scale_mask(uint16_t mask, uint8_t scale_select) {

    switch (scale_select) {
      case 0:  
        apply_value(DQ_CHANNEL_SETTING_MASK1, mask); 
      break;
      case 1:  
        apply_value(DQ_CHANNEL_SETTING_MASK2, mask); 
      break;
      case 2:  
        apply_value(DQ_CHANNEL_SETTING_MASK3, mask); 
      break;
      case 3: 
        apply_value(DQ_CHANNEL_SETTING_MASK4, mask); 
      break;
      default:
      break;
    }
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

    switch(get_scale_select()) {

      case 0:
      *settings++ = DQ_CHANNEL_SETTING_SCALE1;
      break;
      case 1:
      *settings++ = DQ_CHANNEL_SETTING_SCALE2;
      break;
      case 2:
      *settings++ = DQ_CHANNEL_SETTING_SCALE3;
      break;
      case 3:
      *settings++ = DQ_CHANNEL_SETTING_SCALE4;
      break;
      default:
      break;
    }
    if (OC::Scales::SCALE_NONE != get_scale(get_scale_select())) {
      
      *settings++ = DQ_CHANNEL_SETTING_SCALE_SEQ;

      switch(get_scale_select()) {
        case 0:  
         *settings++ = DQ_CHANNEL_SETTING_MASK1; 
        break;
         case 1:  
         *settings++ = DQ_CHANNEL_SETTING_MASK2; 
        break;
         case 2:  
         *settings++ = DQ_CHANNEL_SETTING_MASK3; 
        break;
         case 3:  
         *settings++ = DQ_CHANNEL_SETTING_MASK4; 
        break;
        default:
        break;
      }
      *settings++ = DQ_CHANNEL_SETTING_SEQ_MODE;
      *settings++ = DQ_CHANNEL_SETTING_ROOT;
    }
    *settings++ = DQ_CHANNEL_SETTING_SOURCE;
    *settings++ = DQ_CHANNEL_SETTING_AUX_CV_DEST;
    *settings++ = DQ_CHANNEL_SETTING_TRIGGER;
    
    if (DQ_CHANNEL_TRIGGER_CONTINUOUS != get_trigger_source()) 
      *settings++ = DQ_CHANNEL_SETTING_DELAY;

    *settings++ = DQ_CHANNEL_SETTING_OCTAVE;
    *settings++ = DQ_CHANNEL_SETTING_TRANSPOSE;
    *settings++ = DQ_CHANNEL_SETTING_AUX_OUTPUT;
    
    switch(get_aux_mode()) {
      
      case DQ_GATE:
        *settings++ = DQ_CHANNEL_SETTING_PULSEWIDTH;
      break;
      case DQ_COPY:
        *settings++ = DQ_CHANNEL_SETTING_AUX_OCTAVE;
      break;
      case DQ_ASR:
        *settings++ = DQ_CHANNEL_SETTING_AUX_OCTAVE; // to do
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
  int last_scale_[NUM_SCALE_SLOTS];
  uint16_t last_mask_[NUM_SCALE_SLOTS];
  int scale_sequence_cnt_;
  int active_scale_slot_;
  int display_scale_slot_;
  int display_root_;
  int prev_scale_slot_;
  int8_t scale_advance_;
  int8_t scale_advance_state_;
  bool schedule_scale_update_;
  int32_t last_sample_;
  int32_t last_raw_sample_;
  uint8_t clock_;
  uint16_t gate_state_;
  uint8_t prev_pulsewidth_;
  uint32_t ticks_;
  uint32_t channel_frequency_in_ticks_;
  uint32_t pulse_width_in_ticks_;

  util::TriggerDelay<OC::kMaxTriggerDelayTicks> trigger_delay_;
  braids::Quantizer quantizer_;
  OC::DigitalInputDisplay trigger_display_;

  int num_enabled_settings_;
  DQ_ChannelSetting enabled_settings_[DQ_CHANNEL_SETTING_LAST];

  OC::vfx::ScrollingHistory<int32_t, 5> scrolling_history_;

  bool update_scale(bool force, uint8_t scale_select, int32_t mask_rotate) {
      
    const int scale = get_scale(scale_select);
    uint16_t mask = get_mask(scale_select);
    
    if (mask_rotate)
      mask = OC::ScaleEditor<DQ_QuantizerChannel>::RotateMask(mask, OC::Scales::GetScale(scale).num_notes, mask_rotate);
      
    if (force || (last_scale_[scale_select] != scale || last_mask_[scale_select] != mask)) {
      last_scale_[scale_select] = scale;
      last_mask_[scale_select] = mask;
      quantizer_.Configure(OC::Scales::GetScale(scale), mask);
      return true;
    } else {
      return false;
    }
  }
};

const char* const dq_channel_trigger_sources[DQ_CHANNEL_TRIGGER_LAST] = {
  "TR1", "TR2", "TR3", "TR4", "cont"
};

const char* const dq_channel_input_sources[DQ_CHANNEL_SOURCE_LAST] = {
  "CV1", "CV2", "CV3", "CV4"
};

const char* const dq_seq_scales[] = {
  "s#1", "s#2", "s#3", "s#4"
};

const char* const dq_seq_modes[] = {
  "-", "TR+1", "TR+2", "TR+3"
};

const char* const dq_aux_outputs[] = {
  "gate", "copy", "asr"
};

const char* const dq_aux_cv_dest[] = {
  "-", "scl#", "root", "oct", "trns", "mask"
};

SETTINGS_DECLARE(DQ_QuantizerChannel, DQ_CHANNEL_SETTING_LAST) {
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { 0, 0, 11, "root", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 },
  { 0, 0, NUM_SCALE_SLOTS - 1, "scale #", dq_seq_scales, settings::STORAGE_TYPE_U4  },
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 },
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 },
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 },
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 },
  { 0, 0, 3, "seq_mode", dq_seq_modes, settings::STORAGE_TYPE_U4 },
  { DQ_CHANNEL_SOURCE_CV1, DQ_CHANNEL_SOURCE_CV1, DQ_CHANNEL_SOURCE_LAST - 1, "CV source", dq_channel_input_sources, settings::STORAGE_TYPE_U4 },
  { DQ_CHANNEL_TRIGGER_CONTINUOUS, 0, DQ_CHANNEL_TRIGGER_LAST - 1, "trigger source", dq_channel_trigger_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, OC::kNumDelayTimes - 1, "--> latency", OC::Strings::trigger_delay_times, settings::STORAGE_TYPE_U4 },
  { 0, -5, 7, "transpose", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -4, 4, "octave", NULL, settings::STORAGE_TYPE_I8 },
  { 0, 0, DQ_AUX_MODE_LAST-1, "aux.output", dq_aux_outputs, settings::STORAGE_TYPE_U4 },
  { 25, 0, PULSEW_MAX, "--> pw", OC::Strings::pulsewidth_ms, settings::STORAGE_TYPE_U8 },
  { 0, -5, 5, "--> aux +/-", NULL, settings::STORAGE_TYPE_I8 }, // aux octave
  { 0, 0, DQ_DEST_LAST-1, "CV aux.", dq_aux_cv_dest, settings::STORAGE_TYPE_U8 }
};

// WIP refactoring to better encapsulate and for possible app interface change
class DualQuantizer {
public:
  void Init() {
    selected_channel = 0;
    cursor.Init(DQ_CHANNEL_SETTING_SCALE1, DQ_CHANNEL_SETTING_LAST - 1);
    scale_editor.Init_seq();
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

  dq_quantizer_channels[0].Update(triggers, DAC_CHANNEL_A, DAC_CHANNEL_C);
  dq_quantizer_channels[1].Update(triggers, DAC_CHANNEL_B, DAC_CHANNEL_D);
}

void DQ_loop() {
}

void DQ_menu() {

  menu::DualTitleBar::Draw();
  
  for (int i = 0, x = 0; i < NUMCHANNELS; ++i, x += 21) {

    const DQ_QuantizerChannel &channel = dq_quantizer_channels[i];
    menu::DualTitleBar::SetColumn(i);
    menu::DualTitleBar::DrawGateIndicator(i, channel.getTriggerState());
    
    graphics.movePrintPos(5, 0);
    graphics.print((char)('A' + i));
    graphics.movePrintPos(2, 0);
    graphics.print('#');
    graphics.print(channel.get_display_scale() + 1);
    graphics.movePrintPos(12, 0);
    graphics.print(OC::Strings::note_names[channel.get_display_root()]);
    int octave = channel.get_octave();
    if (octave)
      graphics.pretty_print(octave);
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
      case DQ_CHANNEL_SETTING_SCALE1:
      case DQ_CHANNEL_SETTING_SCALE2:
      case DQ_CHANNEL_SETTING_SCALE3:
      case DQ_CHANNEL_SETTING_SCALE4:
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
        menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, channel.get_rotated_mask(channel.get_display_scale()), OC::Scales::GetScale(channel.get_scale(channel.get_display_scale())).num_notes); 
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
      if (DQ_CHANNEL_SETTING_MASK1 != setting || DQ_CHANNEL_SETTING_MASK2 != setting || DQ_CHANNEL_SETTING_MASK3 != setting || DQ_CHANNEL_SETTING_MASK4 != setting) { 
        if (selected.change_value(setting, event.value))
          selected.force_update();

        switch (setting) {
          case DQ_CHANNEL_SETTING_SCALE1:
          case DQ_CHANNEL_SETTING_SCALE2:
          case DQ_CHANNEL_SETTING_SCALE3:
          case DQ_CHANNEL_SETTING_SCALE4:
          case DQ_CHANNEL_SETTING_SCALE_SEQ:
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
      int scale = selected.get_scale(selected.get_scale_select());
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
  dq_state.selected_channel = (dq_state.selected_channel + 1) & 1u;
  DQ_QuantizerChannel &selected = dq_quantizer_channels[dq_state.selected_channel];
  dq_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
}

void DQ_leftButtonLong() {
  DQ_QuantizerChannel &selected_channel = dq_quantizer_channels[dq_state.selected_channel];
  int scale = selected_channel.get_scale(selected_channel.get_scale_select());
  int mask = selected_channel.get_mask(selected_channel.get_scale_select());
  int root = selected_channel.get_root();

  dq_quantizer_channels[(~dq_state.selected_channel)&1u].apply_value(DQ_CHANNEL_SETTING_ROOT, root);
   
  for (int i = 0; i < NUM_SCALE_SLOTS; ++i) {
    for (int j = 0; j < NUMCHANNELS; ++j)
      dq_quantizer_channels[j].set_scale(scale, mask, i);
  }
}

void DQ_downButtonLong() {
   // to do...
   // CV menu
   //for (int i = 0; i < NUMCHANNELS; ++i) 
   //  dq_quantizer_channels[i].instant_update();
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
