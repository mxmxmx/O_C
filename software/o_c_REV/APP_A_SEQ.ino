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
#include "util/util_trigger_delay.h"
#include "OC_apps.h"
#include "OC_DAC.h"
#include "OC_menus.h"
#include "OC_strings.h"
#include "OC_visualfx.h"
#include "OC_sequence_edit.h"
#include "OC_patterns.h"
#include "OC_scales.h"
#include "OC_scale_edit.h"
#include "OC_input_map.h"
#include "OC_input_maps.h"
#include "braids_quantizer.h"
#include "braids_quantizer_scales.h"
#include "extern/dspinst.h"
#include "util/util_arp.h"
#include "peaks_multistage_envelope.h"


namespace menu = OC::menu; 

const uint8_t NUM_CHANNELS = 2;
const uint8_t MULT_MAX = 26;    // max multiplier
const uint8_t MULT_BY_ONE = 19; // default multiplication  
const uint8_t PULSEW_MAX = 255; // max pulse width [ms]

const uint32_t SCALE_PULSEWIDTH = 58982; // 0.9 for signed_multiply_32x16b
const uint32_t TICKS_TO_MS = 43691; // 0.6667f : fraction, if TU_CORE_TIMER_RATE = 60 us : 65536U * ((1000 / TU_CORE_TIMER_RATE) - 16)
const uint32_t TICK_JITTER = 0xFFFFFFF;  // 1/16 : threshold/double triggers reject -> ext_frequency_in_ticks_
const uint32_t TICK_SCALE  = 0xC0000000; // 0.75 for signed_multiply_32x32               
const uint32_t COPYTIMEOUT = 200000; // in ticks

uint32_t ticks_src1 = 0; // main clock frequency (top)
uint32_t ticks_src2 = 0; // sec. clock frequency (bottom)

// copy sequence, global
uint8_t copy_sequence = 0;  
uint8_t copy_length = OC::Patterns::kMax;
uint16_t copy_mask = 0xFFFF;
uint64_t copy_timeout = COPYTIMEOUT;

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
  0xFBFFFFFF, // /63
  0xF7FFFFFF, // /62
  0xC3FFFFFF, // /49
  0xC0000000, // /48
  0xBBFFFFFF, // /47
  0x83FFFFFF, // /33
  0x80000000, // /32
  0x7BFFFFFF, // /31
  0x43FFFFFF, // /17
  0x40000000, // /16
  0x3BFFFFFF, // /15
  0x20000000, // /8
  0x1C000000, // /7
  0x18000000, // /6
  0x14000000, // /5
  0x10000000, // /4
  0xC000000,  // /3
  0x8000000,  // /2
  0x4000000,  // x1

}; // = 0xFFFFFFFF * divisor / 64

const uint8_t divisors_[] = {
   64,
   63,
   62,
   49,
   48,
   47,
   33,
   32,
   31,
   17,
   16,
   15,
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
  SEQ_CHANNEL_SETTING_TRIGGER_DELAY,
  SEQ_CHANNEL_SETTING_RESET,
  SEQ_CHANNEL_SETTING_MULT,
  SEQ_CHANNEL_SETTING_PULSEWIDTH,
  //
  SEQ_CHANNEL_SETTING_SCALE,
  SEQ_CHANNEL_SETTING_OCTAVE, 
  SEQ_CHANNEL_SETTING_ROOT,
  SEQ_CHANNEL_SETTING_OCTAVE_AUX, 
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
  SEQ_CHANNEL_SETTING_SEQUENCE_DIRECTION,
  SEQ_CHANNEL_SETTING_SEQUENCE_PLAYMODE_CV_RANGES,
  SEQ_CHANNEL_SETTING_SEQUENCE_ARP_DIRECTION,
  SEQ_CHANNEL_SETTING_SEQUENCE_ARP_RANGE,
  SEQ_CHANNEL_SETTING_BROWNIAN_PROBABILITY,
  // cv sources
  SEQ_CHANNEL_SETTING_MULT_CV_SOURCE,
  SEQ_CHANNEL_SETTING_TRANSPOSE_CV_SOURCE,
  SEQ_CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE,
  SEQ_CHANNEL_SETTING_OCTAVE_CV_SOURCE,
  SEQ_CHANNEL_SETTING_ROOT_CV_SOURCE,
  SEQ_CHANNEL_SETTING_OCTAVE_AUX_CV_SOURCE,
  SEQ_CHANNEL_SETTING_SEQ_CV_SOURCE,
  SEQ_CHANNEL_SETTING_SCALE_MASK_CV_SOURCE,
  SEQ_CHANNEL_SETTING_SEQUENCE_ARP_DIRECTION_CV_SOURCE,
  SEQ_CHANNEL_SETTING_SEQUENCE_ARP_RANGE_CV_SOURCE,
  SEQ_CHANNEL_SETTING_DIRECTION_CV_SOURCE,
  SEQ_CHANNEL_SETTING_BROWNIAN_CV_SOURCE,
  SEQ_CHANNEL_SETTING_LENGTH_CV_SOURCE,
  SEQ_CHANNEL_SETTING_ENV_ATTACK_CV_SOURCE,
  SEQ_CHANNEL_SETTING_ENV_DECAY_CV_SOURCE,
  SEQ_CHANNEL_SETTING_ENV_SUSTAIN_CV_SOURCE,
  SEQ_CHANNEL_SETTING_ENV_RELEASE_CV_SOURCE,
  SEQ_CHANNEL_SETTING_ENV_LOOPS_CV_SOURCE,
  SEQ_CHANNEL_SETTING_DUMMY,
  // aux envelope settings
  SEQ_CHANNEL_SETTING_ENV_ATTACK_DURATION,
  SEQ_CHANNEL_SETTING_ENV_ATTACK_SHAPE,
  SEQ_CHANNEL_SETTING_ENV_DECAY_DURATION,
  SEQ_CHANNEL_SETTING_ENV_DECAY_SHAPE,
  SEQ_CHANNEL_SETTING_ENV_SUSTAIN_LEVEL,
  SEQ_CHANNEL_SETTING_ENV_RELEASE_DURATION,
  SEQ_CHANNEL_SETTING_ENV_RELEASE_SHAPE,
  SEQ_CHANNEL_SETTING_ENV_MAX_LOOPS, 
  SEQ_CHANNEL_SETTING_ENV_ATTACK_RESET_BEHAVIOUR,
  SEQ_CHANNEL_SETTING_ENV_ATTACK_FALLING_GATE_BEHAVIOUR,
  SEQ_CHANNEL_SETTING_ENV_DECAY_RELEASE_RESET_BEHAVIOUR,  
  // marker   
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
  ON = 0xFFFF
};

enum MENU_PAGES {
  PARAMETERS,
  CV_MAPPING  
};

enum SEQ_UPDATE {
  ALL_OK,
  WAIT,
  CLEAR
};

enum PLAY_MODES {
  PM_NONE,
  PM_SEQ1,
  PM_SEQ2,
  PM_SEQ3,
  PM_TR1,
  PM_TR2,
  PM_TR3,
  PM_ARP,
  PM_SH1,
  PM_SH2,
  PM_SH3,
  PM_SH4,
  PM_CV1,
  PM_CV2,
  PM_CV3,
  PM_CV4,
  PM_LAST
};

enum SEQ_DIRECTIONS {
  FORWARD,
  REVERSE,
  PENDULUM1,
  PENDULUM2,
  RANDOM,
  BROWNIAN,
  SEQ_DIRECTIONS_LAST
};

enum SQ_AUX_MODES {
  GATE,
  COPY,
  ENV_AD,
  ENV_ADR,
  ENV_ADSR,
  SQ_AUX_MODES_LAST
};

uint64_t ext_frequency[SEQ_CHANNEL_TRIGGER_NONE + 1];

class SEQ_Channel : public settings::SettingsBase<SEQ_Channel, SEQ_CHANNEL_SETTING_LAST> {
public:

  uint8_t get_menu_page() const {
    return menu_page_;  
  }

  bool octave_toggle() {
    octave_toggle_ = (~octave_toggle_) & 1u;
    return octave_toggle_;
  }

  bool poke_octave_toggle() const {
    return octave_toggle_;
  }

  bool wait_for_EoS() {
    return wait_for_EoS_;
  }

  void toggle_EoS() {
    wait_for_EoS_ = (~wait_for_EoS_) & 1u;
    apply_value(SEQ_CHANNEL_SETTING_DUMMY, wait_for_EoS_);
  }

  void set_EoS_update() {
    wait_for_EoS_ = values_[SEQ_CHANNEL_SETTING_DUMMY];
  }

  void set_menu_page(uint8_t _menu_page) {
    menu_page_ = _menu_page;  
  }
  
  uint8_t get_aux_mode() const {
    return values_[SEQ_CHANNEL_SETTING_MODE];
  }

  uint8_t get_root(uint8_t DUMMY) const {
    return values_[SEQ_CHANNEL_SETTING_ROOT];
  }

  uint8_t get_clock_source() const {
    return values_[SEQ_CHANNEL_SETTING_CLOCK];
  }

  uint16_t get_trigger_delay() const {
    return values_[SEQ_CHANNEL_SETTING_TRIGGER_DELAY];
  }
  
  void set_clock_source(uint8_t _src) {
    apply_value(SEQ_CHANNEL_SETTING_CLOCK, _src);
  }

