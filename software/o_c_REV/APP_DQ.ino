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

#ifdef BUCHLA_4U
 #define DQ_OFFSET_X 22
#else
 #define DQ_OFFSET_X 47
#endif

const uint8_t NUMCHANNELS = 2;
const uint8_t NUM_SCALE_SLOTS = 4;

enum DQ_ChannelSetting {
  DQ_CHANNEL_SETTING_SCALE1,
  DQ_CHANNEL_SETTING_SCALE2,
  DQ_CHANNEL_SETTING_SCALE3,
  DQ_CHANNEL_SETTING_SCALE4,
  DQ_CHANNEL_SETTING_ROOT1,
  DQ_CHANNEL_SETTING_ROOT2,
  DQ_CHANNEL_SETTING_ROOT3,
  DQ_CHANNEL_SETTING_ROOT4,
  DQ_CHANNEL_SETTING_SCALE_SEQ,
  DQ_CHANNEL_SETTING_MASK1,
  DQ_CHANNEL_SETTING_MASK2,
  DQ_CHANNEL_SETTING_MASK3,
  DQ_CHANNEL_SETTING_MASK4,
  DQ_CHANNEL_SETTING_SEQ_MODE,
  DQ_CHANNEL_SETTING_SOURCE,
  DQ_CHANNEL_SETTING_TRIGGER,
  DQ_CHANNEL_SETTING_DELAY,
  DQ_CHANNEL_SETTING_TRANSPOSE1,
  DQ_CHANNEL_SETTING_TRANSPOSE2,
  DQ_CHANNEL_SETTING_TRANSPOSE3,
  DQ_CHANNEL_SETTING_TRANSPOSE4,
  DQ_CHANNEL_SETTING_OCTAVE,
  DQ_CHANNEL_SETTING_AUX_OUTPUT,
  DQ_CHANNEL_SETTING_PULSEWIDTH,
  DQ_CHANNEL_SETTING_AUX_OCTAVE,
  DQ_CHANNEL_SETTING_AUX_CV_DEST,
  DQ_CHANNEL_SETTING_TURING_LENGTH,
  DQ_CHANNEL_SETTING_TURING_PROB,
  DQ_CHANNEL_SETTING_TURING_CV_SOURCE,
  DQ_CHANNEL_SETTING_TURING_RANGE,
  DQ_CHANNEL_SETTING_TURING_TRIG_OUT,
  DQ_CHANNEL_SETTING_LAST
};

enum DQ_ChannelTriggerSource {
  DQ_CHANNEL_TRIGGER_TR1,
  DQ_CHANNEL_TRIGGER_TR2,
  DQ_CHANNEL_TRIGGER_TR3,
  DQ_CHANNEL_TRIGGER_TR4,
  DQ_CHANNEL_TRIGGER_CONTINUOUS_UP,
  DQ_CHANNEL_TRIGGER_CONTINUOUS_DOWN,
  DQ_CHANNEL_TRIGGER_LAST
};

enum DQ_ChannelSource {
  DQ_CHANNEL_SOURCE_CV1,
  DQ_CHANNEL_SOURCE_CV2,
  DQ_CHANNEL_SOURCE_CV3,
  DQ_CHANNEL_SOURCE_CV4,
  DQ_CHANNEL_SOURCE_TURING,
  DQ_CHANNEL_SOURCE_LOGISTIC_MAP,
  DQ_CHANNEL_SOURCE_BYTEBEAT,
  DQ_CHANNEL_SOURCE_INT_SEQ,
  DQ_CHANNEL_SOURCE_LAST
};

enum DQ_AUX_MODE {
  DQ_GATE,
  DQ_COPY,
  DQ_ASR,
  DQ_AUX_MODE_LAST
};

enum TRIG_AUX_MODE {
  DQ_ECHO,
  DQ_LSB,
  DQ_CHANGE,
  DQ_TRIG_AUX_LAST
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

enum DQ_SLOTS {
  SLOT1,
  SLOT2,
  SLOT3,
  SLOT4,
  LAST_SLOT
};

class DQ_QuantizerChannel : public settings::SettingsBase<DQ_QuantizerChannel, DQ_CHANNEL_SETTING_LAST> {
public:

