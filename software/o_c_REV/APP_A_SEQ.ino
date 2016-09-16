// Copyright (c) 2015, 2016 Max Stadler, Patrick Dowling
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


#include "util/util_settings.h"
#include "OC_apps.h"
#include "OC_DAC.h"
#include "OC_menus.h"
#include "OC_strings.h"
#include "OC_visualfx.h"
#include "OC_sequence_edit.h"
#include "OC_patterns.h"
#include "OC_scales.h"
#include "OC_scale_edit.h"
#include "braids_quantizer.h"
#include "braids_quantizer_scales.h"
#include "extern/dspinst.h"

namespace menu = OC::menu; 

const uint8_t NUM_CHANNELS = 2;
const uint8_t MULT_MAX = 18;    // max multiplier
const uint8_t MULT_BY_ONE = 11; // default multiplication  
const uint8_t PULSEW_MAX = 255; // max pulse width [ms]

const uint32_t SCALE_PULSEWIDTH = 58982; // 0.9 for signed_multiply_32x16b
const uint32_t TICKS_TO_MS = 43691; // 0.6667f : fraction, if TU_CORE_TIMER_RATE = 60 us : 65536U * ((1000 / TU_CORE_TIMER_RATE) - 16)
const uint32_t TICK_JITTER = 0xFFFFFFF;  // 1/16 : threshold/double triggers reject -> ext_frequency_in_ticks_
const uint32_t TICK_SCALE  = 0xC0000000; // 0.75 for signed_multiply_32x32                      

uint32_t ticks_src1 = 0; // main clock frequency (top)
uint32_t ticks_src2 = 0; // sec. clock frequency (bottom)

const uint64_t multipliers_[] = {
  
  0xFFFFFFFF, // x1
  0x80000000, // x2
  0x55555555, // x3
  0x40000000, // x4
  0x33333333, // x5
  0x2AAAAAAB, // x6
  0x24924925, // x7
  0x20000000  // x8

}; // = 2^32 / multiplier

const uint64_t pw_scale_[] = {
  
  0xFFFFFFFF, // /64
  0xC0000000, // /48
  0x80000000, // /32
  0x40000000, // /16
  0x20000000, // /8
  0x1C000000, // /7
  0x18000000, // /6
  0x14000000, // /5
  0x10000000, // /4
  0xC000000,  // /3
  0x8000000,  // /2
  0x4000000,  // x1

}; // = 2^32 * divisor / 64

const uint8_t divisors_[] = {

   64,
   48,
   32,
   16,
   8,
   7,
   6,
   5,
   4,
   3,
   2,
   1
};

enum SEQ_ChannelSetting { 

  SEQ_CHANNEL_SETTING_MODE, // 
  SEQ_CHANNEL_SETTING_CLOCK,
  SEQ_CHANNEL_SETTING_RESET,
  SEQ_CHANNEL_SETTING_MULT,
  SEQ_CHANNEL_SETTING_PULSEWIDTH,
  //
  SEQ_CHANNEL_SETTING_SCALE,
  SEQ_CHANNEL_SETTING_OCTAVE, 
  SEQ_CHANNEL_SETTING_SCALE_MASK,
  //
  SEQ_CHANNEL_SETTING_MASK1,
  SEQ_CHANNEL_SETTING_MASK2,
  SEQ_CHANNEL_SETTING_MASK3,
  SEQ_CHANNEL_SETTING_MASK4,
  SEQ_CHANNEL_SETTING_SEQUENCE,
  SEQ_CHANNEL_SETTING_SEQUENCE_LEN1,
  SEQ_CHANNEL_SETTING_SEQUENCE_LEN2,
  SEQ_CHANNEL_SETTING_SEQUENCE_LEN3,
  SEQ_CHANNEL_SETTING_SEQUENCE_LEN4,
  SEQ_CHANNEL_SETTING_SEQUENCE_PLAYMODE,
  // cv sources
  SEQ_CHANNEL_SETTING_MULT_CV_SOURCE,
  SEQ_CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE,
  SEQ_CHANNEL_SETTING_SEQ_CV_SOURCE,
  SEQ_CHANNEL_SETTING_MASK_CV_SOURCE,
  SEQ_CHANNEL_SETTING_LAST
};

enum SEQ_ChannelTriggerSource {
  SEQ_CHANNEL_TRIGGER_TR1,
  SEQ_CHANNEL_TRIGGER_TR2,
  SEQ_CHANNEL_TRIGGER_NONE,
  SEQ_CHANNEL_TRIGGER_FREEZE_HI2,
  SEQ_CHANNEL_TRIGGER_FREEZE_LO2,
  SEQ_CHANNEL_TRIGGER_FREEZE_HI4,
  SEQ_CHANNEL_TRIGGER_FREEZE_LO4,
  SEQ_CHANNEL_TRIGGER_LAST
};

enum SEQ_ChannelCV_Mapping {
  CHANNEL_CV_MAPPING_CV1,
  CHANNEL_CV_MAPPING_CV2,
  CHANNEL_CV_MAPPING_CV3,
  CHANNEL_CV_MAPPING_CV4,
  CHANNEL_CV_MAPPING_LAST
};

enum SEQ_CLOCKSTATES {
  OFF,
  ON = 50000
};

uint64_t ext_frequency[SEQ_CHANNEL_TRIGGER_NONE];

class SEQ_Channel : public settings::SettingsBase<SEQ_Channel, SEQ_CHANNEL_SETTING_LAST> {
public:
  
  uint8_t get_mode() const {
    return values_[SEQ_CHANNEL_SETTING_MODE];
  }

  uint8_t get_clock_source() const {
    return values_[SEQ_CHANNEL_SETTING_CLOCK];
  }