  int get_octave() const {
    return values_[SEQ_CHANNEL_SETTING_OCTAVE];
  }

  int get_octave_aux() const {
    return values_[SEQ_CHANNEL_SETTING_OCTAVE_AUX];
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
    return display_num_sequence_;
  }

  int get_direction() const {
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE_DIRECTION];
  }

  int get_direction_cv() const {
    return values_[SEQ_CHANNEL_SETTING_DIRECTION_CV_SOURCE];
  }

  int get_playmode() const {
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE_PLAYMODE];
  }

  int draw_clock() const {
    return get_playmode() != PM_ARP;
  }

  int get_arp_direction() const {
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE_ARP_DIRECTION];
  }

  int get_arp_range() const {
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE_ARP_RANGE];
  }

  int get_display_num_sequence() const {
    return display_num_sequence_;
  }

  int get_display_length() const {
    return active_sequence_length_;
  }

  void set_display_num_sequence(uint8_t seq) {
    display_num_sequence_ = seq;  
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

  uint8_t get_sequence_length(uint8_t _num_seq) const {
    
    switch (_num_seq) {

    case 0:
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE_LEN1];
    break;
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

  uint8_t get_sequence_length_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_LENGTH_CV_SOURCE];
  }
   
  uint8_t get_mult_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_MULT_CV_SOURCE];
  }

  uint8_t get_transpose_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_TRANSPOSE_CV_SOURCE];
  }
  
  uint8_t get_pulsewidth_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE];
  }

  uint8_t get_scale_mask_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_SCALE_MASK_CV_SOURCE];
  }

  uint8_t get_sequence_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_SEQ_CV_SOURCE];
  }
  
  uint8_t get_octave_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_OCTAVE_CV_SOURCE]; 
  }

  uint8_t get_root_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_ROOT_CV_SOURCE]; 
  }

  uint8_t get_octave_aux_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_OCTAVE_AUX_CV_SOURCE]; 
  }

  uint8_t get_arp_direction_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE_ARP_DIRECTION_CV_SOURCE];
  }

  uint8_t get_arp_range_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE_ARP_RANGE_CV_SOURCE];
  }

  uint8_t get_brownian_probability() const {
    return values_[SEQ_CHANNEL_SETTING_BROWNIAN_PROBABILITY];
  }

  int8_t get_brownian_probability_cv() const {
    return values_[SEQ_CHANNEL_SETTING_BROWNIAN_CV_SOURCE];
  }

  uint16_t get_attack_duration() const {
    return SCALE8_16(values_[SEQ_CHANNEL_SETTING_ENV_ATTACK_DURATION]);
  }

  int8_t get_attack_duration_cv() const {
    return values_[SEQ_CHANNEL_SETTING_ENV_ATTACK_CV_SOURCE];
  }

  peaks::EnvelopeShape get_attack_shape() const {
    return static_cast<peaks::EnvelopeShape>(values_[SEQ_CHANNEL_SETTING_ENV_ATTACK_SHAPE]);
  }

  uint16_t get_decay_duration() const {
    return SCALE8_16(values_[SEQ_CHANNEL_SETTING_ENV_DECAY_DURATION]);
  }

  int8_t get_decay_duration_cv() const {
    return values_[SEQ_CHANNEL_SETTING_ENV_DECAY_CV_SOURCE];
  }

  peaks::EnvelopeShape get_decay_shape() const {
    return static_cast<peaks::EnvelopeShape>(values_[SEQ_CHANNEL_SETTING_ENV_DECAY_SHAPE]);
  }

  uint16_t get_sustain_level() const {
    return SCALE8_16(values_[SEQ_CHANNEL_SETTING_ENV_SUSTAIN_LEVEL]);
  }

  int8_t get_sustain_level_cv() const {
    return values_[SEQ_CHANNEL_SETTING_ENV_SUSTAIN_CV_SOURCE];
  }

  uint16_t get_release_duration() const {
    return SCALE8_16(values_[SEQ_CHANNEL_SETTING_ENV_RELEASE_DURATION]);
  }

  int8_t get_release_duration_cv() const {
    return values_[SEQ_CHANNEL_SETTING_ENV_RELEASE_CV_SOURCE];
  }

  peaks::EnvelopeShape get_release_shape() const {
    return static_cast<peaks::EnvelopeShape>(values_[SEQ_CHANNEL_SETTING_ENV_RELEASE_SHAPE]);
  }

  peaks::EnvResetBehaviour get_attack_reset_behaviour() const {
    return static_cast<peaks::EnvResetBehaviour>(values_[SEQ_CHANNEL_SETTING_ENV_ATTACK_RESET_BEHAVIOUR]);
  }

  peaks::EnvFallingGateBehaviour get_attack_falling_gate_behaviour() const {
    return static_cast<peaks::EnvFallingGateBehaviour>(values_[SEQ_CHANNEL_SETTING_ENV_ATTACK_FALLING_GATE_BEHAVIOUR]);
  }

  peaks::EnvResetBehaviour get_decay_release_reset_behaviour() const {
    return static_cast<peaks::EnvResetBehaviour>(values_[SEQ_CHANNEL_SETTING_ENV_DECAY_RELEASE_RESET_BEHAVIOUR]);
  }

  uint16_t get_max_loops() const {
    return values_[SEQ_CHANNEL_SETTING_ENV_MAX_LOOPS] << 9 ;
  }

  uint8_t get_env_loops_cv_source() const {
    return values_[SEQ_CHANNEL_SETTING_ENV_LOOPS_CV_SOURCE]; 
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
  
  int get_mask(uint8_t _this_num_sequence) const {

    switch(_this_num_sequence) {
      
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

  uint32_t get_step_pitch_aux() const {
    return step_pitch_aux_;
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

  uint16_t get_rotated_scale_mask() const {
    return last_scale_mask_;
  }

  void clear_user_pattern(uint8_t seq) {
    
    uint8_t _channel_offset = !channel_id_ ? 0x0 : OC::Patterns::NUM_PATTERNS;
    memcpy(&OC::user_patterns[seq + _channel_offset], &OC::patterns[0], sizeof(OC::Pattern));
  }

  void copy_seq(uint8_t seq, uint8_t len, uint16_t mask) {

    // which sequence ?
    copy_sequence = seq + (!channel_id_ ? 0x0 : OC::Patterns::NUM_PATTERNS);
    copy_length = len;
    copy_mask = mask;
    copy_timeout = 0;
  }

  uint8_t paste_seq(uint8_t seq) {

    if (copy_timeout < COPYTIMEOUT) {

       // which sequence to copy to ?
       uint8_t sequence = seq + (!channel_id_ ? 0x0 : OC::Patterns::NUM_PATTERNS);
       // copy length:
       set_sequence_length(copy_length, seq);
       // copy mask:
       update_pattern_mask(copy_mask, seq);
       // copy note values:
       memcpy(&OC::user_patterns[sequence], &OC::user_patterns[copy_sequence], sizeof(OC::Pattern));
       // give more time for more pasting...
       copy_timeout = 0;
       
       return copy_length;
    }
    else 
      return 0;
  }

  uint8_t getTriggerState() const {
    return clock_display_.getState();
  }

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  void pattern_changed(uint16_t mask, bool force_update) {
    
    force_update_ = force_update;
    if (force_update)
      display_mask_ = mask;

    if (get_playmode() == PM_ARP) {
      // update note stack
      uint8_t seq = active_sequence_;
      arpeggiator_.UpdateArpeggiator(channel_id_, seq, get_mask(seq), get_sequence_length(seq)); 
    } 
  }

  void sync() {
    pending_sync_ = true;
  }

  void clear_CV_mapping() {
    apply_value(SEQ_CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_MULT_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_OCTAVE_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_ROOT_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_OCTAVE_AUX_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_SEQ_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_SCALE_MASK_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_TRANSPOSE_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_DIRECTION_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_SEQUENCE_ARP_DIRECTION_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_SEQUENCE_ARP_RANGE_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_BROWNIAN_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_LENGTH_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_ENV_ATTACK_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_ENV_DECAY_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_ENV_SUSTAIN_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_ENV_RELEASE_CV_SOURCE, 0);
    apply_value(SEQ_CHANNEL_SETTING_ENV_LOOPS_CV_SOURCE, 0);
  }
  
  int get_scale(uint8_t dummy) const {
    return values_[SEQ_CHANNEL_SETTING_SCALE];
  }

  void set_scale(int scale) {
     apply_value(SEQ_CHANNEL_SETTING_SCALE, scale);
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
  
  int get_scale_mask(uint8_t scale_select) const {
    return values_[SEQ_CHANNEL_SETTING_SCALE_MASK];
  }

  void scale_changed() {
    force_scale_update_ = true;
  }

  void update_scale_mask(uint16_t mask, uint16_t dummy) {
    force_scale_update_ = true;
    apply_value(SEQ_CHANNEL_SETTING_SCALE_MASK, mask); // Should automatically be updated
  }

  void update_inputmap(int num_slots, uint8_t range) {
    input_map_.Configure(OC::InputMaps::GetInputMap(num_slots), range);
  }

  int get_cv_input_range() const {
    return values_[SEQ_CHANNEL_SETTING_SEQUENCE_PLAYMODE_CV_RANGES];
  }
  
  void Init(SEQ_ChannelTriggerSource trigger_source, uint8_t id) {
    
    InitDefaults();
    trigger_delay_.Init();
    channel_id_ = id;
    octave_toggle_ = false;
    wait_for_EoS_ = false;
    note_repeat_ = false;
    menu_page_ = PARAMETERS;
    apply_value(SEQ_CHANNEL_SETTING_CLOCK, trigger_source);
    quantizer_.Init();  
    input_map_.Init();
    env_.Init();
    force_update_ = true;
    force_scale_update_ = true;
    gate_state_ = step_state_ = OFF;
    step_pitch_ = 0;
    step_pitch_aux_ = 0;
    subticks_ = 0;
    tickjitter_ = 10000;
    clk_cnt_ = 0;
    clk_src_ = get_clock_source();
    prev_reset_state_ = false;
    reset_pending_ = false;
    prev_multiplier_ = get_multiplier();
    prev_pulsewidth_ = get_pulsewidth();
    prev_input_range_ = 0;
    prev_playmode_ = get_playmode();
    pending_sync_ = false;
    sequence_change_pending_ = 0x0;
    prev_gate_raised_ = 0 ;
    env_gate_raised_ = 0 ;
    env_gate_state_ = 0 ;
 
    ext_frequency_in_ticks_ = 0xFFFFFFFF;
    channel_frequency_in_ticks_ = 0xFFFFFFFF;
    pulse_width_in_ticks_ = get_pulsewidth() << 10;

    display_num_sequence_ = get_sequence();
    display_mask_ = get_mask(display_num_sequence_);
    active_sequence_ = display_num_sequence_;
    sequence_manual_ = display_num_sequence_;
    sequence_advance_state_ = false; 
    pendulum_fwd_ = true;
    uint32_t _seed = OC::ADC::value<ADC_CHANNEL_1>() + OC::ADC::value<ADC_CHANNEL_2>() + OC::ADC::value<ADC_CHANNEL_3>() + OC::ADC::value<ADC_CHANNEL_4>();
    randomSeed(_seed);
    clock_display_.Init();
    arpeggiator_.Init();
    update_enabled_settings(0);  
  }

  bool rotate_scale(int32_t mask_rotate) {
    
    uint16_t  scale_mask = get_scale_mask(DUMMY);
    const int scale = get_scale(DUMMY);
    
    if (mask_rotate)
      scale_mask = OC::ScaleEditor<SEQ_Channel>::RotateMask(scale_mask, OC::Scales::GetScale(scale).num_notes, mask_rotate);

    if (last_scale_mask_ != scale_mask) {

      force_scale_update_ = false;  
      last_scale_ = scale;
      last_scale_mask_ = scale_mask;
      quantizer_.Configure(OC::Scales::GetScale(scale), scale_mask);
      return true;
    } else {
      return false;
    }
  }

  bool update_scale(bool force) {

    const int scale = get_scale(DUMMY);
   
    if (force || last_scale_ != scale) {
     
      uint16_t  scale_mask = get_scale_mask(DUMMY);
      force_scale_update_ = false;  
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

  bool update_timeout() {
    // wait for ~ 1.5 sec 
    return (subticks_ > 30000) ? true : false;
  }

  /* main channel update below: */
   
  inline void Update(uint32_t triggers, DAC_CHANNEL dac_channel) {

     // increment channel ticks .. 
     subticks_++; 
     
     int8_t _clock_source, _reset_source = 0x0, _aux_mode, _playmode;
     int8_t _multiplier = 0x0;
     bool _none, _triggered, _tock, _sync, _continuous;
     uint32_t _subticks = 0x0, prev_channel_frequency_in_ticks_ = 0x0;

     // core channel parameters -- 
     // 1. clock source:
     _clock_source = get_clock_source();
   
     // 2. sequencer aux channel / play modes - 
     _aux_mode = get_aux_mode();
     _playmode = get_playmode();
     _continuous = _playmode >= PM_SH1 ? true : false;

     // 3. update scale? 
     update_scale(force_scale_update_); 
     
     // clocked ?
     _none = SEQ_CHANNEL_TRIGGER_NONE == _clock_source;
     // TR1 or TR3?
     _triggered = _clock_source ? (!_none && (triggers & (1 << OC::DIGITAL_INPUT_3))) : (!_none && (triggers & (1 << OC::DIGITAL_INPUT_1)));
     _tock = false;
     _sync = false;

     // trigger delay:
     if (_playmode < PM_SH1) {
      
      trigger_delay_.Update();
      if (_triggered) 
        trigger_delay_.Push(OC::trigger_delay_ticks[get_trigger_delay()]);
        _triggered = trigger_delay_.triggered();
     }

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

     if (!_continuous) {

         _multiplier = get_multiplier();
   
         if (get_mult_cv_source()) {
            _multiplier += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_mult_cv_source() - 1)) + 127) >> 8;             
            CONSTRAIN(_multiplier, 0, MULT_MAX);
         }
        
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
         
         if (_reset_source < SEQ_CHANNEL_TRIGGER_NONE && !reset_pending_) {
    
            uint8_t reset_state_ = !_reset_source ? digitalReadFast(TR2) : digitalReadFast(TR4); // TR1, TR3 are main clock sources
    
            // ?
            if (reset_state_ < prev_reset_state_) {
               div_cnt_ = 0x0;
               reset_pending_ = true; // reset clock counter below
            }
            prev_reset_state_ = reset_state_;
         } 
    
        /*             
         *  brute force ugly sync hack:
         *  this, presumably, is needlessly complicated. 
         *  but seems to work ok-ish, w/o too much jitter and missing clocks... 
         */

         _subticks = subticks_;
         // sync? (manual)
         div_cnt_ = pending_sync_ ? 0x0 : div_cnt_;
    
         if (_multiplier <= MULT_BY_ONE && _triggered && div_cnt_ <= 0) { 
            // division, so we track
            _sync = true;
            div_cnt_ = divisors_[_multiplier]; 
            subticks_ = channel_frequency_in_ticks_; // force sync
         }
         else if (_multiplier <= MULT_BY_ONE && _triggered) {
            // division, mute output:
            step_state_ = OFF;
         }
         else if (_multiplier > MULT_BY_ONE && _triggered)  {
            // multiplication, force sync, if clocked:
            _sync = true;
            subticks_ = channel_frequency_in_ticks_; 
         }
         else if (_multiplier > MULT_BY_ONE)
            _sync = true;   
         // end of ugly hack
     }
     else { 
     // S+H mode 
        if (_playmode <= PM_SH4) {
          
          if (_triggered) {
            // new frequency (used for pulsewidth):
            channel_frequency_in_ticks_ = ext_frequency_in_ticks_;
            subticks_ = 0x0;
          }
          else
            // don't trigger, if no trigger - see below
            _continuous = _sync = false; 
        }
     }
         
     // time to output ? 
     if ((subticks_ >= channel_frequency_in_ticks_ && _sync) || _continuous) { 
       
         if (!_continuous) {
           // reset ticks       
           subticks_ = 0x0;
           //reject, if clock is too jittery or skip quasi-double triggers when ext. frequency increases:
           if (_subticks < tickjitter_ || (_subticks < prev_channel_frequency_in_ticks_ && !reset_pending_)) 
              return;
         }
         
         // resync/clear pending sync
         if (_triggered && pending_sync_) {
          pending_sync_ = false;
          clk_cnt_ = 0x0;
         }
    
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

         // mask CV ?
         if (get_scale_mask_cv_source()) {
            int16_t _rotate = (OC::ADC::value(static_cast<ADC_CHANNEL>(get_scale_mask_cv_source() - 1)) + 127) >> 8;
            rotate_scale(_rotate);
         }  
                              
         // finally, process trigger + output:
         if (process_num_seq_channel(_playmode, reset_pending_)) {

            // turn on gate
            gate_state_ = ON;
            
            int8_t _octave = get_octave();        
            if (get_octave_cv_source()) 
              _octave += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_octave_cv_source() - 1)) + 255) >> 9;

            int8_t _transpose = 0x0;
            if (get_transpose_cv_source()) {
              _transpose += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_transpose_cv_source() - 1)) + 64) >> 7;
              CONSTRAIN(_transpose, -12, 12);
            }

            int8_t _root = get_root(0x0);
            if (get_root_cv_source()) {
              _root += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_root_cv_source() - 1)) + 127) >> 8;
              CONSTRAIN(_root, 0, 11);
            }

            if (_playmode != PM_ARP) {
              // use the current sequence, updated in process_num_seq_channel():
              step_pitch_ = get_pitch_at_step(display_num_sequence_, clk_cnt_) + (_octave * 12 << 7); 
            }
            else {

              int8_t arp_range = get_arp_range();
              if (get_arp_range_cv_source()) {
                arp_range += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_arp_range_cv_source() - 1)) + 255) >> 9;
                CONSTRAIN(arp_range, 0, 4); 
              }
              arpeggiator_.set_range(arp_range);

              int8_t arp_direction = get_arp_direction();
              if (get_arp_direction_cv_source()) {
                arp_direction += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_arp_direction_cv_source() - 1)) + 255) >> 9;
                CONSTRAIN(arp_direction, 0, 4);
              } 
              arpeggiator_.set_direction(arp_direction);
              
              step_pitch_ = arpeggiator_.ClockArpeggiator() + (_octave * 12 << 7);
              // mute ? 
              if (step_pitch_ == 0xFFFFFF)
                gate_state_ = step_state_ = OFF;
            }
            // update output:
            step_pitch_ = quantizer_.Process(step_pitch_, _root << 7, _transpose);  

            int32_t _attack = get_attack_duration();        
            int32_t _decay = get_decay_duration();        
            int32_t _sustain = get_sustain_level();        
            int32_t _release = get_release_duration();        
            int32_t _loops = get_max_loops();
            
            switch (_aux_mode) {
                case ENV_AD:
                case ENV_ADR:
                case ENV_ADSR:
                  if (get_attack_duration_cv()) {
                    _attack += OC::ADC::value(static_cast<ADC_CHANNEL>(get_attack_duration_cv() - 1)) << 3;
                    USAT16(_attack) ;
                  }
                  if (get_decay_duration_cv()) {
                    _decay += OC::ADC::value(static_cast<ADC_CHANNEL>(get_decay_duration_cv() - 1)) << 3;
                    USAT16(_decay); 
                  }
                  if (get_sustain_level_cv()) {
                    _sustain += OC::ADC::value(static_cast<ADC_CHANNEL>(get_sustain_level_cv() - 1)) << 4;
                    CONSTRAIN(_sustain, 0, 65534); 
                  }
                  if (get_release_duration_cv()) {
                    _release += OC::ADC::value(static_cast<ADC_CHANNEL>(get_release_duration_cv() - 1)) << 3;
                    USAT16(_release) ;
                  }
                  if (get_env_loops_cv_source()) {
                    _loops += OC::ADC::value(static_cast<ADC_CHANNEL>(get_env_loops_cv_source() - 1)) ;
                    CONSTRAIN(_loops,1<<8, 65534) ;
                  }
                  // set the specified reset behaviours
                  env_.set_attack_reset_behaviour(get_attack_reset_behaviour());
                  env_.set_attack_falling_gate_behaviour(get_attack_falling_gate_behaviour());
                  env_.set_decay_release_reset_behaviour(get_decay_release_reset_behaviour());
                  // set number of loops
                  env_.set_max_loops(_loops);
                break;
                default:
                break;
            }

            switch (_aux_mode) {

                case COPY:
                {
                  int8_t _octave_aux = _octave + get_octave_aux();
                  if (get_octave_aux_cv_source())
                    _octave_aux += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_octave_aux_cv_source() - 1)) + 255) >> 9;  
                    
                  if (_playmode != PM_ARP)
                    step_pitch_aux_ = get_pitch_at_step(display_num_sequence_, clk_cnt_) + (_octave_aux * 12 << 7);
                  else 
                  // this *might* not be quite a copy...
                    step_pitch_aux_ = step_pitch_ + (_octave_aux * 12 << 7);
                  step_pitch_aux_ = quantizer_.Process(step_pitch_aux_, _root << 7, _transpose); 
                }
                break;
                case ENV_AD:
                {
                  env_.set_ad(_attack, _decay, 0, 2);
                  env_.set_attack_shape(get_attack_shape());
                  env_.set_decay_shape(get_decay_shape());
                }
                break;
                case ENV_ADR:
                {
                  env_.set_adr(_attack, _decay, _sustain >> 1, _release, 0, 2);
                  env_.set_attack_shape(get_attack_shape());
                  env_.set_decay_shape(get_decay_shape());
                  env_.set_release_shape(get_release_shape());
                }
                break;
                case ENV_ADSR:
                {
                  env_.set_adsr(_attack, _decay, _sustain >> 1, _release);
                  env_.set_attack_shape(get_attack_shape());
                  env_.set_decay_shape(get_decay_shape());
                  env_.set_release_shape(get_release_shape());
                }
                break;
                default:
                break;
            }
         } 
         // clear for reset:
         reset_pending_ = false;
     }
  
     /*
      *  below: pulsewidth stuff
      */
      
     if (_aux_mode != COPY && gate_state_) { 
       
        // pulsewidth setting -- 
        int16_t _pulsewidth = get_pulsewidth();
        bool _we_cannot_echo = _playmode >= PM_CV1 ? true : false;

        if (_pulsewidth || _multiplier > MULT_BY_ONE || _we_cannot_echo) {

            bool _gates = false;
            
            // do we echo && multiply? if so, do half-duty cycle:
            if (!_pulsewidth)
                _pulsewidth = PULSEW_MAX;
                
            if (_pulsewidth == PULSEW_MAX)
              _gates = true;
            // CV?
            if (get_pulsewidth_cv_source()) {

              _pulsewidth += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_pulsewidth_cv_source() - 1)) + 4) >> 3; 
              if (!_gates)          
                CONSTRAIN(_pulsewidth, 1, PULSEW_MAX);
              else // CV for 50% duty cycle: 
                CONSTRAIN(_pulsewidth, 1, (PULSEW_MAX<<1) - 55);  // incl margin, max < 2x mult. see below 
            }
            // recalculate (in ticks), if new pulsewidth setting:
            if (prev_pulsewidth_ != _pulsewidth || ! subticks_) {
                if (!_gates || _we_cannot_echo) {
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

  /* details re: sequence processing happens (mostly) here: */
  inline bool process_num_seq_channel(uint8_t playmode, uint8_t reset) {
 
      bool _out = true;
      bool _change = true;
      bool _reset = reset;
      int8_t _playmode, sequence_max, sequence_cnt, _num_seq, num_sequence_cv, sequence_length, sequence_length_cv;
      
      _num_seq = get_sequence();
      _playmode = playmode;
      sequence_max = 0x0;
      sequence_cnt = 0x0;
      num_sequence_cv = 0x0;
      sequence_length = 0x0;
      sequence_length_cv = 0x0;
      
      if (_num_seq != sequence_manual_) {
        // setting changed ... 
        if (!wait_for_EoS_) {
          _reset = true;
          if (_playmode >= PM_TR1 && _playmode <= PM_TR3)
            active_sequence_ = _num_seq;
        }
        else if (_playmode < PM_SH1)
          sequence_change_pending_ = WAIT;
      }
      sequence_manual_ = _num_seq;

      if (sequence_change_pending_ == WAIT)
        _num_seq = active_sequence_;
      else if (sequence_change_pending_ == CLEAR) {
        _reset = true;
        sequence_change_pending_ = ALL_OK;
        if (_playmode >= PM_TR1 && _playmode <= PM_TR3)
            active_sequence_ = _num_seq;
      }
      
      if (get_sequence_cv_source()) {
        num_sequence_cv = _num_seq += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_sequence_cv_source() - 1)) + 255) >> 9;
        CONSTRAIN(_num_seq, 0, OC::Patterns::PATTERN_USER_LAST - 0x1); 
      }

      if (get_sequence_length_cv_source())
        sequence_length_cv = (OC::ADC::value(static_cast<ADC_CHANNEL>(get_sequence_length_cv_source() - 1)) + 64) >> 7;
              
      switch (_playmode) {

        case PM_NONE:
        active_sequence_ = _num_seq;     
        break;
        case PM_ARP:
        sequence_change_pending_ = ALL_OK;
        sequence_length = get_sequence_length(_num_seq) + sequence_length_cv;
        CONSTRAIN(sequence_length, OC::Patterns::kMin, OC::Patterns::kMax);
        
        if (active_sequence_ != _num_seq || sequence_length != active_sequence_length_ || prev_playmode_ != _playmode)
          arpeggiator_.UpdateArpeggiator(channel_id_, _num_seq, get_mask(_num_seq), sequence_length);
        active_sequence_ = _num_seq;
        active_sequence_length_ = sequence_length;
        if (_reset)
          arpeggiator_.reset();
        prev_playmode_ = _playmode;
        // and skip the stuff below:
        _playmode = 0xFF; 
        break;
        case PM_SEQ1:
        case PM_SEQ2:
        case PM_SEQ3:
        {
         // concatenate sequences:
         sequence_max = _playmode;
              
              if (sequence_EoS_) {
                
                // increment sequence #
                sequence_cnt_ += sequence_EoS_;
                // reset sequence #
                sequence_cnt_ = sequence_cnt_ > sequence_max ? 0x0 : sequence_cnt_;
                // update 
                active_sequence_ = _num_seq + sequence_cnt_;
                // wrap around:
                if (active_sequence_ >= OC::Patterns::PATTERN_USER_LAST)
                    active_sequence_ -= OC::Patterns::PATTERN_USER_LAST;
                // reset    
                _clock(get_sequence_length(active_sequence_), 0x0, sequence_max, true); 
                _reset = true;    
              }
              else if (num_sequence_cv)  {
                active_sequence_ += num_sequence_cv;
                CONSTRAIN(active_sequence_, 0, OC::Patterns::PATTERN_USER_LAST - 1);
              }
              sequence_cnt = sequence_cnt_;
        }
        break;
        case PM_TR1:
        case PM_TR2:
        case PM_TR3:
        {  
          sequence_max = _playmode - PM_SEQ3;
          prev_playmode_ = _playmode;
          // trigger?
          uint8_t _advance_trig = (channel_id_ == DAC_CHANNEL_A) ? digitalReadFast(TR2) : digitalReadFast(TR4);
      
          if (_advance_trig < sequence_advance_state_) {
       
            // increment sequence #
            sequence_cnt_++;
            // reset sequence #
            sequence_cnt_ = sequence_cnt_ > sequence_max ? 0x0 : sequence_cnt_;
            // update 
            active_sequence_ = _num_seq + sequence_cnt_;
            // + reset
            _reset = true;   
            // wrap around:
            if (active_sequence_ >= OC::Patterns::PATTERN_USER_LAST)
                active_sequence_ -= OC::Patterns::PATTERN_USER_LAST;    
          }
          else if (num_sequence_cv)  {
              active_sequence_ += num_sequence_cv;
              CONSTRAIN(active_sequence_, 0, OC::Patterns::PATTERN_USER_LAST - 1);
          }
          sequence_advance_state_ = _advance_trig;
          sequence_max = 0x0;
        }
        break;
        case PM_SH1:
        case PM_SH2:
        case PM_SH3:
        case PM_SH4: 
        {
           int input_range;
           sequence_length = get_sequence_length(_num_seq) + sequence_length_cv;
           CONSTRAIN(sequence_length, OC::Patterns::kMin, OC::Patterns::kMax);
           input_range =  get_cv_input_range();
         
           // length or range changed ? 
           if (active_sequence_length_ != sequence_length || input_range != prev_input_range_ || prev_playmode_ != _playmode) 
              update_inputmap(sequence_length, input_range); 
           // store values:  
           active_sequence_ = _num_seq;    
           active_sequence_length_ = sequence_length;
           prev_input_range_ = input_range;
           prev_playmode_ = _playmode;
           
           // process input: 
           if (!input_range) 
              clk_cnt_ = input_map_.Process(OC::ADC::value(static_cast<ADC_CHANNEL>(_playmode - PM_SH1)));
           else    
              clk_cnt_ = input_map_.Process(0xFFF - OC::ADC::smoothed_raw_value(static_cast<ADC_CHANNEL>(_playmode - PM_SH1)));
        }
        break;
        case PM_CV1:
        case PM_CV2:
        case PM_CV3:
        case PM_CV4:
        {
           int input_range;
           sequence_length = get_sequence_length(_num_seq) + sequence_length_cv;
           CONSTRAIN(sequence_length, OC::Patterns::kMin, OC::Patterns::kMax);
           input_range =  get_cv_input_range();
           // length changed ? 
           if (active_sequence_length_ != sequence_length || input_range != prev_input_range_ || prev_playmode_ != _playmode) 
              update_inputmap(sequence_length, input_range);
           // store values:   
           active_sequence_length_ = sequence_length;
           prev_input_range_ = input_range;
           active_sequence_ = _num_seq;
           prev_playmode_ = _playmode; 
           
           // process input: 
           if (!input_range) 
              clk_cnt_ = input_map_.Process(OC::ADC::value(static_cast<ADC_CHANNEL>(_playmode - PM_CV1))); // = 5V
           else
              clk_cnt_ = input_map_.Process(0xFFF - OC::ADC::smoothed_raw_value(static_cast<ADC_CHANNEL>(_playmode - PM_CV1))); // = 10V
 
           // update output, if slot # changed:
           if (prev_slot_ == clk_cnt_) 
             _change = false;
           else {
              subticks_ = 0x0; 
              prev_slot_ = clk_cnt_;   
           }
        }
        break;
        default:
        break;
      }
      // end switch
        
      _num_seq = active_sequence_;

      if (_playmode < PM_SH1) {
  
        sequence_length = get_sequence_length(_num_seq) + sequence_length_cv;
        if (sequence_length_cv) 
            CONSTRAIN(sequence_length, OC::Patterns::kMin, OC::Patterns::kMax);
             
        CONSTRAIN(clk_cnt_, 0x0, sequence_length);
        sequence_EoS_ = _clock(sequence_length - 0x1, sequence_cnt, sequence_max, _reset); 
      } 
      
      // this is the current sequence # (USER1-USER4):
      display_num_sequence_ = _num_seq;
      // and corresponding pattern mask:
      display_mask_ = get_mask(_num_seq);
      // ... and the length
      active_sequence_length_ = sequence_length;
                 
      // slot at current position:  
      if (_playmode != PM_ARP) {
        _out = (display_mask_ >> clk_cnt_) & 1u;
        step_state_ = _out ? ON : OFF;  
        _out = (_out && _change) ? true : false; 
      }
      else {
         step_state_ = ON;
         clk_cnt_ = 0x0;
      }
      // return step:  
      return _out; 
  }

  // update sequencer clock, return -1, 0, 1 when EoS is reached:

  int8_t _clock(uint8_t sequence_length, uint8_t sequence_count, uint8_t sequence_max, bool _reset) {

    int8_t EoS = 0x0, _clk_cnt, _direction;
    bool reset = _reset;
        
    _clk_cnt = clk_cnt_;
    _direction = get_direction();

    if (get_direction_cv()) {
       _direction += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_direction_cv() - 1)) + 255) >> 9;
       CONSTRAIN(_direction, 0, SEQ_DIRECTIONS_LAST - 0x1);
    }
    
    switch (_direction) {

      case FORWARD:
      {
        _clk_cnt++;
        if (reset)
          _clk_cnt = 0x0;
        // end of sequence?  
        else if (_clk_cnt > sequence_length)
          _clk_cnt = 0x0;
        else if (_clk_cnt == sequence_length) {
           EoS = 0x1;  
        }
      }
      break;
      case REVERSE:
      {
        _clk_cnt--;
        if (reset)
          _clk_cnt = sequence_length;
        // end of sequence? 
        else if (_clk_cnt < 0) 
          _clk_cnt = sequence_length;
        else if (!_clk_cnt)  
           EoS = 0x1;
      }
      break;
      case PENDULUM1:
      case BROWNIAN:
      if (BROWNIAN == _direction) {
        // Compare Brownian probability and reverse direction if needed
        int16_t brown_prb = get_brownian_probability();

        if (get_brownian_probability_cv()) {
          brown_prb += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_brownian_probability_cv() - 1)) + 8) >> 3;
          CONSTRAIN(brown_prb, 0, 256);
        }
        if (random(0,256) < brown_prb) 
          pendulum_fwd_ = !pendulum_fwd_; 
      }
      {
        if (pendulum_fwd_) {
          _clk_cnt++;  
          if (reset)
            _clk_cnt = 0x0;
          else if (_clk_cnt >= sequence_length) {
            
            if (sequence_count >= sequence_max) {
              pendulum_fwd_ = false;
              _clk_cnt = sequence_length;
            }
            else EoS = 0x1;
            // pendulum needs special care (when PM_NONE)
            if (!pendulum_fwd_ && sequence_change_pending_ == WAIT) sequence_change_pending_ = CLEAR;
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
              pendulum_fwd_ = true;
              _clk_cnt = 0x0;
            }
            else EoS = -0x1;
            if (pendulum_fwd_ && sequence_change_pending_ == WAIT) sequence_change_pending_ = CLEAR;
          } 
        }
      }
      break;
      case PENDULUM2:
      {
        if (pendulum_fwd_) {

          if (!note_repeat_)
            _clk_cnt++;
          note_repeat_ = false;
           
          if (reset)
            _clk_cnt = 0x0;
          else if (_clk_cnt >= sequence_length) {
            // end of sequence ? 
            if (sequence_count >= sequence_max) {
              pendulum_fwd_ = false;
              _clk_cnt = sequence_length;  
              note_repeat_ = true; // repeat last step
            }
            else EoS = 0x1;
            if (!pendulum_fwd_ && sequence_change_pending_ == WAIT) sequence_change_pending_ = CLEAR;
          }
        }
        // reverse direction:
        else {
          
          if (!note_repeat_)
            _clk_cnt--; 
          note_repeat_ = false;
          
          if (reset)  
            _clk_cnt = sequence_length;
          else if (_clk_cnt <= 0x0) {
            // end of sequence ? 
            if (sequence_count == 0x0) {
              pendulum_fwd_ = true;
              _clk_cnt = 0x0; 
              note_repeat_ = true; // repeat first step
            }
            else EoS = -0x1;
            if (pendulum_fwd_ && sequence_change_pending_ == WAIT) sequence_change_pending_ = CLEAR;
          }
        }
      }
      break;
      case RANDOM:
      _clk_cnt = random(sequence_length + 0x1);
      if (reset)
        _clk_cnt = 0x0;
      // jump to next sequence if we happen to hit the last note:  
      else if (_clk_cnt >= sequence_length)
        EoS = random(0x2);  
      break;
      default:
      break;
    }
    clk_cnt_ = _clk_cnt;
    
    if (EoS && sequence_change_pending_ == WAIT) sequence_change_pending_ = CLEAR;
    return EoS;
  }

  SEQ_ChannelSetting enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  void update_enabled_settings(uint8_t channel_id) {

    SEQ_ChannelSetting *settings = enabled_settings_;

    switch(get_menu_page()) {

      case PARAMETERS: {

          *settings++ = SEQ_CHANNEL_SETTING_SCALE;
          *settings++ = SEQ_CHANNEL_SETTING_SCALE_MASK;
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
         
         if (get_playmode() < PM_SH1) {
          
             *settings++ = (get_playmode() == PM_ARP) ? SEQ_CHANNEL_SETTING_SEQUENCE_ARP_DIRECTION : SEQ_CHANNEL_SETTING_SEQUENCE_DIRECTION;
             if (get_playmode() == PM_ARP)
               *settings++ = SEQ_CHANNEL_SETTING_SEQUENCE_ARP_RANGE;
             else if (get_direction() == BROWNIAN)       
               *settings++ = SEQ_CHANNEL_SETTING_BROWNIAN_PROBABILITY;  
             *settings++ = SEQ_CHANNEL_SETTING_MULT;
         }
         else 
            *settings++ = SEQ_CHANNEL_SETTING_SEQUENCE_PLAYMODE_CV_RANGES;
            
         *settings++ = SEQ_CHANNEL_SETTING_OCTAVE;
         *settings++ = SEQ_CHANNEL_SETTING_ROOT;    
         // aux output:   
         *settings++ = SEQ_CHANNEL_SETTING_MODE;
         
         switch (get_aux_mode()) {    
            case GATE: 
              *settings++ = SEQ_CHANNEL_SETTING_PULSEWIDTH;
            break;
            case COPY: 
              *settings++ = SEQ_CHANNEL_SETTING_OCTAVE_AUX;
            break;
            case ENV_AD: 
              *settings++ = SEQ_CHANNEL_SETTING_ENV_ATTACK_DURATION;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_ATTACK_SHAPE;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_DECAY_DURATION;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_DECAY_SHAPE;
              *settings++ = SEQ_CHANNEL_SETTING_PULSEWIDTH;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_MAX_LOOPS;
            break;
            case ENV_ADR: 
              *settings++ = SEQ_CHANNEL_SETTING_ENV_ATTACK_DURATION;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_ATTACK_SHAPE;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_DECAY_DURATION;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_DECAY_SHAPE;
              *settings++ = SEQ_CHANNEL_SETTING_PULSEWIDTH;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_SUSTAIN_LEVEL;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_RELEASE_DURATION;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_RELEASE_SHAPE;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_MAX_LOOPS;
            break;
            case ENV_ADSR: 
              *settings++ = SEQ_CHANNEL_SETTING_ENV_ATTACK_DURATION;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_ATTACK_SHAPE;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_DECAY_DURATION;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_DECAY_SHAPE;
              *settings++ = SEQ_CHANNEL_SETTING_PULSEWIDTH;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_SUSTAIN_LEVEL;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_RELEASE_DURATION;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_RELEASE_SHAPE;
            break;
            default:
            break; 
         }
         
         if (get_playmode() < PM_SH1) {
           *settings++ = SEQ_CHANNEL_SETTING_RESET; 
           *settings++ = SEQ_CHANNEL_SETTING_CLOCK;
         }
      }
      break;  
      
      case CV_MAPPING: {
        
         *settings++ = SEQ_CHANNEL_SETTING_TRANSPOSE_CV_SOURCE;  // = transpose SCALE
         *settings++ = SEQ_CHANNEL_SETTING_SCALE_MASK_CV_SOURCE; // = rotate mask 
         *settings++ = SEQ_CHANNEL_SETTING_SEQ_CV_SOURCE; // sequence #

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
         
         *settings++ = SEQ_CHANNEL_SETTING_LENGTH_CV_SOURCE; // = playmode
         
         if (get_playmode() < PM_SH1) {
            
            if (get_playmode() == PM_ARP) {
               *settings++ = SEQ_CHANNEL_SETTING_SEQUENCE_ARP_DIRECTION_CV_SOURCE; 
               *settings++ = SEQ_CHANNEL_SETTING_SEQUENCE_ARP_RANGE_CV_SOURCE; 
            }
            else *settings++ = SEQ_CHANNEL_SETTING_DIRECTION_CV_SOURCE; // = directions
            
            if (get_playmode() != PM_ARP && get_direction() == BROWNIAN)       
               *settings++ = SEQ_CHANNEL_SETTING_BROWNIAN_CV_SOURCE;
               
            *settings++ = SEQ_CHANNEL_SETTING_MULT_CV_SOURCE;
           
         }
         else 
            *settings++ = SEQ_CHANNEL_SETTING_DUMMY; // = range

         *settings++ = SEQ_CHANNEL_SETTING_OCTAVE_CV_SOURCE;
         *settings++ = SEQ_CHANNEL_SETTING_ROOT_CV_SOURCE;   
         *settings++ = SEQ_CHANNEL_SETTING_DUMMY; // = mode

         switch (get_aux_mode()) {
      
            case GATE: 
              *settings++ = SEQ_CHANNEL_SETTING_PULSEWIDTH_CV_SOURCE;
            break;
            case COPY: 
              *settings++ = SEQ_CHANNEL_SETTING_OCTAVE_AUX_CV_SOURCE;
            break;
            case ENV_AD:
              *settings++ = SEQ_CHANNEL_SETTING_ENV_ATTACK_CV_SOURCE;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_DECAY_CV_SOURCE;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_LOOPS_CV_SOURCE;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_ATTACK_RESET_BEHAVIOUR;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_ATTACK_FALLING_GATE_BEHAVIOUR;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_DECAY_RELEASE_RESET_BEHAVIOUR;
            break;
            case ENV_ADR:
            case ENV_ADSR:
              *settings++ = SEQ_CHANNEL_SETTING_ENV_ATTACK_CV_SOURCE;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_DECAY_CV_SOURCE;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_SUSTAIN_CV_SOURCE;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_RELEASE_CV_SOURCE;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_LOOPS_CV_SOURCE;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_ATTACK_RESET_BEHAVIOUR;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_ATTACK_FALLING_GATE_BEHAVIOUR;
              *settings++ = SEQ_CHANNEL_SETTING_ENV_DECAY_RELEASE_RESET_BEHAVIOUR;
            break;
            default:
            break; 
         }

         if (get_playmode() < PM_SH1) {
           *settings++ =  SEQ_CHANNEL_SETTING_TRIGGER_DELAY; // 
           *settings++ =  SEQ_CHANNEL_SETTING_CLOCK; // = reset source
         }

         *settings++ = SEQ_CHANNEL_SETTING_DUMMY; // = mode
         *settings++ = SEQ_CHANNEL_SETTING_DUMMY; // = mode
         *settings++ = SEQ_CHANNEL_SETTING_DUMMY; // = mode
         *settings++ = SEQ_CHANNEL_SETTING_DUMMY; // = mode

      }
      break;
      default:
      break;
   } 
   num_enabled_settings_ = settings - enabled_settings_;  
  }
  
  template <DAC_CHANNEL dacChannel>
  void update_main_channel() {
    int32_t _output = OC::DAC::pitch_to_scaled_voltage_dac(dacChannel, get_step_pitch(), 0, OC::DAC::get_voltage_scaling(dacChannel));
    OC::DAC::set<dacChannel>(_output);  
    
  }
  
  template <DAC_CHANNEL dacChannel>
  void update_aux_channel()
  {

      int8_t _mode = get_aux_mode();
      uint32_t _output = 0;
    
      switch (_mode) {

        case GATE: // gate
          #ifdef BUCHLA_4U
          _output = (get_step_gate() == ON) ? OC::DAC::get_octave_offset(dacChannel, OCTAVES - OC::DAC::kOctaveZero - 0x2) : OC::DAC::get_zero_offset(dacChannel);
          #else
          _output = (get_step_gate() == ON) ? OC::DAC::get_octave_offset(dacChannel, OCTAVES - OC::DAC::kOctaveZero - 0x1) : OC::DAC::get_zero_offset(dacChannel);
          #endif
        break;
        case COPY: // copy
          _output = OC::DAC::pitch_to_scaled_voltage_dac(dacChannel, get_step_pitch_aux(), 0, OC::DAC::get_voltage_scaling(dacChannel));
        break;
        // code to process envelopes here
        case  ENV_AD:
        case ENV_ADR:
        case ENV_ADSR:
          env_gate_state_ = 0;
          env_gate_raised_ = (get_step_gate() == ON);
          if (env_gate_raised_ && !prev_gate_raised_)
             env_gate_state_ |= peaks::CONTROL_GATE_RISING;
          if (env_gate_raised_)
             env_gate_state_ |= peaks::CONTROL_GATE;
          else if (prev_gate_raised_) 
            env_gate_state_ |= peaks::CONTROL_GATE_FALLING;
          prev_gate_raised_ = env_gate_raised_;   
          _output = OC::DAC::get_zero_offset(dacChannel) + env_.ProcessSingleSample(env_gate_state_);
        break;
        default:
        break;
      }
      OC::DAC::set<dacChannel>(_output);    
  }

  void RenderScreensaver() const;
  