  int get_scale(uint8_t selected_scale_slot_) const {

    switch(selected_scale_slot_) {
   
       case SLOT2:
        return values_[DQ_CHANNEL_SETTING_SCALE2];
       break;
       case SLOT3:
        return values_[DQ_CHANNEL_SETTING_SCALE3];
       break;
       case SLOT4:
        return values_[DQ_CHANNEL_SETTING_SCALE4];
       break;
       case SLOT1:
       default:
        return values_[DQ_CHANNEL_SETTING_SCALE1];
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

  void set_scale_at_slot(int scale, uint16_t mask, int root, int transpose, uint8_t scale_slot) {

    if (scale != get_scale(scale_slot) || mask != get_mask(scale_slot) || root != root_last_ || transpose != transpose_last_) {
 
      const OC::Scale &scale_def = OC::Scales::GetScale(scale);
      root_last_ = root;
      transpose_last_ = transpose;
      
      if (0 == (mask & ~(0xffff << scale_def.num_notes)))
        mask |= 0x1;
      switch (scale_slot) {  
        case SLOT2:
        apply_value(DQ_CHANNEL_SETTING_MASK2, mask); 
        apply_value(DQ_CHANNEL_SETTING_SCALE2, scale);
        apply_value(DQ_CHANNEL_SETTING_ROOT2, root);
        apply_value(DQ_CHANNEL_SETTING_TRANSPOSE2, transpose);
        break;
        case SLOT3:
        apply_value(DQ_CHANNEL_SETTING_MASK3, mask); 
        apply_value(DQ_CHANNEL_SETTING_SCALE3, scale);
        apply_value(DQ_CHANNEL_SETTING_ROOT3, root);
        apply_value(DQ_CHANNEL_SETTING_TRANSPOSE3, transpose);
        break;
        case SLOT4:
        apply_value(DQ_CHANNEL_SETTING_MASK4, mask); 
        apply_value(DQ_CHANNEL_SETTING_SCALE4, scale);
        apply_value(DQ_CHANNEL_SETTING_ROOT4, root);
        apply_value(DQ_CHANNEL_SETTING_TRANSPOSE4, transpose);
        break;
        case SLOT1:
        default:
        apply_value(DQ_CHANNEL_SETTING_MASK1, mask); 
        apply_value(DQ_CHANNEL_SETTING_SCALE1, scale);
        apply_value(DQ_CHANNEL_SETTING_ROOT1, root);
        apply_value(DQ_CHANNEL_SETTING_TRANSPOSE1, transpose);
        break;
      }
    }
  }

  int get_root(uint8_t selected_scale_slot_) const {

    switch(selected_scale_slot_) {
      
       case SLOT2:
        return values_[DQ_CHANNEL_SETTING_ROOT2];
       break;
       case SLOT3:
        return values_[DQ_CHANNEL_SETTING_ROOT3];
       break;
       case SLOT4:
        return values_[DQ_CHANNEL_SETTING_ROOT4];
       break;
       case SLOT1:
       default:
        return values_[DQ_CHANNEL_SETTING_ROOT1];
       break;        
    }
  }

  uint16_t get_mask(uint8_t selected_scale_slot_) const { 

    switch(selected_scale_slot_) {
      
      case SLOT2:
        return values_[DQ_CHANNEL_SETTING_MASK2];
      break;
      case SLOT3:  
        return values_[DQ_CHANNEL_SETTING_MASK3];
      break;
      case SLOT4:  
        return values_[DQ_CHANNEL_SETTING_MASK4];
      break;
      case SLOT1:  
      default:
        return values_[DQ_CHANNEL_SETTING_MASK1];
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

  int get_transpose(uint8_t selected_scale_slot_) const {
    
    switch(selected_scale_slot_) {
      
      case SLOT2:
        return values_[DQ_CHANNEL_SETTING_TRANSPOSE2];
      break;
      case SLOT3:  
        return values_[DQ_CHANNEL_SETTING_TRANSPOSE3];
      break;
      case SLOT4:   
        return values_[DQ_CHANNEL_SETTING_TRANSPOSE4];
      break;
      case SLOT1:
      default:
        return values_[DQ_CHANNEL_SETTING_TRANSPOSE1];
      break;
    }
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

  uint8_t get_turing_length() const {
    return values_[DQ_CHANNEL_SETTING_TURING_LENGTH];
  }

  uint8_t get_turing_display_length() const {
    return turing_display_length_;
  }

  uint8_t get_turing_range() const {
    return values_[DQ_CHANNEL_SETTING_TURING_RANGE];
  }

  uint8_t get_turing_probability() const {
    return values_[DQ_CHANNEL_SETTING_TURING_PROB];
  }
 
  uint8_t get_turing_CV() const {
    return values_[DQ_CHANNEL_SETTING_TURING_CV_SOURCE];
  }

  uint8_t get_turing_trig_out() const {
    return values_[DQ_CHANNEL_SETTING_TURING_TRIG_OUT];
  }

  uint32_t get_shift_register() const {
    return turing_machine_.get_shift_register();
  }

  void clear_dest() {
    // ...
    schedule_mask_rotate_ = 0x0;
    continuous_offset_ = 0x0;
    prev_transpose_cv_ = 0x0;
    prev_octave_cv_ = 0x0;
    prev_root_cv_ = 0x0;   
    prev_scale_cv_ = 0x0;       
  }
    
  void reset_scale() {
    scale_reset_ = true;
  }
  
  void Init(DQ_ChannelSource source, DQ_ChannelTriggerSource trigger_source) {
    
    InitDefaults();
    apply_value(DQ_CHANNEL_SETTING_SOURCE, source);
    apply_value(DQ_CHANNEL_SETTING_TRIGGER, trigger_source);

    force_update_ = true;

    for (int i = 0; i < NUM_SCALE_SLOTS; i++) {
      last_scale_[i] = -1;
      last_mask_[i] = 0;
    }
    
    aux_sample_ = 0;
    last_sample_ = 0;
    last_aux_sample_ = 0;
    continuous_offset_ = 0;
    scale_sequence_cnt_ = 0;
    scale_reset_ = 0;
    active_scale_slot_ = 0;
    display_scale_slot_ = 0;
    display_root_ = 0;
    root_last_ = 0;
    transpose_last_ = 0;
    prev_scale_slot_ = 0;
    scale_advance_ = 0;
    scale_advance_state_ = 0;
    schedule_scale_update_ = 0;
    schedule_mask_rotate_ = 0;
    prev_octave_cv_ = 0;
    prev_transpose_cv_ = 0;
    prev_root_cv_ = 0;
    prev_scale_cv_ = 0;
    prev_destination_ = 0;
    prev_pulsewidth_ = 100;
    ticks_ = 0;
    channel_frequency_in_ticks_ = 1000;
    pulse_width_in_ticks_ = 1000;
    
    trigger_delay_.Init();
    quantizer_.Init();
    update_scale(true, 0, false);
    trigger_display_.Init();
    update_enabled_settings();

    turing_machine_.Init();
    turing_display_length_ = get_turing_length();

    scrolling_history_.Init(OC::DAC::kOctaveZero * 12 << 7);
  }

  void force_update() {
    force_update_ = true;
  }

  void schedule_scale_update() {
    schedule_scale_update_ = true;
  }

  inline void Update(uint32_t triggers, DAC_CHANNEL dac_channel, DAC_CHANNEL aux_channel) {

    ticks_++;

    DQ_ChannelTriggerSource trigger_source = get_trigger_source();
    bool continuous = DQ_CHANNEL_TRIGGER_CONTINUOUS_UP == trigger_source || DQ_CHANNEL_TRIGGER_CONTINUOUS_DOWN == trigger_source;
    bool triggered = !continuous &&
      (triggers & DIGITAL_INPUT_MASK(trigger_source - DQ_CHANNEL_TRIGGER_TR1));

    trigger_delay_.Update();
    if (triggered)
      trigger_delay_.Push(OC::trigger_delay_ticks[get_trigger_delay()]);
    triggered = trigger_delay_.triggered();

    if (triggered) {
      channel_frequency_in_ticks_ = ticks_;
      ticks_ = 0x0;
      update_asr_ = true;  
      aux_sample_ = ON; 
    }
         
    if (get_scale_seq_mode()) {
        // to do, don't hardcode .. 
      uint8_t _advance_trig = (dac_channel == DAC_CHANNEL_A) ? digitalReadFast(TR2) : digitalReadFast(TR4);
      if (_advance_trig < scale_advance_state_) 
        scale_advance_ = true;
      scale_advance_state_ = _advance_trig;  

      if (scale_reset_) {
       // manual change?
       scale_reset_ = false;
       active_scale_slot_ = get_scale_select();
       prev_scale_slot_ = display_scale_slot_ = active_scale_slot_;
      }
    }
    else if (prev_scale_slot_ != get_scale_select()) {
      active_scale_slot_ = get_scale_select();
      prev_scale_slot_ = display_scale_slot_ = active_scale_slot_;
    }
          
    if (scale_advance_) {
      scale_sequence_cnt_++;
      active_scale_slot_ = get_scale_select() + (scale_sequence_cnt_ % (get_scale_seq_mode() + 1));
      
      if (active_scale_slot_ >= NUM_SCALE_SLOTS)
        active_scale_slot_ -= NUM_SCALE_SLOTS;
      scale_advance_ = false;
      schedule_scale_update_ = true;
    }

    bool update = continuous || triggered;
       
    int32_t sample = last_sample_;
    int32_t temp_sample = 0;
    int32_t history_sample = 0;
    uint8_t aux_mode = get_aux_mode();

    if (update) {
        
      int32_t transpose, pitch, quantized = 0x0;
      int source, cv_source, channel_id, octave, root, _aux_cv_destination;
      
      source = cv_source = get_source();
      _aux_cv_destination = get_aux_cv_dest();
      channel_id = (dac_channel == DAC_CHANNEL_A) ? 1 : 3; // hardcoded to use CV2, CV4, for now

      if (_aux_cv_destination != prev_destination_)
        clear_dest();
      prev_destination_ = _aux_cv_destination;
      // active scale slot:
      display_scale_slot_ = prev_scale_slot_ = active_scale_slot_ + prev_scale_cv_;
      // get root value
      root = get_root(display_scale_slot_) + prev_root_cv_;
      // get transpose value
      transpose = get_transpose(display_scale_slot_) + prev_transpose_cv_;
      // get octave value
      octave = get_octave() + prev_octave_cv_;

      // S/H: ADC values
      if (!continuous) {
        
        switch(_aux_cv_destination) {
        
          case DQ_DEST_NONE:
          break;
          case DQ_DEST_SCALE_SLOT:
            display_scale_slot_ += (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) + 255) >> 9;
            // if scale changes, we have to update the root and transpose values, too; mask gets updated in update_scale
            root = get_root(display_scale_slot_);
            transpose = get_transpose(display_scale_slot_);
          break;
          case DQ_DEST_ROOT:
              root += (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) + 127) >> 8;
          break;
          case DQ_DEST_MASK:
              schedule_mask_rotate_ = (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) + 127) >> 8;
          break;
          case DQ_DEST_OCTAVE:
            octave += (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) + 255) >> 9;
          break;
          case DQ_DEST_TRANSPOSE:
            transpose += (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) + 64) >> 7;
          break;
          default:
          break;
        } // end switch  
      } // -> triggered update

      // constrain values: 
      CONSTRAIN(display_scale_slot_, 0, NUM_SCALE_SLOTS-1);
      CONSTRAIN(octave, -4, 4);
      CONSTRAIN(root, 0, 11);
      CONSTRAIN(transpose, -12, 12); 

      // update scale?
      update_scale(force_update_, display_scale_slot_, schedule_mask_rotate_);

      // internal CV source?
      if (source > DQ_CHANNEL_SOURCE_CV4) 
        cv_source = channel_id - 1;

      // now, acquire + process sample:   
      pitch = quantizer_.enabled()
                ? OC::ADC::raw_pitch_value(static_cast<ADC_CHANNEL>(cv_source))
                : OC::ADC::pitch_value(static_cast<ADC_CHANNEL>(cv_source));

      switch (source) {

        case DQ_CHANNEL_SOURCE_CV1:
        case DQ_CHANNEL_SOURCE_CV2:
        case DQ_CHANNEL_SOURCE_CV3:
        case DQ_CHANNEL_SOURCE_CV4:
        quantized = quantizer_.Process(pitch, root << 7, transpose);
        break;
        case DQ_CHANNEL_SOURCE_TURING:
        {
          if (continuous)
              break;
              
          int16_t _length = get_turing_length();
          int16_t _probability = get_turing_probability();
          int16_t _range = get_turing_range();

          // _pitch can do other things now -- 
          switch (get_turing_CV()) {

              case 1:  // range
               _range += ((pitch + 63) >> 6);
               CONSTRAIN(_range, 1, 120);
              case 2:  // LEN, 1-32
               _length += ((pitch + 255) >> 8);
               CONSTRAIN(_length, 1, 32);
              break;
               case 3:  // P
               _probability += ((pitch + 15) >> 4);
               CONSTRAIN(_probability, 0, 255);
              break;
              default:
              break;
          }
          
          turing_machine_.set_length(_length);
          turing_machine_.set_probability(_probability); 
          turing_display_length_ = _length;

          uint32_t _shift_register = turing_machine_.Clock();   
          // Since our range is limited anyway, just grab the last byte for lengths > 8, otherwise scale to use bits.
          uint32_t shift = turing_machine_.length();
          uint32_t _scaled = (_shift_register & 0xFF) * _range;
          _scaled = _scaled >> (shift > 7 ? 8 : shift);
          quantized = quantizer_.Lookup(64 + _range / 2 - _scaled + transpose) + (root<< 7);
        }
        break;
        default:
        break;
      }
      
      // the output, thus far:
      sample = temp_sample = OC::DAC::pitch_to_scaled_voltage_dac(dac_channel, quantized, octave + continuous_offset_, OC::DAC::get_voltage_scaling(dac_channel));

      // special treatment, continuous update -- only update the modulation values if/when the quantized input changes:    
      bool _continuous_update = continuous && last_sample_ != sample;
       
      if (_continuous_update) {

          bool _re_quantize = false;
          int _aux_cv = 0;

          switch(_aux_cv_destination) {
            
            case DQ_DEST_NONE:
            break;
            case DQ_DEST_SCALE_SLOT:
            _aux_cv = (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) + 255) >> 9;
            if (_aux_cv !=  prev_scale_cv_) {  
                display_scale_slot_ += _aux_cv;
                CONSTRAIN(display_scale_slot_, 0, NUM_SCALE_SLOTS - 0x1);
                prev_scale_cv_ = _aux_cv;
                // update the root and transpose values
                root = get_root(display_scale_slot_);
                transpose = get_transpose(display_scale_slot_);
                // and update quantizer below:
                schedule_scale_update_ = true;
                _re_quantize = true;
            }
            break;
            case DQ_DEST_TRANSPOSE:
              _aux_cv = (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) + 63) >> 7;
              if (_aux_cv != prev_transpose_cv_) {
                  transpose = get_transpose(display_scale_slot_) + _aux_cv;
                  CONSTRAIN(transpose, -12, 12); 
                  prev_transpose_cv_ = _aux_cv;
                  _re_quantize = true;
              }
            break;
            case DQ_DEST_ROOT:
              _aux_cv = (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) + 127) >> 8;
              if (_aux_cv != prev_root_cv_) {
                  display_root_ = root = get_root(display_scale_slot_) + _aux_cv; 
                  CONSTRAIN(root, 0, 11);
                  prev_root_cv_ = _aux_cv;
                  _re_quantize = true;
              }
            break;
            case DQ_DEST_OCTAVE:
              _aux_cv = (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) + 255) >> 9;
              if (_aux_cv != prev_octave_cv_) {
                  octave = get_octave() + _aux_cv;
                  CONSTRAIN(octave, -4, 4);
                  prev_octave_cv_ = _aux_cv;
                  _re_quantize = true;
              }
            break;   
            case DQ_DEST_MASK:
              schedule_mask_rotate_ = (OC::ADC::value(static_cast<ADC_CHANNEL>(channel_id)) + 127) >> 8;
              schedule_scale_update_ = true;
            break;
            default:
            break; 
          } 
          // end switch