  void set_clock_source(uint8_t _src) {
    apply_value(SEQ_CHANNEL_SETTING_CLOCK, _src);
  }

  int get_octave() const {
    return values_[SEQ_CHANNEL_SETTING_OCTAVE];
  }
  
  int8_t get_multiplier() const {
    return values_[SEQ_CHANNEL_SETTING_MULT];
  }

  uint16_t get_pulsewidth() const {
    return values_[SEQ_CHANNEL_SETTING_PULSEWIDTH];
  }

  uint8_t get_reset_source() const {
    return values_[SEQ_CHANNEL_SETTING_RESET];
  }

  void set_reset_source(uint8_t src) {
    apply_value(SEQ_CHANNEL_SETTING_RESET, src);
  }

  int get_sequence() const {
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE];
  }

  int get_current_sequence() const {
    return sequence_last_;
  }

  int get_playmode() const {
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE_PLAYMODE];
  }

  int get_display_sequence() const {
    return display_sequence_;
  }

  void set_display_sequence(uint8_t seq) {
    display_sequence_ = seq;
  }

  int get_display_mask() const {
    return display_mask_;
  }

  void set_sequence(uint8_t seq) {
    apply_value(SEQ_CHANNEL_SETTING_SEQUENCE, seq);
  }

  void set_gate(uint16_t _state) {
    gate_state_ = _state;
  }

  uint8_t get_sequence_length(uint8_t _seq) const {
    
    switch (_seq) {

    case 1:
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE_LEN2];
    break;
    case 2:
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE_LEN3];
    break;
    case 3:
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE_LEN4];
    break;    
    default:
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE_LEN1];
    break;
    }
  }
  
  uint8_t get_mult_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_MULT_CV_SOURCE];
  }

  uint8_t get_pulsewidth_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE];
  }

  uint8_t get_mask_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_MASK_CV_SOURCE];
  }

  uint8_t get_sequence_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_SEQ_CV_SOURCE];
  }
  
  void update_pattern_mask(uint16_t mask, uint8_t sequence) {

    switch(sequence) {
   
    case 1:
      apply_value(SEQ_CHANNEL_SETTING_MASK2, mask);
      break;
    case 2:
      apply_value(SEQ_CHANNEL_SETTING_MASK3, mask);
      break;
    case 3:
      apply_value(SEQ_CHANNEL_SETTING_MASK4, mask);
      break;    
    default:
      apply_value(SEQ_CHANNEL_SETTING_MASK1, mask);
      break;   
    }
  }
  
  int get_mask(uint8_t _this_sequence) const {

    switch(_this_sequence) {
      
    case 1:
      return values_[SEQ_CHANNEL_SETTING_MASK2];
      break;
    case 2:
      return values_[SEQ_CHANNEL_SETTING_MASK3];
      break;
    case 3:
      return values_[SEQ_CHANNEL_SETTING_MASK4];
      break;    
    default:
      return values_[SEQ_CHANNEL_SETTING_MASK1];
      break;   
    }
  }
  
  void set_sequence_length(uint8_t len, uint8_t seq) {

    switch(seq) {
      case 0:
      apply_value(SEQ_CHANNEL_SETTING_SEQUENCE_LEN1, len);
      break;
      case 1:
      apply_value(SEQ_CHANNEL_SETTING_SEQUENCE_LEN2, len);
      break;
      case 2:
      apply_value(SEQ_CHANNEL_SETTING_SEQUENCE_LEN3, len);
      break;
      case 3:
      apply_value(SEQ_CHANNEL_SETTING_SEQUENCE_LEN4, len);
      break;
      default:
      break;
    }
  }

  uint16_t get_clock_cnt() const {
    return clk_cnt_;
  }

  uint32_t get_step_pitch() const {
    return step_pitch_;
  }

  uint32_t get_step_gate() const {
    return gate_state_;
  }

  uint32_t get_step_state() const {
    return step_state_;
  }

  int32_t get_pitch_at_step(uint8_t seq, uint8_t step) const {

    uint8_t _channel_offset = !channel_id_ ? 0x0 : OC::Patterns::NUM_PATTERNS; 
    
    OC::Pattern *read_pattern_ = &OC::user_patterns[seq + _channel_offset];
    return read_pattern_->notes[step];
  }

  void set_pitch_at_step(uint8_t seq, uint8_t step, int32_t pitch) {

    uint8_t _channel_offset = !channel_id_ ? 0x0 : OC::Patterns::NUM_PATTERNS;
    
    OC::Pattern *write_pattern_ = &OC::user_patterns[seq + _channel_offset];
    write_pattern_->notes[step] = pitch;
  }

  void clear_user_pattern(uint8_t seq) {
    
    uint8_t _channel_offset = !channel_id_ ? 0x0 : OC::Patterns::NUM_PATTERNS;
    memcpy(&OC::user_patterns[seq + _channel_offset], &OC::patterns[0], sizeof(OC::Pattern));
  }

  uint8_t getTriggerState() const {
    return clock_display_.getState();
  }

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  void pattern_changed(uint16_t mask) {
    force_update_ = true;
    if (get_sequence() == sequence_last_)
      display_mask_ = mask;
  }

  void sync() {
    clk_cnt_ = 0x0;
    div_cnt_ = 0x0;
  }

  int get_scale() const {
    return values_[SEQ_CHANNEL_SETTING_SCALE];
  }

  int get_scale_mask() const {
    return values_[SEQ_CHANNEL_SETTING_SCALE_MASK];
  }

  void scale_changed() {
    force_scale_update_ = true;
  }

  void update_scale_mask(uint16_t mask) {
    
    force_scale_update_ = true;
    apply_value(SEQ_CHANNEL_SETTING_SCALE_MASK, mask); // Should automatically be updated
  }

  template <DAC_CHANNEL dac_channel>
  uint16_t get_zero() const {
     
    return OC::DAC::get_zero_offset(dac_channel);
  }
  
  void Init(SEQ_ChannelTriggerSource trigger_source, uint8_t id) {
    
    InitDefaults();
    channel_id_ = id;
    apply_value(SEQ_CHANNEL_SETTING_CLOCK, trigger_source);
    quantizer_.Init();  
    force_update_ = true;
    force_scale_update_ = true;
    gate_state_ = step_state_ = OFF;
    ticks_ = 0;
    subticks_ = 0;
    tickjitter_ = 10000;
    clk_cnt_ = 0;
    clk_src_ = get_clock_source();
    reset_ = false;
    reset_counter_ = false;
    reset_me_ = false;
    prev_multiplier_ = get_multiplier();
    prev_pulsewidth_ = get_pulsewidth();
 
    ext_frequency_in_ticks_ = get_pulsewidth() << 15; // init to something...
    channel_frequency_in_ticks_ = get_pulsewidth() << 15;
    pulse_width_in_ticks_ = get_pulsewidth() << 10;

    // TODO this needs to be per channel, not just 0 
    _ZERO = OC::calibration_data.dac.calibrated_octaves[0][OC::DAC::kOctaveZero];

    // WTF? get_mask doesn't return the saved mask?
    display_sequence_ = get_sequence();
    display_mask_ = get_mask(display_sequence_);
    sequence_last_ = display_sequence_;
    sequence_advance_ = false;
    sequence_advance_state_ = false; 

    uint32_t _seed = OC::ADC::value<ADC_CHANNEL_1>() + OC::ADC::value<ADC_CHANNEL_2>() + OC::ADC::value<ADC_CHANNEL_3>() + OC::ADC::value<ADC_CHANNEL_4>();
    randomSeed(_seed);
    clock_display_.Init();
    update_enabled_settings(0);  
  }

  bool update_scale(bool force, int32_t mask_rotate) {

    if (!force)
      return false;
    force_scale_update_ = false;  
    
    const int scale = get_scale();
    uint16_t  scale_mask = get_scale_mask();
   
    if (mask_rotate)
      scale_mask = OC::ScaleEditor<SEQ_Channel>::RotateMask(scale_mask, OC::Scales::GetScale(scale).num_notes, mask_rotate);

    if (force || (last_scale_ != scale || last_scale_mask_ != scale_mask)) {
      last_scale_ = scale;
      last_scale_mask_ = scale_mask;
     
      quantizer_.Configure(OC::Scales::GetScale(scale), scale_mask);
      return true;
    } else {
      return false;
    }
  }
 
  void force_update() {
    force_update_ = true;
  }

  /* main channel update below: */
   
  inline void Update(uint32_t triggers, DAC_CHANNEL dac_channel) {

     // increment channel ticks .. 
     subticks_++; 
     
     int8_t _clock_source, _reset_source, _mode;
     int8_t _multiplier;
     bool _none, _triggered, _tock, _sync;
     uint32_t prev_channel_frequency_in_ticks_ = 0x0;

     // core channel parameters -- 
     // 1. clock source:
     _clock_source = get_clock_source();
   
     // 2. multiplication:
     _multiplier = get_multiplier();

     if (get_mult_cv_source()) {
        _multiplier += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_mult_cv_source() - 1)) + 127) >> 8;             
        CONSTRAIN(_multiplier, 0, MULT_MAX);
     }
     // 3. sequencer_channel mode? 
     _mode = get_mode();
     // clocked ?
     _none = SEQ_CHANNEL_TRIGGER_NONE == _clock_source;
     // TR1 or TR3?
     _triggered = _clock_source ? (!_none && (triggers & (1 << OC::DIGITAL_INPUT_3))) : (!_none && (triggers & (1 << OC::DIGITAL_INPUT_1)));
     _tock = false;
     _sync = false;
     
     // new tick frequency:
     if (_clock_source <= SEQ_CHANNEL_TRIGGER_TR2) {
      
         if (_triggered || clk_src_ != _clock_source) {   
            ext_frequency_in_ticks_ = ext_frequency[_clock_source]; 
            _tock = true;
            div_cnt_--;
         }
     }    
     // store clock source:
     clk_src_ = _clock_source;
 
     // new multiplier ?
     if (prev_multiplier_ != _multiplier)
       _tock |= true;  
     prev_multiplier_ = _multiplier; 

     // if so, recalculate channel frequency and corresponding jitter-thresholds:
     if (_tock) {

        // when multiplying, skip too closely spaced triggers:
        if (_multiplier > MULT_BY_ONE) {
           prev_channel_frequency_in_ticks_ = multiply_u32xu32_rshift32(channel_frequency_in_ticks_, TICK_SCALE);
           // new frequency:    
           channel_frequency_in_ticks_ = multiply_u32xu32_rshift32(ext_frequency_in_ticks_, multipliers_[_multiplier-MULT_BY_ONE]); 
        }
        else {
           prev_channel_frequency_in_ticks_ = 0x0;
           // new frequency (used for pulsewidth):
           channel_frequency_in_ticks_ = multiply_u32xu32_rshift32(ext_frequency_in_ticks_, pw_scale_[_multiplier]) << 6;
        }
           
        tickjitter_ = multiply_u32xu32_rshift32(channel_frequency_in_ticks_, TICK_JITTER);  
     }
     // limit frequency to > 0
     if (!channel_frequency_in_ticks_)  
        channel_frequency_in_ticks_ = 1u;
          
     // reset? 
     _reset_source = get_reset_source();
     
     if (_reset_source < SEQ_CHANNEL_TRIGGER_NONE && reset_me_) {

        uint8_t reset_state_ = !_reset_source ? digitalReadFast(TR2) : digitalReadFast(TR4); // TR1, TR3 are main clock sources

        // ?
        if (reset_state_ < reset_) {
           div_cnt_ = 0x0;
           reset_counter_ = true; // reset clock counter below
           reset_me_ = false;
        }
        reset_ = reset_state_;
     } 

     //  do we advance sequences by TR2/4?
     if (get_playmode() > 3) {

        uint8_t _advance_trig = (dac_channel == DAC_CHANNEL_A) ? digitalReadFast(TR2) : digitalReadFast(TR4);
        // ?
        if (_advance_trig < sequence_advance_state_) 
          sequence_advance_ = true;
          
        sequence_advance_state_ = _advance_trig;  
       
     }

     // update scale? 
     update_scale(force_scale_update_, 0); // TODO rotate
       
    /*             
     *  brute force ugly sync hack:
     *  this, presumably, is needlessly complicated. 
     *  but seems to work ok-ish, w/o too much jitter and missing clocks... 
     */
     uint32_t _subticks = subticks_;

     if (_multiplier <= MULT_BY_ONE && _triggered && div_cnt_ <= 0) { 
        // division, so we track
        _sync = true;
        div_cnt_ = divisors_[_multiplier]; 
        subticks_ = channel_frequency_in_ticks_; // force sync
     }
     else if (_multiplier <= MULT_BY_ONE && _triggered) {
        // division, mute output:
        step_state_ = _ZERO;
     }
     else if (_multiplier > MULT_BY_ONE && _triggered)  {
        // multiplication, force sync, if clocked:
        _sync = true;
        subticks_ = channel_frequency_in_ticks_; 
     }
     else if (_multiplier > MULT_BY_ONE)
        _sync = true;   
     // end of ugly hack
     
     // time to output ? 
     if (subticks_ >= channel_frequency_in_ticks_ && _sync) { 
         
         // if so, reset ticks: 
         subticks_ = 0x0;
         // if tempo changed, reset _internal_ clock counter:
         if (_tock)
            ticks_ = 0x0;
     
         //reject, if clock is too jittery or skip quasi-double triggers when ext. frequency increases:
         if (_subticks < tickjitter_ || (_subticks < prev_channel_frequency_in_ticks_ && reset_me_)) 
            return;
            
         // mute output ?
         bool mute = 0;
         
         switch (_reset_source) {
             
             case SEQ_CHANNEL_TRIGGER_TR1: 
             case SEQ_CHANNEL_TRIGGER_TR2:
             case SEQ_CHANNEL_TRIGGER_NONE: 
             break; 
             case SEQ_CHANNEL_TRIGGER_FREEZE_HI2: 
              mute = !digitalReadFast(TR2);
             break; 
             case SEQ_CHANNEL_TRIGGER_FREEZE_LO2: 
              mute = digitalReadFast(TR2);
             break; 
             case SEQ_CHANNEL_TRIGGER_FREEZE_HI4: 
              mute = !digitalReadFast(TR4);
             break; 
             case SEQ_CHANNEL_TRIGGER_FREEZE_LO4:
              mute = digitalReadFast(TR4); 
             break; 
             default:
             break;
         }
         
         if (mute)
           return;
                     
         // only then count clocks:  
         clk_cnt_++;  
         
         // reset counter ? 
         if (reset_counter_) 
            clk_cnt_ = 0x0;
     
         // clear for reset:
         reset_me_ = true;
         reset_counter_ = false;
         // finally, process trigger + output
         step_state_ = gate_state_ = process_seq_channel(_mode); // = gate ...either ON, OFF
         
         if (step_state_ == ON) {
            uint8_t _transpose = 0; //TODO...
            uint8_t _root =  0; // TODO ... 
            // use the current sequence, updated in  process_seq_channel():
            step_pitch_ = get_pitch_at_step(display_sequence_, clk_cnt_) + (get_octave() * 12 << 7); /// 
            // todo -- limit range
            step_pitch_ = quantizer_.Process(step_pitch_, _root << 7, _transpose);  
         }
     }
  
     /*
      *  below: pulsewidth stuff
      */
      
     if (gate_state_) { 
       
        // pulsewidth setting -- 
        int16_t _pulsewidth = get_pulsewidth();

        if (_pulsewidth || _multiplier > MULT_BY_ONE) {

            bool _gates = false;

            // do we echo && multiply? if so, do half-duty cycle:
            if (!_pulsewidth)
                _pulsewidth = PULSEW_MAX;
                
            if (_pulsewidth == PULSEW_MAX)
              _gates = true;
            // CV?
            if (get_pulsewidth_cv_source()) {

              _pulsewidth += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_pulsewidth_cv_source() - 1)) + 8) >> 3; 
              if (!_gates)          
                CONSTRAIN(_pulsewidth, 1, PULSEW_MAX);
              else // CV for 50% duty cycle: 
                CONSTRAIN(_pulsewidth, 1, (PULSEW_MAX<<1) - 55);  // incl margin, max < 2x mult. see below 
            }
            // recalculate (in ticks), if new pulsewidth setting:
            if (prev_pulsewidth_ != _pulsewidth || ! subticks_) {
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
            if (subticks_ >= pulse_width_in_ticks_) 
              gate_state_ = OFF;
            else // keep on 
              gate_state_ = ON; 
         }
         else {
            // we simply echo the pulsewidth:
            bool _state = (_clock_source == SEQ_CHANNEL_TRIGGER_TR1) ? !digitalReadFast(TR1) : !digitalReadFast(TR3);  
           
            if (_state)
              gate_state_ = ON; 
            else  
              gate_state_ = OFF;
         }   
     }
  } // end update

  /* details re: trigger processing happens (mostly) here: */
  inline uint16_t process_seq_channel(uint8_t mode) {
 
      uint16_t _out = ON;
  
      switch (mode) {
  
          case 0: {
              // sequencer mode, adapted from o_C scale edit:
              int16_t _seq = get_sequence();
              
              if (get_sequence_cv_source()) {
                _seq += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_sequence_cv_source() - 1)) + 255) >> 9;             
                CONSTRAIN(_seq, 0, OC::Patterns::PATTERN_USER_LAST-1); 
              }

              uint8_t _playmode = get_playmode();
              
              if (_playmode) {

                // concatenate sequences:
                if (_playmode <= 3 && clk_cnt_ >= get_sequence_length(sequence_last_)) {
                  sequence_cnt_++;
                  sequence_last_ = _seq + (sequence_cnt_ % (_playmode+1));
                  clk_cnt_ = 0; 
                }
                else if (_playmode > 3 && sequence_advance_) {
                  _playmode -= 3;
                  sequence_cnt_++;
                  sequence_last_ = _seq + (sequence_cnt_ % (_playmode+1));
                  clk_cnt_ = 0;
                  sequence_advance_ = false; 
                }
               
                if (sequence_last_ >= OC::Patterns::PATTERN_USER_LAST)
                  sequence_last_ -= OC::Patterns::PATTERN_USER_LAST;
              }
              else 
                sequence_last_ = _seq;
                         
              _seq = sequence_last_;
              // this is the sequence # (USER1-USER4):
              display_sequence_ = _seq;
              // and corresponding pattern mask:
              uint16_t _mask = get_mask(_seq);
              // rotate mask ?
              if (get_mask_cv_source())
                _mask = update_sequence((OC::ADC::value(static_cast<ADC_CHANNEL>(get_mask_cv_source() - 1)) + 127) >> 8, _seq, _mask);
              display_mask_ = _mask;
              // reset counter ?
              if (clk_cnt_ >= get_sequence_length(_seq))
                clk_cnt_ = 0; 
              // output slot at current position:  
              _out = (_mask >> clk_cnt_) & 1u;
              _out = _out ? ON : OFF;
            }   
            break; 
           default:
            break; // end mode switch       
      }
      return _out; 
  }
 
  SEQ_ChannelSetting enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  void update_enabled_settings(uint8_t channel_id) {

    SEQ_ChannelSetting *settings = enabled_settings_;

    *settings++ = SEQ_CHANNEL_SETTING_SCALE,
    *settings++ = SEQ_CHANNEL_SETTING_SCALE_MASK,
    *settings++ = SEQ_CHANNEL_SETTING_SEQUENCE;
    
    switch (get_sequence()) {
    
      case 0:
        *settings++ = SEQ_CHANNEL_SETTING_MASK1;
      break;
      case 1:
        *settings++ = SEQ_CHANNEL_SETTING_MASK2;
      break;
      case 2:
        *settings++ = SEQ_CHANNEL_SETTING_MASK3;
      break;
      case 3:
        *settings++ = SEQ_CHANNEL_SETTING_MASK4;
      break;
      default:
      break;             
    }
   
   *settings++ = SEQ_CHANNEL_SETTING_SEQUENCE_PLAYMODE;
   *settings++ = SEQ_CHANNEL_SETTING_MULT;
   *settings++ = SEQ_CHANNEL_SETTING_PULSEWIDTH;
   *settings++ = SEQ_CHANNEL_SETTING_MODE;
   *settings++ = SEQ_CHANNEL_SETTING_RESET; 
   *settings++ = SEQ_CHANNEL_SETTING_CLOCK;
  

   // TODO
   /*
   SEQ_CHANNEL_SETTING_MULT_CV_SOURCE,
   SEQ_CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE,
   SEQ_CHANNEL_SETTING_SEQ_CV_SOURCE,
   SEQ_CHANNEL_SETTING_MASK_CV_SOURCE,
   */
    
    num_enabled_settings_ = settings - enabled_settings_;  
  }
  

  uint16_t update_sequence(int32_t mask_rotate_, uint8_t sequence_, uint16_t mask_) {
    
    const int sequence_num = sequence_;
    uint16_t mask = mask_;
    
    if (mask_rotate_)
      mask = OC::PatternEditor<SEQ_Channel>::RotateMask(mask, get_sequence_length(sequence_num), mask_rotate_);
    return mask;
  }

  void RenderScreensaver(weegfx::coord_t start_x, uint8_t seq_id) const;
  