private:

  bool channel_id_;
  bool octave_toggle_;
  bool wait_for_EoS_;
  bool note_repeat_;
  bool sequence_EoS_;
  uint8_t menu_page_;
  uint16_t _sync_cnt;
  bool force_update_;
  bool force_scale_update_;
  uint8_t clk_src_;
  bool prev_reset_state_;
  bool reset_pending_;
  uint32_t subticks_;
  uint32_t tickjitter_;
  int32_t clk_cnt_;
  int32_t prev_slot_;
  int16_t div_cnt_;
  uint32_t ext_frequency_in_ticks_;
  uint32_t channel_frequency_in_ticks_;
  uint32_t pulse_width_in_ticks_;
  uint16_t gate_state_;
  uint8_t prev_gate_raised_;
  uint8_t env_gate_state_;
  uint8_t env_gate_raised_;
  uint16_t step_state_;
  int32_t step_pitch_;
  int32_t step_pitch_aux_;
  uint8_t prev_multiplier_;
  uint8_t prev_pulsewidth_;
  uint8_t display_num_sequence_;
  uint16_t display_mask_;
  int8_t active_sequence_;
  int8_t sequence_manual_;
  int8_t active_sequence_length_;
  int32_t sequence_cnt_;
  int8_t sequence_advance_state_;
  int8_t sequence_change_pending_;
  int8_t pendulum_fwd_;
  int last_scale_;
  uint16_t last_scale_mask_;
  uint8_t prev_input_range_;
  uint8_t prev_playmode_;
  bool pending_sync_;

  util::TriggerDelay<OC::kMaxTriggerDelayTicks> trigger_delay_;
  util::Arpeggiator arpeggiator_;
  
  int num_enabled_settings_;
  SEQ_ChannelSetting enabled_settings_[SEQ_CHANNEL_SETTING_LAST];
  braids::Quantizer quantizer_; 
  OC::Input_Map input_map_;
  OC::DigitalInputDisplay clock_display_;
  peaks::MultistageEnvelope env_;
 
};