          // update scale?
          if (schedule_scale_update_ && _continuous_update) {
            update_scale(false, display_scale_slot_, schedule_mask_rotate_);
            schedule_scale_update_ = false;
          }  

          // offset when TR source = continuous ?
          int8_t _trigger_offset = 0;
          bool _trigger_update = false;
          if (OC::DigitalInputs::read_immediate(static_cast<OC::DigitalInput>(channel_id - 1))) {
             _trigger_offset = (trigger_source == DQ_CHANNEL_TRIGGER_CONTINUOUS_UP) ? 1 : -1;
          }
          if (_trigger_offset != continuous_offset_)
            _trigger_update = true;
          continuous_offset_ = _trigger_offset;
 
          // run quantizer again -- presumably could be made more efficient...
          if (_re_quantize) 
            quantized = quantizer_.Process(pitch, root << 7, transpose);
          if (_re_quantize || _trigger_update)
            sample = OC::DAC::pitch_to_scaled_voltage_dac(dac_channel, quantized, octave + continuous_offset_, OC::DAC::get_voltage_scaling(dac_channel));
          // update ASR?
          update_asr_ = (aux_mode == DQ_ASR && last_sample_ != sample);

      } 
      // end special treatment
      
      display_root_ = root;     
      history_sample = quantized + ((OC::DAC::kOctaveZero + octave) * 12 << 7);  
      
      // deal with aux output:
      switch (aux_mode) {
        
        case DQ_COPY:
        // offset the quantized value:
          aux_sample_ = OC::DAC::pitch_to_scaled_voltage_dac(aux_channel, quantized, octave + continuous_offset_ + get_aux_octave(), OC::DAC::get_voltage_scaling(aux_channel));
        break;
        case DQ_ASR:
        {
          if (update_asr_) {
            update_asr_ = false;
            aux_sample_ = OC::DAC::pitch_to_scaled_voltage_dac(aux_channel, last_aux_sample_, octave + continuous_offset_ + get_aux_octave(), OC::DAC::get_voltage_scaling(aux_channel));
            last_aux_sample_ = quantized;
          }
        }
        break;
        case DQ_GATE: {
        
           if (source == DQ_CHANNEL_SOURCE_TURING) {
          
            switch(get_turing_trig_out()) {
              
              case DQ_ECHO:
              break;
              case DQ_LSB:
              if (!turing_machine_.get_LSB())
                aux_sample_ = OFF;
              break;
              case DQ_CHANGE:
              if (last_sample_ == sample)
                aux_sample_ = OFF;
              break;
              default:
              break;
            }
          }
        }
        break;
        default:
        break;
      }
    }

    // in continuous mode, don't track transposed sample:
    bool changed = continuous ? (last_sample_ != temp_sample) : (last_sample_ != sample);
     
    if (changed) {
      
      MENU_REDRAW = 1;
      last_sample_ = continuous ? temp_sample : sample;

      // in continuous mode, make aux output go high:
      if (continuous && aux_mode == DQ_GATE) {
        aux_sample_ = ON;
        ticks_ = 0x0;
      } 
    }
    
    // aux outputs:
    int32_t aux_sample = aux_sample_; 

    switch (aux_mode) {

      case DQ_COPY: 
      case DQ_ASR:
      break;
      case DQ_GATE:
      { 
      if (aux_sample) { 
           
            // pulsewidth setting -- 
            int16_t _pulsewidth = get_pulsewidth();
    
            if (_pulsewidth || continuous) { // don't echo
    
                bool _gates = false;
             
                if (_pulsewidth == PULSEW_MAX)
                  _gates = true;
                // we-can't-echo-hack  
                if (continuous && !_pulsewidth)
                  _pulsewidth = 0x1;
           
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
                  aux_sample_ = OFF;
                else // keep on 
                  aux_sample_ = ON; 
             }
             else {
                // we simply echo the pulsewidth:
                 aux_sample_ = OC::DigitalInputs::read_immediate(get_digital_input()) ? ON : OFF;
             }
         }  
      } 
      // scale gate
      #ifdef BUCHLA_4U
        aux_sample = (aux_sample_ == ON) ? OC::DAC::get_octave_offset(aux_channel, OCTAVES - OC::DAC::kOctaveZero - 0x2) : OC::DAC::get_zero_offset(aux_channel);
      #else
        aux_sample = (aux_sample_ == ON) ? OC::DAC::get_octave_offset(aux_channel, OCTAVES - OC::DAC::kOctaveZero - 0x1) : OC::DAC::get_zero_offset(aux_channel);
      #endif
      break;
      default:
      break;
    }
    
    OC::DAC::set(dac_channel, sample);
    OC::DAC::set(aux_channel, aux_sample);

    if (triggered || (continuous && changed)) {
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
      
      case SLOT1: 
        apply_value(DQ_CHANNEL_SETTING_MASK1, mask); 
        last_mask_[0] = mask;
      break;
      case SLOT2: 
        apply_value(DQ_CHANNEL_SETTING_MASK2, mask); 
        last_mask_[1] = mask;
      break;
      case SLOT3:  
        apply_value(DQ_CHANNEL_SETTING_MASK3, mask); 
        last_mask_[2] = mask;
      break;
      case SLOT4: 
        apply_value(DQ_CHANNEL_SETTING_MASK4, mask); 
        last_mask_[3] = mask;
      break;
      default:
      break;
    }
    force_update_ = true;
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

      case SLOT1:
      *settings++ = DQ_CHANNEL_SETTING_SCALE1;
      break;
      case SLOT2:
      *settings++ = DQ_CHANNEL_SETTING_SCALE2;
      break;
      case SLOT3:
      *settings++ = DQ_CHANNEL_SETTING_SCALE3;
      break;
      case SLOT4:
      *settings++ = DQ_CHANNEL_SETTING_SCALE4;
      break;
      default:
      break;
    }

    // to do -- might as well disable no scale
    if (OC::Scales::SCALE_NONE != get_scale(get_scale_select())) {

      switch(get_scale_select()) {
        
        case SLOT1:  
         *settings++ = DQ_CHANNEL_SETTING_MASK1; 
        break;
        case SLOT2:  
         *settings++ = DQ_CHANNEL_SETTING_MASK2; 
        break;
        case SLOT3:
         *settings++ = DQ_CHANNEL_SETTING_MASK3; 
        break;
        case SLOT4:  
         *settings++ = DQ_CHANNEL_SETTING_MASK4; 
        break;
        default:
        break;
      }
      
      *settings++ = DQ_CHANNEL_SETTING_SEQ_MODE;
      *settings++ = DQ_CHANNEL_SETTING_SCALE_SEQ;

      switch(get_scale_select()) {
        
        case SLOT1:
        *settings++ = DQ_CHANNEL_SETTING_ROOT1;
        *settings++ = DQ_CHANNEL_SETTING_TRANSPOSE1;
        break;
        case SLOT2:
        *settings++ = DQ_CHANNEL_SETTING_ROOT2;
        *settings++ = DQ_CHANNEL_SETTING_TRANSPOSE2;
        break;
        case SLOT3:
        *settings++ = DQ_CHANNEL_SETTING_ROOT3;
        *settings++ = DQ_CHANNEL_SETTING_TRANSPOSE3;
        break;
        case SLOT4:
        *settings++ = DQ_CHANNEL_SETTING_ROOT4;
        *settings++ = DQ_CHANNEL_SETTING_TRANSPOSE4;
        break;
        default:
        break;
      }      
    }

    // todo -- item order?
    *settings++ = DQ_CHANNEL_SETTING_OCTAVE;
    *settings++ = DQ_CHANNEL_SETTING_SOURCE;
    
    // CV sources:
    switch (get_source()) {
      
      case DQ_CHANNEL_SOURCE_TURING:
        *settings++ = DQ_CHANNEL_SETTING_TURING_RANGE;
        *settings++ = DQ_CHANNEL_SETTING_TURING_LENGTH;
        *settings++ = DQ_CHANNEL_SETTING_TURING_PROB;
        *settings++ = DQ_CHANNEL_SETTING_TURING_CV_SOURCE;
        *settings++ = DQ_CHANNEL_SETTING_TURING_TRIG_OUT;
       break;
       default:
       break;
    }
    
    *settings++ = DQ_CHANNEL_SETTING_AUX_CV_DEST;
    *settings++ = DQ_CHANNEL_SETTING_TRIGGER;
    
    if (get_trigger_source() < DQ_CHANNEL_TRIGGER_CONTINUOUS_UP) 
      *settings++ = DQ_CHANNEL_SETTING_DELAY;
      
    *settings++ = DQ_CHANNEL_SETTING_AUX_OUTPUT;
    
    switch(get_aux_mode()) {
      
      case DQ_GATE:
        *settings++ = DQ_CHANNEL_SETTING_PULSEWIDTH;
      break;
      case DQ_COPY:
        *settings++ = DQ_CHANNEL_SETTING_AUX_OCTAVE;
      break;
      case DQ_ASR:
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
  bool update_asr_;
  int last_scale_[NUM_SCALE_SLOTS];
  uint16_t last_mask_[NUM_SCALE_SLOTS];
  int scale_sequence_cnt_;
  int active_scale_slot_;
  int display_scale_slot_;
  int display_root_;
  int root_last_;
  int transpose_last_;
  int prev_scale_slot_;
  int8_t scale_advance_;
  int8_t scale_advance_state_;
  bool scale_reset_;
  bool schedule_scale_update_;
  int32_t schedule_mask_rotate_;
  int32_t last_sample_;
  int32_t aux_sample_;
  int32_t last_aux_sample_;
  int8_t continuous_offset_;
  uint8_t prev_pulsewidth_;
  int8_t prev_destination_;
  int8_t prev_octave_cv_;
  int8_t prev_transpose_cv_;
  int8_t prev_root_cv_;
  int8_t prev_scale_cv_;
  
  uint32_t ticks_;
  uint32_t channel_frequency_in_ticks_;
  uint32_t pulse_width_in_ticks_;

  util::TriggerDelay<OC::kMaxTriggerDelayTicks> trigger_delay_;
  braids::Quantizer quantizer_;
  OC::DigitalInputDisplay trigger_display_;

  // internal CV sources;
  util::TuringShiftRegister turing_machine_;
  int8_t turing_display_length_;
  
  int num_enabled_settings_;
  DQ_ChannelSetting enabled_settings_[DQ_CHANNEL_SETTING_LAST];

  OC::vfx::ScrollingHistory<int32_t, 5> scrolling_history_;

  bool update_scale(bool force, uint8_t scale_select, int32_t mask_rotate) {

    force_update_ = false;  
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

const char* const dq_tm_trig_out[] = {
  "echo", "lsb", "chng"
};

SETTINGS_DECLARE(DQ_QuantizerChannel, DQ_CHANNEL_SETTING_LAST) {
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { 0, 0, 11, "root #1", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 },
  { 0, 0, 11, "root #2", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 },
  { 0, 0, 11, "root #3", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 },
  { 0, 0, 11, "root #4", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 },
  { 0, 0, NUM_SCALE_SLOTS - 1, "scale #", dq_seq_scales, settings::STORAGE_TYPE_U8 },
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 },
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 },
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 },
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 },
  { 0, 0, 3, "seq_mode", dq_seq_modes, settings::STORAGE_TYPE_U4 },
  { DQ_CHANNEL_SOURCE_CV1, DQ_CHANNEL_SOURCE_CV1, 4, "CV source", OC::Strings::cv_input_names, settings::STORAGE_TYPE_U4 }, /// to do ..
  { DQ_CHANNEL_TRIGGER_CONTINUOUS_DOWN, 0, DQ_CHANNEL_TRIGGER_LAST - 1, "trigger source", OC::Strings::channel_trigger_sources, settings::STORAGE_TYPE_U8 },
  { 0, 0, OC::kNumDelayTimes - 1, "--> latency", OC::Strings::trigger_delay_times, settings::STORAGE_TYPE_U8 },
  { 0, -5, 7, "transpose #1", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -5, 7, "transpose #2", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -5, 7, "transpose #3", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -5, 7, "transpose #4", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -4, 4, "octave", NULL, settings::STORAGE_TYPE_I8 },
  { 0, 0, DQ_AUX_MODE_LAST-1, "aux.output", dq_aux_outputs, settings::STORAGE_TYPE_U8 },
  { 25, 0, PULSEW_MAX, "--> pw", NULL, settings::STORAGE_TYPE_U8 },
  { 0, -5, 5, "--> aux +/-", NULL, settings::STORAGE_TYPE_I8 }, // aux octave
  { 0, 0, DQ_DEST_LAST-1, "CV aux.", dq_aux_cv_dest, settings::STORAGE_TYPE_U8 },
  { 16, 1, 32, " > LFSR length", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, " > LFSR p", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 3, " > LFSR CV", OC::Strings::TM_aux_cv_destinations, settings::STORAGE_TYPE_U8 }, // ??
  { 15, 1, 120, " > LFSR range", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, DQ_TRIG_AUX_LAST-1, " > LFSR TRIG", dq_tm_trig_out, settings::STORAGE_TYPE_U8 }
};