private:

  bool channel_id_;
  uint16_t _sync_cnt;
  bool force_update_;
  bool force_scale_update_;
  uint16_t _ZERO;
  uint8_t clk_src_;
  bool reset_;
  bool reset_me_;
  bool reset_counter_;
  uint32_t ticks_;
  uint32_t subticks_;
  uint32_t tickjitter_;
  uint32_t clk_cnt_;
  int16_t div_cnt_;
  uint32_t ext_frequency_in_ticks_;
  uint32_t channel_frequency_in_ticks_;
  uint32_t pulse_width_in_ticks_;
  uint16_t gate_state_;
  uint16_t step_state_;
  uint32_t step_pitch_;
  uint8_t prev_multiplier_;
  uint8_t prev_pulsewidth_;
  uint8_t display_sequence_;
  uint16_t display_mask_;
  int8_t sequence_last_;
  int32_t sequence_cnt_;
  int8_t sequence_advance_;
  int8_t sequence_advance_state_;
  int last_scale_;
  uint16_t last_scale_mask_;

  int num_enabled_settings_;
  SEQ_ChannelSetting enabled_settings_[SEQ_CHANNEL_SETTING_LAST];
  braids::Quantizer quantizer_; 
  OC::DigitalInputDisplay clock_display_;
};

const char* const SEQ_CHANNEL_TRIGGER_sources[] = {
  "TR1", "TR3", " - "
};