const char* const SEQ_CHANNEL_TRIGGER_sources[] = {
  "TR1", "TR3", " - "
};

const char* const reset_trigger_sources[] = {
  "RST2", "RST4", " - ", "=HI2", "=LO2", "=HI4", "=LO4"
};

const char* const display_multipliers[] = {
  "/64", "/63", "/62", "/49", "/48", "/47", "/33", "/32", "/31", "/17", "/16", "/15", "/8", "/7", "/6", "/5", "/4", "/3", "/2", "-", "x2", "x3", "x4", "x5", "x6", "x7", "x8"
};

const char* const modes[] = {
  "gate", "copy", "AD", "ADR", "ADSR",
};

const char* const cv_ranges[] = {
  " 5V", "10V"
};

const char* const arp_directions[] = {
  "up", "down", "u/d", "rnd"
};

const char* const arp_range[] = {
  "1", "2", "3", "4"
};

SETTINGS_DECLARE(SEQ_Channel, SEQ_CHANNEL_SETTING_LAST) {
 
  { 0, 0, 4, "aux. mode", modes, settings::STORAGE_TYPE_U4 },
  { SEQ_CHANNEL_TRIGGER_TR1, 0, SEQ_CHANNEL_TRIGGER_NONE, "clock src", SEQ_CHANNEL_TRIGGER_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, OC::kNumDelayTimes - 1, "trigger delay", OC::Strings::trigger_delay_times, settings::STORAGE_TYPE_U8 },
  { 2, 0, SEQ_CHANNEL_TRIGGER_LAST - 1, "reset/mute", reset_trigger_sources, settings::STORAGE_TYPE_U8 },
  { MULT_BY_ONE, 0, MULT_MAX, "mult/div", display_multipliers, settings::STORAGE_TYPE_U8 },
  { 25, 0, PULSEW_MAX, "--> pw", NULL, settings::STORAGE_TYPE_U8 },
  //
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "scale", OC::scale_names_short, settings::STORAGE_TYPE_U8 },
  { 0, -5, 5, "octave", NULL, settings::STORAGE_TYPE_I8 }, // octave
  { 0, 0, 11, "root", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 },
  { 0, -5, 5, "--> aux +/-", NULL, settings::STORAGE_TYPE_I8 }, // aux octave
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
  { 0, 0, PM_LAST - 1, "playmode", OC::Strings::seq_playmodes, settings::STORAGE_TYPE_U8 },
  { 0, 0, SEQ_DIRECTIONS_LAST - 1, "direction", OC::Strings::seq_directions, settings::STORAGE_TYPE_U8 },
  { 0, 0, 1, "CV adr. range", cv_ranges, settings::STORAGE_TYPE_U4 },
  { 0, 0, 3, "direction", arp_directions, settings::STORAGE_TYPE_U4 },
  { 0, 0, 3, "arp.range", arp_range, settings::STORAGE_TYPE_U8 },
  { 64, 0, 255, "-->brown prob", NULL, settings::STORAGE_TYPE_U8 },
  // cv sources
  { 0, 0, 4, "mult/div CV ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "transpose   ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "--> pw      ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "octave  +/- ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "root    +/- ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "--> aux +/- ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "sequence #  ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "mask rotate ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "direction   ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "arp.range   ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "direction   ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "-->brwn.prb ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "seq.length  ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "att dur  ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "dec dur  ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "sus lvl  ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "rel dur  ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "env loops ->", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U8 },
  { 0, 0, 1, "-", NULL, settings::STORAGE_TYPE_U4 }, // DUMMY, use to store update behaviour
  // envelope parameters
  { 128, 0, 255, "--> att dur", NULL, settings::STORAGE_TYPE_U8 },
  { peaks::ENV_SHAPE_QUARTIC, peaks::ENV_SHAPE_LINEAR, peaks::ENV_SHAPE_LAST - 1, "--> att shape", OC::Strings::envelope_shapes, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "--> dec dur", NULL, settings::STORAGE_TYPE_U8 },
  { peaks::ENV_SHAPE_EXPONENTIAL, peaks::ENV_SHAPE_LINEAR, peaks::ENV_SHAPE_LAST - 1, "--> dec shape", OC::Strings::envelope_shapes, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "--> sus lvl", NULL, settings::STORAGE_TYPE_U16 },
  { 128, 0, 255, "--> rel dur", NULL, settings::STORAGE_TYPE_U8 },
  { peaks::ENV_SHAPE_EXPONENTIAL, peaks::ENV_SHAPE_LINEAR, peaks::ENV_SHAPE_LAST - 1, "--> rel shape", OC::Strings::envelope_shapes, settings::STORAGE_TYPE_U16 },
  {1, 1, 127, "--> loops", NULL, settings::STORAGE_TYPE_U8 },
  { peaks::RESET_BEHAVIOUR_NULL, peaks::RESET_BEHAVIOUR_NULL, peaks::RESET_BEHAVIOUR_LAST - 1, "att reset", OC::Strings::reset_behaviours, settings::STORAGE_TYPE_U8 },
  { peaks::FALLING_GATE_BEHAVIOUR_IGNORE, peaks::FALLING_GATE_BEHAVIOUR_IGNORE, peaks::FALLING_GATE_BEHAVIOUR_LAST - 1, "att fall gt", OC::Strings::falling_gate_behaviours, settings::STORAGE_TYPE_U8 },
  { peaks::RESET_BEHAVIOUR_SEGMENT_PHASE, peaks::RESET_BEHAVIOUR_NULL, peaks::RESET_BEHAVIOUR_LAST - 1, "dec/rel reset", OC::Strings::reset_behaviours, settings::STORAGE_TYPE_U8 },  
};
  