// WIP refactoring to better encapsulate and for possible app interface change
class DualQuantizer {
public:
  void Init() {
    selected_channel = 0;
    cursor.Init(DQ_CHANNEL_SETTING_SCALE1, DQ_CHANNEL_SETTING_LAST - 1);
    scale_editor.Init(true);
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
    dq_quantizer_channels[i].Init(static_cast<DQ_ChannelSource>(DQ_CHANNEL_SOURCE_CV1 + 2*i), static_cast<DQ_ChannelTriggerSource>(DQ_CHANNEL_TRIGGER_TR1 + 2*i));
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
    //int scale = dq_quantizer_channels[i].get_scale_select();
    for (size_t j = SLOT1; j < LAST_SLOT; j++) {
      dq_quantizer_channels[i].update_scale_mask(dq_quantizer_channels[i].get_mask(j), j);
    }
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
    if (channel.get_aux_cv_dest() == DQ_DEST_ROOT)
      graphics.print(OC::Strings::note_names[channel.get_display_root()]);
    else
      graphics.print(OC::Strings::note_names[channel.get_root(channel.get_display_scale())]);
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
      case DQ_CHANNEL_SETTING_TRIGGER:
      {
        if (channel.get_source() > DQ_CHANNEL_SOURCE_CV4)
           list_item.DrawValueMax(value, attr, DQ_CHANNEL_TRIGGER_TR4);
        else 
          list_item.DrawDefault(value, attr);
      }
      break;
      case DQ_CHANNEL_SETTING_SOURCE:
      {
      if (channel.get_source() == DQ_CHANNEL_SOURCE_TURING) {
       
          int turing_length = channel.get_turing_display_length();
          int w = turing_length >= 16 ? 16 * 3 : turing_length * 3;

          menu::DrawMask<true, 16, 8, 1>(menu::kDisplayWidth, list_item.y, channel.get_shift_register(), turing_length);
          list_item.valuex = menu::kDisplayWidth - w - 1;
          list_item.DrawNoValue<true>(value, attr);
       } 
       else if (channel.get_trigger_source() > DQ_CHANNEL_TRIGGER_TR4)
          list_item.DrawValueMax(value, attr, DQ_CHANNEL_SOURCE_CV4);
       else 
          list_item.DrawDefault(value, attr);
      }
      break;
      case DQ_CHANNEL_SETTING_PULSEWIDTH:
        list_item.Draw_PW_Value(value, attr);
      break;
      default:
        list_item.DrawDefault(value, attr);
      break;
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
    CONSTRAIN(selected_channel, 0, NUMCHANNELS - 0x1);
    dq_state.selected_channel = selected_channel;

    DQ_QuantizerChannel &selected = dq_quantizer_channels[dq_state.selected_channel];
    selected.update_enabled_settings();
    dq_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
    //??
    dq_state.cursor.Scroll(0x0);
    
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    
    DQ_QuantizerChannel &selected = dq_quantizer_channels[dq_state.selected_channel];
    
    if (dq_state.editing()) {
      
      DQ_ChannelSetting setting = selected.enabled_setting_at(dq_state.cursor_pos());
      if (DQ_CHANNEL_SETTING_MASK1 != setting || DQ_CHANNEL_SETTING_MASK2 != setting || DQ_CHANNEL_SETTING_MASK3 != setting || DQ_CHANNEL_SETTING_MASK4 != setting) {

        int event_value = event.value;

        // hack disable internal sources when mode = continuous:
        switch (setting) {
          
          case DQ_CHANNEL_SETTING_TRIGGER:
          {
            if (selected.get_trigger_source() == DQ_CHANNEL_TRIGGER_TR4 && selected.get_source() > DQ_CHANNEL_SOURCE_CV4 && event_value > 0)
              event_value = 0x0;
          }
          break;
          case DQ_CHANNEL_SETTING_SOURCE: 
          {
             if (selected.get_source() == DQ_CHANNEL_SOURCE_CV4 && selected.get_trigger_source() > DQ_CHANNEL_TRIGGER_TR4 && event_value > 0)
              event_value = 0x0;
          }
          break;
          default:
          break;
        }
        
        if (selected.change_value(setting, event_value))
          selected.force_update();

        switch (setting) {
          case DQ_CHANNEL_SETTING_SCALE1:
          case DQ_CHANNEL_SETTING_SCALE2:
          case DQ_CHANNEL_SETTING_SCALE3:
          case DQ_CHANNEL_SETTING_SCALE4:
          case DQ_CHANNEL_SETTING_TRIGGER:
          case DQ_CHANNEL_SETTING_SOURCE:
          case DQ_CHANNEL_SETTING_AUX_OUTPUT:
            selected.update_enabled_settings();
            dq_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
          break;
          case DQ_CHANNEL_SETTING_SCALE_SEQ:
            selected.update_enabled_settings();
            dq_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
            selected.reset_scale();
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

  // copy scale settings to all slots:
  DQ_QuantizerChannel &selected_channel = dq_quantizer_channels[dq_state.selected_channel];
  int _slot = selected_channel.get_scale_select();
  int scale = selected_channel.get_scale(_slot);
  int mask = selected_channel.get_mask(_slot);
  int root = selected_channel.get_root(_slot); 
  int transpose = selected_channel.get_transpose(_slot); 
 
  for (int i = 0; i < NUM_SCALE_SLOTS; ++i) {
    for (int j = 0; j < NUMCHANNELS; ++j)
      dq_quantizer_channels[j].set_scale_at_slot(scale, mask, root, transpose, i);
  }
}

void DQ_downButtonLong() {
  // reset mask
  DQ_QuantizerChannel &selected_channel = dq_quantizer_channels[dq_state.selected_channel];
  int scale_slot = selected_channel.get_scale_select();
  selected_channel.set_scale_at_slot(selected_channel.get_scale(scale_slot), 0xFFFF, selected_channel.get_root(scale_slot), selected_channel.get_transpose(scale_slot), scale_slot);
}

int32_t dq_history[5];
static const weegfx::coord_t dq_kBottom = 60;

inline int32_t dq_render_pitch(int32_t pitch, weegfx::coord_t x, weegfx::coord_t width) {
  CONSTRAIN(pitch, 0, 120 << 7);
  int32_t octave = pitch / (12 << 7);
  pitch -= (octave * 12 << 7);
  graphics.drawHLine(x, dq_kBottom - ((pitch * 4) >> 7), width << 1);
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
    case DQ_CHANNEL_SOURCE_TURING:
      menu::DrawMask<true, 16, 8, 1>(start_x + 58, 1, get_shift_register(), get_turing_display_length());
      break;
    default: {
      graphics.setPixel(start_x + DQ_OFFSET_X - 16, 4);
      int32_t cv = OC::ADC::value(static_cast<ADC_CHANNEL>(source));
      cv = (cv * 20 + 2047) >> 11;
      if (cv < 0)
        graphics.drawRect(start_x + DQ_OFFSET_X - 16 + cv, 6, -cv, 2);
      else if (cv > 0)
        graphics.drawRect(start_x + DQ_OFFSET_X - 16, 6, cv, 2);
      else
        graphics.drawRect(start_x + DQ_OFFSET_X - 16, 6, 1, 2);
    }
    break;
  }

#ifdef DQ_DEBUG_SCREENSAVER
  graphics.drawVLinePattern(start_x + 56, 0, 64, 0x55);
#endif

  // Draw semitone intervals, 4px apart
  weegfx::coord_t x = start_x + 56;
  weegfx::coord_t y = dq_kBottom;
  for (int i = 0; i < 12; ++i, y -= 4)
    graphics.setPixel(x, y);

  x = start_x + 1;
  dq_render_pitch(dq_history[0], x, scroll_pos); x += scroll_pos;
  dq_render_pitch(dq_history[1], x, 6); x += 6;
  dq_render_pitch(dq_history[2], x, 6); x += 6;
  dq_render_pitch(dq_history[3], x, 6); x += 6;

  int32_t octave = dq_render_pitch(dq_history[4], x, 6 - scroll_pos);
  graphics.drawBitmap8(start_x + 58, dq_kBottom - octave * 4 - 1, OC::kBitmapLoopMarkerW, OC::bitmap_loop_markers_8 + OC::kBitmapLoopMarkerW);
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