const char* const reset_trigger_sources[] = {
  "RST2", "RST4", " - ", "=HI2", "=LO2", "=HI4", "=LO4"
};

const char* const display_multipliers[] = {
  "/64", "/48", "/32", "/16", "/8", "/7", "/6", "/5", "/4", "/3", "/2", "-", "x2", "x3", "x4", "x5", "x6", "x7", "x8"
};

const char* const cv_sources[] = {
  "--", "CV1", "CV2", "CV3", "CV4"
};

const char* const modes[] = {
  "gate", "-"
};

SETTINGS_DECLARE(SEQ_Channel, SEQ_CHANNEL_SETTING_LAST) {
 
  { 0, 0, 1, "mode", modes, settings::STORAGE_TYPE_U4 },
  { SEQ_CHANNEL_TRIGGER_TR1, 0, SEQ_CHANNEL_TRIGGER_NONE, "clock src", SEQ_CHANNEL_TRIGGER_sources, settings::STORAGE_TYPE_U4 },
  { 2, 0, SEQ_CHANNEL_TRIGGER_LAST - 1, "reset/mute", reset_trigger_sources, settings::STORAGE_TYPE_U8 },
  { MULT_BY_ONE, 0, MULT_MAX, "mult/div", display_multipliers, settings::STORAGE_TYPE_U8 },
  { 25, 0, PULSEW_MAX, "pulsewidth", OC::Strings::pulsewidth_ms, settings::STORAGE_TYPE_U8 },
  //
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names_short, settings::STORAGE_TYPE_U8 },
  { 0, -5, 5, "octave", NULL, settings::STORAGE_TYPE_I8 }, // octave
  { 65535, 1, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 }, // mask
  // seq
  { 65535, 0, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 }, // seq 1
  { 65535, 0, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 }, // seq 2
  { 65535, 0, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 }, // seq 3
  { 65535, 0, 65535, "--> edit", NULL, settings::STORAGE_TYPE_U16 }, // seq 4
  { OC::Patterns::PATTERN_USER_0_1, 0, OC::Patterns::PATTERN_USER_LAST-1, "sequence #", OC::pattern_names_short, settings::STORAGE_TYPE_U8 },
  { OC::Patterns::kMax, OC::Patterns::kMin, OC::Patterns::kMax, "sequence length", NULL, settings::STORAGE_TYPE_U8 }, // seq 1
  { OC::Patterns::kMax, OC::Patterns::kMin, OC::Patterns::kMax, "sequence length", NULL, settings::STORAGE_TYPE_U8 }, // seq 2
  { OC::Patterns::kMax, OC::Patterns::kMin, OC::Patterns::kMax, "sequence length", NULL, settings::STORAGE_TYPE_U8 }, // seq 3
  { OC::Patterns::kMax, OC::Patterns::kMin, OC::Patterns::kMax, "sequence length", NULL, settings::STORAGE_TYPE_U8 }, // seq 4
  { 0, 0, 6, "playmode", OC::Strings::seq_playmodes, settings::STORAGE_TYPE_U4 },
  // cv sources
  { 0, 0, 3, "mult/div    >>", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 3, "pulsewidth  >>", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 3, "sequence #  >>", cv_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, 3, "mask        >>", cv_sources, settings::STORAGE_TYPE_U4 },
};
  