class SEQ_State {
public:
  void Init() {
    selected_channel = 0;
    cursor.Init(SEQ_CHANNEL_SETTING_MODE, SEQ_CHANNEL_SETTING_LAST - 1);
    pattern_editor.Init();
    scale_editor.Init(false);
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
    // update display
    seq_channel[i].pattern_changed(seq_channel[i].get_mask(seq_channel[i].get_sequence()), true);
    seq_channel[i].set_display_num_sequence(seq_channel[i].get_sequence()); 
    seq_channel[i].update_enabled_settings(i);
    seq_channel[i].set_EoS_update();
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
    {  
       SEQ_Channel &selected = seq_channel[seq_state.selected_channel];
       selected.set_menu_page(PARAMETERS);
       selected.update_enabled_settings(seq_state.selected_channel);
    }
    break;
  }
}

void SEQ_loop() {
}

void SEQ_isr() {

  ticks_src1++; // src #1 ticks
  ticks_src2++; // src #2 ticks
  copy_timeout++;
   
  uint32_t triggers = OC::DigitalInputs::clocked();  

  if (triggers & (1 << OC::DIGITAL_INPUT_1)) {
    ext_frequency[SEQ_CHANNEL_TRIGGER_TR1] = ticks_src1;
    ticks_src1 = 0x0;
  }
  if (triggers & (1 << OC::DIGITAL_INPUT_3)) {
    ext_frequency[SEQ_CHANNEL_TRIGGER_TR2] = ticks_src2;
    ticks_src2 = 0x0;
  }

  // update sequencer channels 1, 2:
  seq_channel[0].Update(triggers, DAC_CHANNEL_A);
  seq_channel[1].Update(triggers, DAC_CHANNEL_B);
  // update DAC channels A, B: 
  seq_channel[0].update_main_channel<DAC_CHANNEL_A>();
  seq_channel[1].update_main_channel<DAC_CHANNEL_B>();
  // update DAC channels C, D: 
  seq_channel[0].update_aux_channel<DAC_CHANNEL_C>();
  seq_channel[1].update_aux_channel<DAC_CHANNEL_D>();
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
                uint8_t seq = selected.get_sequence();
                uint8_t playmode = selected.get_playmode();
                // details: update mask/sequence, depending on mode.
                if (!playmode || playmode >= PM_CV1 || selected.get_current_sequence() == seq || selected.update_timeout()) {
                  selected.set_display_num_sequence(seq); 
                  selected.pattern_changed(selected.get_mask(seq), true); 
                }
              }
              break;
              case SEQ_CHANNEL_SETTING_MODE:
              case SEQ_CHANNEL_SETTING_SEQUENCE_PLAYMODE:
              case SEQ_CHANNEL_SETTING_SEQUENCE_DIRECTION:
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
  
  if (selected.get_menu_page() == PARAMETERS) {

    if (selected.octave_toggle())
      selected.change_value(SEQ_CHANNEL_SETTING_OCTAVE, 1);
    else 
      selected.change_value(SEQ_CHANNEL_SETTING_OCTAVE, -1);
  }
  else  {
    selected.set_menu_page(PARAMETERS);
    selected.update_enabled_settings(seq_state.selected_channel);
    seq_state.cursor.set_editing(false);
  }
}