class SEQ_State {
public:
  void Init() {
    selected_channel = 0;
    cursor.Init(SEQ_CHANNEL_SETTING_MODE, SEQ_CHANNEL_SETTING_LAST - 1);
    pattern_editor.Init();
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
  menu::ScreenCursor<menu::kScreenLines> cursor_state;
  OC::PatternEditor<SEQ_Channel> pattern_editor;
  OC::ScaleEditor<SEQ_Channel> scale_editor;
};

SEQ_State seq_state;
SEQ_Channel seq_channel[NUM_CHANNELS];

void SEQ_init() {

  ext_frequency[SEQ_CHANNEL_TRIGGER_TR1]  = 0xFFFFFFFF;
  ext_frequency[SEQ_CHANNEL_TRIGGER_TR2]  = 0xFFFFFFFF;
  ext_frequency[SEQ_CHANNEL_TRIGGER_NONE] = 0xFFFFFFFF;

  seq_state.Init();
  for (size_t i = 0; i < NUM_CHANNELS; ++i) 
    seq_channel[i].Init(static_cast<SEQ_ChannelTriggerSource>(SEQ_CHANNEL_TRIGGER_TR1), i);
  seq_state.cursor.AdjustEnd(seq_channel[0].num_enabled_settings() - 1);
}

size_t SEQ_storageSize() {
  return NUM_CHANNELS * SEQ_Channel::storageSize();
}

size_t SEQ_save(void *storage) {

  size_t used = 0;
  for (size_t i = 0; i < NUM_CHANNELS; ++i) {
    used += seq_channel[i].Save(static_cast<char*>(storage) + used);
  }
  return used;
}
size_t SEQ_restore(const void *storage) {
  size_t used = 0;
  for (size_t i = 0; i < NUM_CHANNELS; ++i) {
    used += seq_channel[i].Restore(static_cast<const char*>(storage) + used);
    seq_channel[i].update_enabled_settings(i);
  }
  seq_state.cursor.AdjustEnd(seq_channel[0].num_enabled_settings() - 1);
  return used;
}

void SEQ_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
        seq_state.cursor.set_editing(false);
        seq_state.pattern_editor.Close();
        seq_state.scale_editor.Close();
    break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
    break;
  }
}

void SEQ_loop() {
}

void SEQ_isr() {

  ticks_src1++; // src #1 ticks
  ticks_src2++; // src #2 ticks
   
  uint32_t triggers = OC::DigitalInputs::clocked();  

  if (triggers & (1 << OC::DIGITAL_INPUT_1)) {
    ext_frequency[SEQ_CHANNEL_TRIGGER_TR1] = ticks_src1;
    ticks_src1 = 0x0;
  }
  if (triggers & (1 << OC::DIGITAL_INPUT_3)) {
    ext_frequency[SEQ_CHANNEL_TRIGGER_TR2] = ticks_src2;
    ticks_src2 = 0x0;
  }

  // update channels:
  seq_channel[0].Update(triggers, DAC_CHANNEL_A);
  seq_channel[1].Update(triggers, DAC_CHANNEL_B);

  int32_t sample = OC::DAC::pitch_to_dac(DAC_CHANNEL_A, seq_channel[0].get_step_pitch(), 0);
  OC::DAC::set(DAC_CHANNEL_A, sample);
  
  sample = OC::DAC::pitch_to_dac(DAC_CHANNEL_B, seq_channel[1].get_step_pitch(), 0);
  OC::DAC::set(DAC_CHANNEL_B, sample);
  
  int32_t gate = seq_channel[0].get_step_gate();
  OC::DAC::set(DAC_CHANNEL_C, gate);
  
  gate = seq_channel[1].get_step_gate();
  OC::DAC::set(DAC_CHANNEL_D, gate);
}