void SEQ_downButton() {
  
  SEQ_Channel &selected = seq_channel[seq_state.selected_channel];

  if (!seq_state.pattern_editor.active() && !seq_state.scale_editor.active()) { 
    
      uint8_t _menu_page = selected.get_menu_page();
    
      switch (_menu_page) {  
        
        case PARAMETERS:  
        _menu_page = CV_MAPPING;
        break;
        default:
        _menu_page = PARAMETERS;
        break;
        
      }       
      
      selected.set_menu_page(_menu_page);
      selected.update_enabled_settings(seq_state.selected_channel);
      seq_state.cursor.set_editing(false);
  }
  /*
  SEQ_Channel &selected = seq_channel[seq_state.selected_channel];
  if (selected.get_menu_page() == PARAMETERS) 
    selected.change_value(SEQ_CHANNEL_SETTING_OCTAVE, -1);
  else {
    selected.set_menu_page(PARAMETERS);
    selected.update_enabled_settings(seq_state.selected_channel);
    seq_state.cursor.set_editing(false);
  }
  */
}

void SEQ_rightButton() {

  SEQ_Channel &selected = seq_channel[seq_state.selected_channel];
  
  switch (selected.enabled_setting_at(seq_state.cursor_pos())) {

    case SEQ_CHANNEL_SETTING_SCALE:
      seq_state.cursor.toggle_editing();
      selected.update_scale(true);
    break;
    case SEQ_CHANNEL_SETTING_SCALE_MASK:
    {
     int scale = selected.get_scale(DUMMY);
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
    case SEQ_CHANNEL_SETTING_DUMMY:
      selected.set_menu_page(PARAMETERS);
      selected.update_enabled_settings(seq_state.selected_channel);
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
  // copy scale
  if (!seq_state.pattern_editor.active() && !seq_state.scale_editor.active()) {
    
      uint8_t this_channel, the_other_channel, scale;
      uint16_t mask;
      
      this_channel = seq_state.selected_channel;
      scale = seq_channel[this_channel].get_scale(DUMMY);
      mask = seq_channel[this_channel].get_rotated_scale_mask();
      
      the_other_channel = (~this_channel) & 1u;
      seq_channel[the_other_channel].set_scale(scale);
      seq_channel[the_other_channel].update_scale_mask(mask, DUMMY);
      seq_channel[the_other_channel].update_scale(true);
  }
}

void SEQ_upButtonLong() {
  // screensaver short cut (happens elsewhere)
}

void SEQ_downButtonLong() {
  // clear CV mappings:
  SEQ_Channel &selected = seq_channel[seq_state.selected_channel];
  
  if (selected.get_menu_page() == CV_MAPPING) {
    selected.clear_CV_mapping();
    seq_state.cursor.set_editing(false);
  }
  else { // toggle update behaviour:
    seq_channel[0x0].toggle_EoS();
    seq_channel[0x1].toggle_EoS();
  }
}

void SEQ_menu() {

  menu::DualTitleBar::Draw();
  
  for (int i = 0, x = 0; i < NUM_CHANNELS; ++i, x += 21) {

    const SEQ_Channel &channel = seq_channel[i];
    menu::DualTitleBar::SetColumn(i);

    // draw gate/step indicator
    uint8_t gate = 1;
    if (channel.get_step_gate() == ON) 
      gate += 14;
    else if (channel.get_step_state() == ON) 
      gate += 10;
    menu::DualTitleBar::DrawGateIndicator(i, gate);

    graphics.movePrintPos(5, 0);
    // channel id:
    graphics.print("#");
    graphics.print((char)('A' + i));
    // sequence id:
    graphics.print("/");
    graphics.print(1 + channel.get_display_num_sequence());
    // octave:
    graphics.movePrintPos(22, 0);
    if (channel.poke_octave_toggle())
      graphics.print("+");
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

      case SEQ_CHANNEL_SETTING_SCALE:
        list_item.SetPrintPos();
        if (list_item.editing) {
          menu::DrawEditIcon(6, list_item.y, value, attr);
          graphics.movePrintPos(6, 0);
        }
        graphics.print(OC::scale_names[value]);
        list_item.DrawCustom();
        break;
      case SEQ_CHANNEL_SETTING_SCALE_MASK:
       menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, channel.get_rotated_scale_mask(), OC::Scales::GetScale(channel.get_scale(DUMMY)).num_notes);
       list_item.DrawNoValue<false>(value, attr);
      break; 
      case SEQ_CHANNEL_SETTING_MASK1:
      case SEQ_CHANNEL_SETTING_MASK2:
      case SEQ_CHANNEL_SETTING_MASK3:
      case SEQ_CHANNEL_SETTING_MASK4:
      {
        int clock_indicator = channel.get_clock_cnt();
        if (channel.get_playmode() == PM_ARP)
          clock_indicator = 0xFF;
        menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, channel.get_display_mask(), channel.get_display_length(), clock_indicator);
        list_item.DrawNoValue<false>(value, attr);
      }
      break;
      case SEQ_CHANNEL_SETTING_PULSEWIDTH:
        if (channel.get_aux_mode() < ENV_AD) {
          list_item.Draw_PW_Value(value, attr);
        } else {
          list_item.Draw_PW_Value_Char(value, attr, "--> sus dur");
        }
      break;
      case SEQ_CHANNEL_SETTING_DUMMY:
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


void SEQ_Channel::RenderScreensaver() const {

      uint8_t seq_id = channel_id_;
      uint8_t clock_x_pos = seq_channel[seq_id].get_clock_cnt();
      int32_t _dac_value = seq_channel[seq_id].get_step_pitch();
      int32_t _dac_overflow = 0, _dac_overflow2 = 0;

      // reposition ARP:
      if (seq_channel[seq_id].get_playmode() == PM_ARP)
        clock_x_pos = 0x6 + (seq_id << 2);
        
      clock_x_pos = (seq_id << 6) + (clock_x_pos << 2);

      // clock/step indicator:
      if(seq_channel[seq_id].step_state_ == OFF) {
        graphics.drawRect(clock_x_pos, 63, 5, 2);
        _dac_value = 0;
      }
      else  
        graphics.drawRect(clock_x_pos, 60, 5, 5);  

      // separate windows ...  
      graphics.drawVLine(64, 0, 68);

      // display pitch values as squares/frames:
      if (_dac_value < 0) {
        // display negative values as frame (though they're not negative...)
        _dac_value = (_dac_value - (_dac_value << 1 )) >> 6;
        _dac_overflow = _dac_value - 40;
        _dac_overflow2 = _dac_overflow - 40;
        
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
        
        if (_dac_overflow > 0) {
          
            CONSTRAIN(_dac_overflow, 1, 40);
  
            x = 2 + clock_x_pos - (_dac_overflow >> 1);
            
            if (seq_id && x < 64) {
              x_size = 64 - x;
              x = 64;
            }
            else if (!seq_id && (x + _dac_overflow > 63)) 
              x_size =  (x + _dac_overflow) - 64;
  
            graphics.drawRect(x, 30 - (_dac_overflow >> 1), _dac_overflow - x_size, _dac_overflow);
         }
         
         if (_dac_overflow2 > 0) {
          
            CONSTRAIN(_dac_overflow2, 1, 40);
  
            x = 2 + clock_x_pos - (_dac_overflow2 >> 1);
            
            if (seq_id && x < 64) {
              x_size = 64 - x;
              x = 64;
            }
            else if (!seq_id && (x + _dac_overflow2 > 63)) 
              x_size =  (x + _dac_overflow2) - 64;
  
            graphics.clearRect(x, 30 - (_dac_overflow2 >> 1), _dac_overflow2 - x_size, _dac_overflow2);
         }
      }
      else {
      // positive output as rectangle
        _dac_value = (_dac_value  >> 6);
        _dac_overflow = _dac_value - 40;
        _dac_overflow2 = _dac_overflow - 40;
        
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

        if (_dac_overflow > 0) {

          CONSTRAIN(_dac_overflow, 1, 40);

          x = 2 + clock_x_pos - (_dac_overflow >> 1);
          
          if (seq_id && x < 64) {
            x_size = 64 - x;
            x = 64;
          }
          else if (!seq_id && (x + _dac_overflow > 63)) 
             x_size = (x + _dac_overflow) - 64;
          
          graphics.clearRect(x, 30 - (_dac_overflow >> 1), _dac_overflow - x_size, _dac_overflow);
        }
        
        if (_dac_overflow2 > 0) {

          CONSTRAIN(_dac_overflow2, 1, 40);

          x = 2 + clock_x_pos - (_dac_overflow2 >> 1);
          
          if (seq_id && x < 64) {
            x_size = 64 - x;
            x = 64;
          }
          else if (!seq_id && (x + _dac_overflow2 > 63)) 
             x_size = (x + _dac_overflow2) - 64;
          
          graphics.drawRect(x, 30 - (_dac_overflow2 >> 1), _dac_overflow2 - x_size, _dac_overflow2);
        }
    }
}

void SEQ_screensaver() {

  seq_channel[0].RenderScreensaver();
  seq_channel[1].RenderScreensaver();
}