void SEQ_handleButtonEvent(const UI::Event &event) {
  
  if (UI::EVENT_BUTTON_LONG_PRESS == event.type) {
     switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
         SEQ_upButtonLong();
        break;
      case OC::CONTROL_BUTTON_DOWN:
        SEQ_downButtonLong();
        break;
       case OC::CONTROL_BUTTON_L:
        if (!(seq_state.pattern_editor.active()))
          SEQ_leftButtonLong();
        break;  
      default:
        break;
     }
  }
  
  if (seq_state.pattern_editor.active()) {
    seq_state.pattern_editor.HandleButtonEvent(event);
    return;
  }
  else if (seq_state.scale_editor.active()) {
    seq_state.scale_editor.HandleButtonEvent(event);
    return;
  }
 
  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        SEQ_upButton();
        break;
      case OC::CONTROL_BUTTON_DOWN:
        SEQ_downButton();
        break;
      case OC::CONTROL_BUTTON_L:
        SEQ_leftButton();
        break;
      case OC::CONTROL_BUTTON_R:
        SEQ_rightButton();
        break;
    }
  } 
}

void SEQ_handleEncoderEvent(const UI::Event &event) {

  if (seq_state.pattern_editor.active()) {
    seq_state.pattern_editor.HandleEncoderEvent(event);
    return;
  }
  else if (seq_state.scale_editor.active()) {
    seq_state.scale_editor.HandleEncoderEvent(event);
    return;
  }
 
  if (OC::CONTROL_ENCODER_L == event.control) {

    int selected_channel = seq_state.selected_channel + event.value;
    CONSTRAIN(selected_channel, 0, NUM_CHANNELS-1);
    seq_state.selected_channel = selected_channel;

    SEQ_Channel &selected = seq_channel[seq_state.selected_channel];
      
    selected.update_enabled_settings(seq_state.selected_channel);
    seq_state.cursor.Init(SEQ_CHANNEL_SETTING_MODE, 0); 
    seq_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
    
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    
       SEQ_Channel &selected = seq_channel[seq_state.selected_channel];
    
       if (seq_state.editing()) {
        
          SEQ_ChannelSetting setting = selected.enabled_setting_at(seq_state.cursor_pos());
          
          if (SEQ_CHANNEL_SETTING_SCALE_MASK != setting || SEQ_CHANNEL_SETTING_MASK1 != setting || SEQ_CHANNEL_SETTING_MASK2 != setting || SEQ_CHANNEL_SETTING_MASK3 != setting || SEQ_CHANNEL_SETTING_MASK4 != setting) {
            
            if (selected.change_value(setting, event.value))
             selected.force_update();

            switch (setting) {
              
              case SEQ_CHANNEL_SETTING_SEQUENCE:
              {
                if (!selected.get_playmode()) {
                    uint8_t seq = selected.get_sequence();
                    selected.pattern_changed(selected.get_mask(seq));
                    selected.set_display_sequence(seq);
                } 
              }
              break;
              case SEQ_CHANNEL_SETTING_MODE:
                 selected.update_enabled_settings(seq_state.selected_channel);
                 seq_state.cursor.AdjustEnd(selected.num_enabled_settings() - 1);
              break;
             default:
              break;
            }
        }
      } else {
      seq_state.cursor.Scroll(event.value);
    }
  }
}

void SEQ_upButton() {

  SEQ_Channel &selected = seq_channel[seq_state.selected_channel];
  selected.change_value(SEQ_CHANNEL_SETTING_OCTAVE, 1);
}

void SEQ_downButton() {

  SEQ_Channel &selected = seq_channel[seq_state.selected_channel];
  selected.change_value(SEQ_CHANNEL_SETTING_OCTAVE, -1);
}

void SEQ_rightButton() {

  SEQ_Channel &selected = seq_channel[seq_state.selected_channel];
  
  switch (selected.enabled_setting_at(seq_state.cursor_pos())) {

    case SEQ_CHANNEL_SETTING_SCALE:
      seq_state.cursor.toggle_editing();
      selected.update_scale(true, 0);
    break;
    case SEQ_CHANNEL_SETTING_SCALE_MASK:
    {
     int scale = selected.get_scale();
     if (OC::Scales::SCALE_NONE != scale) {
          seq_state.scale_editor.Edit(&selected, scale);
        }
    }
    break; 
    case SEQ_CHANNEL_SETTING_MASK1:
    case SEQ_CHANNEL_SETTING_MASK2:
    case SEQ_CHANNEL_SETTING_MASK3:
    case SEQ_CHANNEL_SETTING_MASK4:
    {
      int pattern = selected.get_sequence();
      if (OC::Patterns::PATTERN_NONE != pattern) {
        seq_state.pattern_editor.Edit(&selected, pattern);
      }
    }
    break;
    default:
     seq_state.cursor.toggle_editing();
    break;
  }

}

void SEQ_leftButton() {
 
  // sync:
  for (int i = 0; i < NUM_CHANNELS; ++i) 
        seq_channel[i].sync();
}

void SEQ_leftButtonLong() {
  
}

void SEQ_upButtonLong() {

}

void SEQ_downButtonLong() {

}

void SEQ_menu() {

  menu::DualTitleBar::Draw();
  
  for (int i = 0, x = 0; i < NUM_CHANNELS; ++i, x += 21) {

    const SEQ_Channel &channel = seq_channel[i];
    menu::DualTitleBar::SetColumn(i);

    uint8_t gate = channel.get_step_state() == ON ? 15 : 1;
    menu::DualTitleBar::DrawGateIndicator(i, gate);

    graphics.movePrintPos(5, 0);
    graphics.print("#");
    graphics.print((char)('1' + i));
    graphics.movePrintPos(30, 0);
    int octave = channel.get_octave();
    if (octave >= 0)
      graphics.print("+");
    graphics.print(octave);  
  }
  
  const SEQ_Channel &channel = seq_channel[seq_state.selected_channel];
 
  menu::DualTitleBar::Selected(seq_state.selected_channel);

  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(seq_state.cursor);
  
  menu::SettingsListItem list_item;

   while (settings_list.available()) {
    const int setting =
        channel.enabled_setting_at(settings_list.Next(list_item));
    const int value = channel.get_value(setting);
    const settings::value_attr &attr = SEQ_Channel::value_attr(setting);

    switch (setting) {

      case SEQ_CHANNEL_SETTING_SCALE_MASK:
       menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, channel.get_scale_mask(), OC::Scales::GetScale(channel.get_scale()).num_notes);
       list_item.DrawNoValue<false>(value, attr);
      break; 
      case SEQ_CHANNEL_SETTING_MASK1:
      case SEQ_CHANNEL_SETTING_MASK2:
      case SEQ_CHANNEL_SETTING_MASK3:
      case SEQ_CHANNEL_SETTING_MASK4:
        menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, channel.get_display_mask(), channel.get_sequence_length(channel.get_display_sequence()), channel.get_clock_cnt());
        list_item.DrawNoValue<false>(value, attr);
      break;
      default:
        list_item.DrawDefault(value, attr);
      break;
    }
  }

  if (seq_state.pattern_editor.active())
    seq_state.pattern_editor.Draw();  
  else if (seq_state.scale_editor.active())
    seq_state.scale_editor.Draw();  
}


void SEQ_Channel::RenderScreensaver(weegfx::coord_t start_x, uint8_t seq_id) const {

      // todo ... 
      uint8_t clock_x_pos = seq_channel[seq_id].get_clock_cnt();
      int32_t _dac_value = seq_channel[seq_id].get_step_pitch();
      
      clock_x_pos = (seq_id << 6) + (clock_x_pos << 2);

      if(seq_channel[seq_id].step_state_ == OFF) {
        graphics.drawRect(clock_x_pos, 63, 5, 2);
        _dac_value = 0;
      }
      else  
        graphics.drawRect(clock_x_pos, 60, 5, 5);  

      // separate windows ...  
      graphics.drawVLine(64, 0, 68);
   
      if (_dac_value < 0) {
        // display negative values as frame (though they're not negative...)
        _dac_value = (_dac_value - (_dac_value << 1 )) >> 6;
        CONSTRAIN(_dac_value, 1, 40);
        int8_t x = 2 + clock_x_pos - (_dac_value >> 1);
        int8_t x_size = 0;
        // limit size of frame to window size
        if (seq_id && x < 64) {
          x_size = 64 - x;
          x = 64;
        }
        else if (!seq_id && (x + _dac_value > 63)) 
          x_size =  (x + _dac_value) - 64;
        // draw  
        graphics.drawFrame(x, 30 - (_dac_value >> 1), _dac_value - x_size, _dac_value);
      }
      else {
      // positive output as rectangle
        _dac_value = (_dac_value  >> 6);
        CONSTRAIN(_dac_value, 1, 40);
        
        int8_t x = 2 + clock_x_pos - (_dac_value >> 1);
        int8_t x_size = 0;
        // limit size of rectangle to window size
        if (seq_id && x < 64) {
          x_size = 64 - x;
          x = 64;
        }
        else if (!seq_id && (x + _dac_value > 63)) 
          x_size = (x + _dac_value) - 64;
        // draw  
        graphics.drawRect(x, 30 - (_dac_value >> 1), _dac_value - x_size, _dac_value);
      }
}

void SEQ_screensaver() {

  seq_channel[0].RenderScreensaver(20, 0);
  seq_channel[1].RenderScreensaver(95, 1);
}
