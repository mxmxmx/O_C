// Copyright (c) 2015, 2016 Patrick Dowling, Tim Churches
//
// Initial app implementation: Patrick Dowling (pld@gurkenkiste.com)
// Modifications by: Tim Churches (tim.churches@gmail.com)
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
#include "util/util_logistic_map.h"
#include "util/util_settings.h"
#include "util/util_trigger_delay.h"
#include "util/util_turing.h"
#include "util/util_integer_sequences.h"
#include "peaks_bytebeat.h"
#include "braids_quantizer.h"
#include "braids_quantizer_scales.h"
#include "OC_menus.h"
#include "OC_scales.h"
#include "OC_scale_edit.h"
#include "OC_strings.h"


#ifdef BUCHLA_4U
 #define QQ_OFFSET_X 20
#else
 #define QQ_OFFSET_X 31
#endif

enum ChannelSetting {
  CHANNEL_SETTING_SCALE,
  CHANNEL_SETTING_ROOT,
  CHANNEL_SETTING_MASK,
  CHANNEL_SETTING_SOURCE,
  CHANNEL_SETTING_AUX_SOURCE_DEST,
  CHANNEL_SETTING_TRIGGER,
  CHANNEL_SETTING_CLKDIV,
  CHANNEL_SETTING_DELAY,
  CHANNEL_SETTING_TRANSPOSE,
  CHANNEL_SETTING_OCTAVE,
  CHANNEL_SETTING_FINE,
  CHANNEL_SETTING_TURING_LENGTH,
  CHANNEL_SETTING_TURING_PROB,
  CHANNEL_SETTING_TURING_MODULUS,
  CHANNEL_SETTING_TURING_RANGE,
  CHANNEL_SETTING_TURING_PROB_CV_SOURCE,
  CHANNEL_SETTING_TURING_MODULUS_CV_SOURCE,
  CHANNEL_SETTING_TURING_RANGE_CV_SOURCE,
  CHANNEL_SETTING_LOGISTIC_MAP_R,
  CHANNEL_SETTING_LOGISTIC_MAP_RANGE,
  CHANNEL_SETTING_LOGISTIC_MAP_R_CV_SOURCE,
  CHANNEL_SETTING_LOGISTIC_MAP_RANGE_CV_SOURCE,
  CHANNEL_SETTING_BYTEBEAT_EQUATION,
  CHANNEL_SETTING_BYTEBEAT_RANGE,
  CHANNEL_SETTING_BYTEBEAT_P0,
  CHANNEL_SETTING_BYTEBEAT_P1,
  CHANNEL_SETTING_BYTEBEAT_P2,
  CHANNEL_SETTING_BYTEBEAT_EQUATION_CV_SOURCE,
  CHANNEL_SETTING_BYTEBEAT_RANGE_CV_SOURCE,
  CHANNEL_SETTING_BYTEBEAT_P0_CV_SOURCE,
  CHANNEL_SETTING_BYTEBEAT_P1_CV_SOURCE,
  CHANNEL_SETTING_BYTEBEAT_P2_CV_SOURCE, 
  CHANNEL_SETTING_INT_SEQ_INDEX,
  CHANNEL_SETTING_INT_SEQ_MODULUS,
  CHANNEL_SETTING_INT_SEQ_RANGE,
  CHANNEL_SETTING_INT_SEQ_DIRECTION,
  CHANNEL_SETTING_INT_SEQ_BROWNIAN_PROB,
  CHANNEL_SETTING_INT_SEQ_LOOP_START,
  CHANNEL_SETTING_INT_SEQ_LOOP_LENGTH,
  CHANNEL_SETTING_INT_SEQ_FRAME_SHIFT_PROB,
  CHANNEL_SETTING_INT_SEQ_FRAME_SHIFT_RANGE,
  CHANNEL_SETTING_INT_SEQ_STRIDE,
  CHANNEL_SETTING_INT_SEQ_INDEX_CV_SOURCE,
  CHANNEL_SETTING_INT_SEQ_MODULUS_CV_SOURCE,
  CHANNEL_SETTING_INT_SEQ_RANGE_CV_SOURCE,
  CHANNEL_SETTING_INT_SEQ_STRIDE_CV_SOURCE,
  CHANNEL_SETTING_INT_SEQ_RESET_TRIGGER,
  CHANNEL_SETTING_LAST
};

enum ChannelTriggerSource {
  CHANNEL_TRIGGER_TR1,
  CHANNEL_TRIGGER_TR2,
  CHANNEL_TRIGGER_TR3,
  CHANNEL_TRIGGER_TR4,
  CHANNEL_TRIGGER_CONTINUOUS_UP,
  CHANNEL_TRIGGER_CONTINUOUS_DOWN,
  CHANNEL_TRIGGER_LAST
};

enum ChannelSource {
  CHANNEL_SOURCE_CV1,
  CHANNEL_SOURCE_CV2,
  CHANNEL_SOURCE_CV3,
  CHANNEL_SOURCE_CV4,
  CHANNEL_SOURCE_TURING,
  CHANNEL_SOURCE_LOGISTIC_MAP,
  CHANNEL_SOURCE_BYTEBEAT,
  CHANNEL_SOURCE_INT_SEQ,
  CHANNEL_SOURCE_LAST
};

enum QQ_CV_DEST {
  QQ_DEST_NONE,
  QQ_DEST_ROOT,
  QQ_DEST_OCTAVE,
  QQ_DEST_TRANSPOSE,
  QQ_DEST_MASK,
  QQ_DEST_LAST
};

class QuantizerChannel : public settings::SettingsBase<QuantizerChannel, CHANNEL_SETTING_LAST> {
public:

  int get_scale(uint8_t dummy) const {
    return values_[CHANNEL_SETTING_SCALE];
  }

  void set_scale(int scale) {
    if (scale != get_scale(DUMMY)) {
      const OC::Scale &scale_def = OC::Scales::GetScale(scale);
      uint16_t mask = get_mask();
      if (0 == (mask & ~(0xffff << scale_def.num_notes)))
        mask |= 0x1;
      apply_value(CHANNEL_SETTING_MASK, mask);
      apply_value(CHANNEL_SETTING_SCALE, scale);
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
  
  int get_root() const {
    return values_[CHANNEL_SETTING_ROOT];
  }

  int get_root(uint8_t DUMMY) const {
    return 0x0;
  }

  uint16_t get_mask() const {
    return values_[CHANNEL_SETTING_MASK];
  }

  uint16_t get_rotated_scale_mask() const {
    return last_mask_;
  }

  ChannelSource get_source() const {
    return static_cast<ChannelSource>(values_[CHANNEL_SETTING_SOURCE]);
  }

  ChannelTriggerSource get_trigger_source() const {
    return static_cast<ChannelTriggerSource>(values_[CHANNEL_SETTING_TRIGGER]);
  }

  uint8_t get_channel_index() const {
    return channel_index_;
  }

  uint8_t get_clkdiv() const {
    return values_[CHANNEL_SETTING_CLKDIV];
  }

  uint16_t get_trigger_delay() const {
    return values_[CHANNEL_SETTING_DELAY];
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

  uint8_t get_aux_cv_dest() const {
    return values_[CHANNEL_SETTING_AUX_SOURCE_DEST];
  }

  uint8_t get_turing_length() const {
    return values_[CHANNEL_SETTING_TURING_LENGTH];
  }

  uint8_t get_turing_prob() const {
    return values_[CHANNEL_SETTING_TURING_PROB];
  }

  uint8_t get_turing_modulus() const {
    return values_[CHANNEL_SETTING_TURING_MODULUS];
  }

  uint8_t get_turing_range() const {
    return values_[CHANNEL_SETTING_TURING_RANGE];
  }

  uint8_t get_turing_prob_cv_source() const {
    return values_[CHANNEL_SETTING_TURING_PROB_CV_SOURCE];
  }

  uint8_t get_turing_modulus_cv_source() const {
    return values_[CHANNEL_SETTING_TURING_MODULUS_CV_SOURCE];
  }

  uint8_t get_turing_range_cv_source() const {
    return values_[CHANNEL_SETTING_TURING_RANGE_CV_SOURCE];
  }

  uint8_t get_logistic_map_r() const {
    return values_[CHANNEL_SETTING_LOGISTIC_MAP_R];
  }

  uint8_t get_logistic_map_range() const {
    return values_[CHANNEL_SETTING_LOGISTIC_MAP_RANGE];
  }

  uint8_t get_logistic_map_r_cv_source() const {
    return values_[CHANNEL_SETTING_LOGISTIC_MAP_R_CV_SOURCE];
  }

  uint8_t get_logistic_map_range_cv_source() const {
    return values_[CHANNEL_SETTING_LOGISTIC_MAP_RANGE_CV_SOURCE];
  }

  uint8_t get_bytebeat_equation() const {
    return values_[CHANNEL_SETTING_BYTEBEAT_EQUATION];
  }

  uint8_t get_bytebeat_range() const {
    return values_[CHANNEL_SETTING_BYTEBEAT_RANGE];
  }

  uint8_t get_bytebeat_p0() const {
    return values_[CHANNEL_SETTING_BYTEBEAT_P0];
  }

  uint8_t get_bytebeat_p1() const {
    return values_[CHANNEL_SETTING_BYTEBEAT_P1];
  }

  uint8_t get_bytebeat_p2() const {
    return values_[CHANNEL_SETTING_BYTEBEAT_P2];
  }

  uint8_t get_bytebeat_equation_cv_source() const {
    return values_[CHANNEL_SETTING_BYTEBEAT_EQUATION_CV_SOURCE];
  }

  uint8_t get_bytebeat_range_cv_source() const {
    return values_[CHANNEL_SETTING_BYTEBEAT_RANGE_CV_SOURCE];
  }

  uint8_t get_bytebeat_p0_cv_source() const {
    return values_[CHANNEL_SETTING_BYTEBEAT_P0_CV_SOURCE];
  }

  uint8_t get_bytebeat_p1_cv_source() const {
    return values_[CHANNEL_SETTING_BYTEBEAT_P1_CV_SOURCE];
  }

  uint8_t get_bytebeat_p2_cv_source() const {
    return values_[CHANNEL_SETTING_BYTEBEAT_P2_CV_SOURCE];
  }

  uint8_t get_int_seq_index() const {
    return values_[CHANNEL_SETTING_INT_SEQ_INDEX];
  }

  uint8_t get_int_seq_modulus() const {
    return values_[CHANNEL_SETTING_INT_SEQ_MODULUS];
  }

  uint8_t get_int_seq_range() const {
    return values_[CHANNEL_SETTING_INT_SEQ_RANGE];
  }

  int16_t get_int_seq_start() const {
    return static_cast<int16_t>(values_[CHANNEL_SETTING_INT_SEQ_LOOP_START]);
  }

  void set_int_seq_start(uint8_t start_pos) {
    values_[CHANNEL_SETTING_INT_SEQ_LOOP_START] = start_pos;
  }

  int16_t get_int_seq_length() const {
    return static_cast<int16_t>(values_[CHANNEL_SETTING_INT_SEQ_LOOP_LENGTH] - 1);
  }

  bool get_int_seq_dir() const {
    return static_cast<bool>(values_[CHANNEL_SETTING_INT_SEQ_DIRECTION]);
  }

  int16_t get_int_seq_brownian_prob() const {
    return static_cast<int16_t>(values_[CHANNEL_SETTING_INT_SEQ_BROWNIAN_PROB]);
  }

  uint8_t get_int_seq_index_cv_source() const {
    return values_[CHANNEL_SETTING_INT_SEQ_INDEX_CV_SOURCE];
  }

  uint8_t get_int_seq_modulus_cv_source() const {
    return values_[CHANNEL_SETTING_INT_SEQ_MODULUS_CV_SOURCE];
  }

  uint8_t get_int_seq_range_cv_source() const {
    return values_[CHANNEL_SETTING_INT_SEQ_RANGE_CV_SOURCE];
  }

  uint8_t get_int_seq_frame_shift_prob() const {
    return values_[CHANNEL_SETTING_INT_SEQ_FRAME_SHIFT_PROB];
  }

  uint8_t get_int_seq_frame_shift_range() const {
    return values_[CHANNEL_SETTING_INT_SEQ_FRAME_SHIFT_RANGE];
  }

  uint8_t get_int_seq_stride() const {
    return values_[CHANNEL_SETTING_INT_SEQ_STRIDE];
  }

  uint8_t get_int_seq_stride_cv_source() const {
    return values_[CHANNEL_SETTING_INT_SEQ_STRIDE_CV_SOURCE];
  }

  ChannelTriggerSource get_int_seq_reset_trigger_source() const {
    return static_cast<ChannelTriggerSource>(values_[CHANNEL_SETTING_INT_SEQ_RESET_TRIGGER]);
  }

  void clear_dest() {
    // ...
    schedule_mask_rotate_ = 0x0;
    continuous_offset_ = 0x0;
    prev_transpose_cv_ = 0x0;
    prev_transpose_cv_ = 0x0;
    prev_root_cv_ = 0x0;          
  }

  void Init(ChannelSource source, ChannelTriggerSource trigger_source) {
    InitDefaults();
    apply_value(CHANNEL_SETTING_SOURCE, source);
    apply_value(CHANNEL_SETTING_TRIGGER, trigger_source);

    channel_index_ = source;
    force_update_ = true;
    instant_update_ = false;
    last_scale_ = -1;
    last_mask_ = 0;
    last_sample_ = 0;
    clock_ = 0;
    int_seq_reset_ = false;
    continuous_offset_ = false;
    schedule_mask_rotate_ = false;
    prev_octave_cv_ = 0;
    prev_transpose_cv_ = 0;
    prev_root_cv_ = 0;
    prev_destination_ = 0;

    trigger_delay_.Init();
    turing_machine_.Init();
    logistic_map_.Init();
    bytebeat_.Init();
    int_seq_.Init(get_int_seq_start(), get_int_seq_length());
    quantizer_.Init();
    update_scale(true, false);
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

  inline void Update(uint32_t triggers, DAC_CHANNEL dac_channel) {
    
    uint8_t index = channel_index_;

    ChannelSource source = get_source();
    ChannelTriggerSource trigger_source = get_trigger_source();
    bool continuous = CHANNEL_TRIGGER_CONTINUOUS_UP == trigger_source || CHANNEL_TRIGGER_CONTINUOUS_DOWN == trigger_source;
    bool triggered = !continuous &&
      (triggers & DIGITAL_INPUT_MASK(trigger_source - CHANNEL_TRIGGER_TR1));

    if (source == CHANNEL_SOURCE_INT_SEQ) {
      ChannelTriggerSource int_seq_reset_trigger_source = get_int_seq_reset_trigger_source() ;
      int_seq_reset_ = (triggers & DIGITAL_INPUT_MASK(int_seq_reset_trigger_source - 1));
    }
    
    trigger_delay_.Update();
    if (triggered)
      trigger_delay_.Push(OC::trigger_delay_ticks[get_trigger_delay()]);
    triggered = trigger_delay_.triggered();

    if (triggered) {
      ++clock_;
      if (clock_ >= get_clkdiv()) {
        clock_ = 0;
      } else {
        triggered = false;
      }
    }

    bool update = continuous || triggered;
    
    if (update)
      update_scale(force_update_, schedule_mask_rotate_);

    int32_t sample = last_sample_;
    int32_t temp_sample = 0;
    int32_t history_sample = 0;


    switch (source) {
      case CHANNEL_SOURCE_TURING: {
          // this doesn't make sense when continuously quantizing; should be hidden via the menu ... 
          if (continuous)
            break;
            
          turing_machine_.set_length(get_turing_length());
          int32_t probability = get_turing_prob();
          if (get_turing_prob_cv_source()) {
            probability += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_turing_prob_cv_source() - 1)) + 7) >> 4;
            CONSTRAIN(probability, 0, 255);
          }
          turing_machine_.set_probability(probability);  
          if (triggered) {
            uint32_t shift_register = turing_machine_.Clock();
            uint8_t range = get_turing_range();
            if (get_turing_range_cv_source()) {
              range += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_turing_range_cv_source() - 1)) + 15) >> 5;
              CONSTRAIN(range, 1, 120);
            }

            if (quantizer_.enabled()) {
    
              uint8_t modulus = get_turing_modulus();
              if (get_turing_modulus_cv_source()) {
                 modulus += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_turing_modulus_cv_source() - 1)) + 15) >> 5;
                 CONSTRAIN(modulus, 2, 121);
              }
 
              // Since our range is limited anyway, just grab the last byte for lengths > 8,
              // otherwise scale to use bits. And apply the modulus
              uint32_t shift = turing_machine_.length();
              uint32_t scaled = (shift_register & 0xff) * range;
              scaled = (scaled >> (shift > 7 ? 8 : shift)) % modulus;
     
              // The quantizer uses a lookup codebook with 128 entries centered
              // about 0, so we use the range/scaled output to lookup a note
              // directly instead of changing to pitch first.
              int32_t pitch =
                  quantizer_.Lookup(64 + range / 2 - scaled + get_transpose()) + (get_root() << 7);
              sample = OC::DAC::pitch_to_scaled_voltage_dac(dac_channel, pitch, get_octave(), OC::DAC::get_voltage_scaling(dac_channel));
              history_sample = pitch + ((OC::DAC::kOctaveZero + get_octave()) * 12 << 7);
            } else {
              // Scale range by 128, so 12 steps = 1V
              // We dont' need a calibrated value here, really.
              uint32_t scaled = multiply_u32xu32_rshift(range << 7, shift_register, get_turing_length());
              scaled += get_transpose() << 7;
              sample = OC::DAC::pitch_to_scaled_voltage_dac(dac_channel, scaled, get_octave(), OC::DAC::get_voltage_scaling(dac_channel));
              history_sample = scaled + ((OC::DAC::kOctaveZero + get_octave()) * 12 << 7);
             }
          }
        }
        break;
      case CHANNEL_SOURCE_BYTEBEAT: {
           // this doesn't make sense when continuously quantizing; should be hidden via the menu ... 
            if (continuous)
              break;
              
            int32_t bytebeat_eqn = get_bytebeat_equation() << 12;
            if (get_bytebeat_equation_cv_source()) {
              bytebeat_eqn += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_bytebeat_equation_cv_source() - 1)) << 4);
              bytebeat_eqn = USAT16(bytebeat_eqn);
            }
            bytebeat_.set_equation(bytebeat_eqn);

            int32_t bytebeat_p0 = get_bytebeat_p0() << 8;
            if (get_bytebeat_p0_cv_source()) {
              bytebeat_p0 += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_bytebeat_p0_cv_source() - 1)) << 4);
              bytebeat_p0 = USAT16(bytebeat_p0);
            }
            bytebeat_.set_p0(bytebeat_p0);

            int32_t bytebeat_p1 = get_bytebeat_p1() << 8;
            if (get_bytebeat_p1_cv_source()) {
              bytebeat_p1 += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_bytebeat_p1_cv_source() - 1)) << 4);
              bytebeat_p1 = USAT16(bytebeat_p1);
            }
            bytebeat_.set_p1(bytebeat_p1);

            int32_t bytebeat_p2 = get_bytebeat_p2() << 8;
            if (get_bytebeat_p2_cv_source()) {
              bytebeat_p2 += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_bytebeat_p2_cv_source() - 1)) << 4);
              bytebeat_p2 = USAT16(bytebeat_p2);
            }
            bytebeat_.set_p2(bytebeat_p2);
            
            if (triggered) {
              uint32_t bb = bytebeat_.Clock();
              uint8_t range = get_bytebeat_range();
              if (get_bytebeat_range_cv_source()) {
                range += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_bytebeat_range_cv_source() - 1)) + 15) >> 5;
                CONSTRAIN(range, 1, 120);
              }

              if (quantizer_.enabled()) {
    
                // Since our range is limited anyway, just grab the last byte
                uint32_t scaled = ((bb >> 8) * range) >> 8;
    
                // The quantizer uses a lookup codebook with 128 entries centered
                // about 0, so we use the range/scaled output to lookup a note
                // directly instead of changing to pitch first.
                int32_t pitch =
                  quantizer_.Lookup(64 + range / 2 - scaled + get_transpose()) + (get_root() << 7);
                sample = OC::DAC::pitch_to_scaled_voltage_dac(dac_channel, pitch, get_octave(), OC::DAC::get_voltage_scaling(dac_channel));
                history_sample = pitch + ((OC::DAC::kOctaveZero + get_octave()) * 12 << 7);
              } else {
                // We dont' need a calibrated value here, really
                int octave = get_octave();
                CONSTRAIN(octave, 0, 6);
                sample = OC::DAC::get_octave_offset(dac_channel, octave) + (get_transpose() << 7); 
                // range is actually 120 (10 oct) but 65535 / 128 is close enough
                sample += multiply_u32xu32_rshift32((static_cast<uint32_t>(range) * 65535U) >> 7, bb << 16);
                sample = USAT16(sample);
                history_sample = sample;
              }
            }
          }
          break;        
      case CHANNEL_SOURCE_LOGISTIC_MAP: {
          // this doesn't make sense when continuously quantizing; should be hidden via the menu ... 
          if (continuous)
            break;
            
          logistic_map_.set_seed(123);
          int32_t logistic_map_r = get_logistic_map_r();
          if (get_logistic_map_r_cv_source()) {
            logistic_map_r += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_logistic_map_r_cv_source() - 1)) + 7) >> 4;
            CONSTRAIN(logistic_map_r, 0, 255);
          }
          logistic_map_.set_r(logistic_map_r);  
          if (triggered) {
            int64_t logistic_map_x = logistic_map_.Clock();
            uint8_t range = get_logistic_map_range();
            if (get_logistic_map_range_cv_source()) {
              range += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_logistic_map_range_cv_source() - 1)) + 15) >> 5;
              CONSTRAIN(range, 1, 120);
            }
            
            if (quantizer_.enabled()) {   
              uint32_t logistic_scaled = (logistic_map_x * range) >> 24;

              // See above, may need tweaking    
              int32_t pitch =
                  quantizer_.Lookup(64 + range / 2 - logistic_scaled + get_transpose()) + (get_root() << 7);
              sample = OC::DAC::pitch_to_scaled_voltage_dac(dac_channel, pitch, get_octave(), OC::DAC::get_voltage_scaling(dac_channel));
              history_sample = pitch + ((OC::DAC::kOctaveZero + get_octave()) * 12 << 7);
            } else {
              int octave = get_octave();
              CONSTRAIN(octave, 0, 6);
              sample = OC::DAC::get_octave_offset(dac_channel, octave) + (get_transpose() << 7);
              sample += multiply_u32xu32_rshift24((static_cast<uint32_t>(range) * 65535U) >> 7, logistic_map_x);
              sample = USAT16(sample);
              history_sample = sample;
            }
          }
        }
        break;
      case CHANNEL_SOURCE_INT_SEQ: {
            // this doesn't make sense when continuously quantizing; should be hidden via the menu ... 
            if (continuous)
              break;
            
            int_seq_.set_loop_direction(get_int_seq_dir());
            int_seq_.set_brownian_prob(get_int_seq_brownian_prob());
            int16_t int_seq_index = get_int_seq_index();
            int16_t int_seq_stride = get_int_seq_stride();

            if (get_int_seq_index_cv_source()) {
              int_seq_index += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_int_seq_index_cv_source() - 1)) + 127) >> 8;
            }
            if (int_seq_index < 0) int_seq_index = 0;
            if (int_seq_index > 8) int_seq_index = 8;
            int_seq_.set_int_seq(int_seq_index);
            int16_t int_seq_modulus_ = get_int_seq_modulus();
            if (get_int_seq_modulus_cv_source()) {
                int_seq_modulus_ += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_int_seq_modulus_cv_source() - 1)) + 31) >> 6;
                CONSTRAIN(int_seq_modulus_, 2, 121);
            }
            int_seq_.set_int_seq_modulus(int_seq_modulus_);

            if (get_int_seq_stride_cv_source()) {
              int_seq_stride += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_int_seq_stride_cv_source() - 1)) + 31) >> 6;
            }
            if (int_seq_stride < 1) int_seq_stride = 1;
            if (int_seq_stride > kIntSeqLen - 1) int_seq_stride = kIntSeqLen - 1;
            int_seq_.set_fractal_stride(int_seq_stride);

            int_seq_.set_loop_start(get_int_seq_start());

            int_seq_.set_loop_length(get_int_seq_length());

            if (int_seq_reset_) {
              int_seq_.reset_loop();
              int_seq_reset_ = false;
            }

            if (triggered) {
              // uint32_t is = int_seq_.Clock();
              // check whether frame should be shifted and if so, by how much.
              if (get_int_seq_pass_go()) {
                // OK, we're at the start of a loop or at one end of a pendulum swing
                uint8_t fs_prob = get_int_seq_frame_shift_prob();
                uint8_t fs_range = get_int_seq_frame_shift_range();
                // Serial.print("fs_prob=");
                // Serial.println(fs_prob);
                // Serial.print("fs_range=");
                // Serial.println(fs_range);
                uint8_t fs_rand = static_cast<uint8_t>(random(0,256)) ;
                // Serial.print("fs_rand=");
                // Serial.println(fs_rand);
                // Serial.println("---"); 
                if (fs_rand < fs_prob) {
                  // OK, move the frame!
                  int16_t frame_shift = random(-fs_range, fs_range + 1) ;
                  // Serial.print("frame_shift=");
                  // Serial.println(frame_shift);
                  // Serial.print("current start pos=");
                  // Serial.println(get_int_seq_start());
                  int16_t new_start_pos = get_int_seq_start() + frame_shift ;
                  // Serial.print("new_start_pos=");
                  // Serial.println(new_start_pos);
                  // Serial.println("==="); 
                  if (new_start_pos < 0) new_start_pos = 0;
                  if (new_start_pos > kIntSeqLen - 2) new_start_pos = kIntSeqLen - 2;
                  set_int_seq_start(static_cast<uint8_t>(new_start_pos)) ;
                  int_seq_.set_loop_start(get_int_seq_start());                  
                }
              }
              uint32_t is = int_seq_.Clock();
              int16_t range_ = get_int_seq_range();
              if (get_int_seq_range_cv_source()) {
                range_ += (OC::ADC::value(static_cast<ADC_CHANNEL>(get_int_seq_range_cv_source() - 1)) + 31) >> 6;
                CONSTRAIN(range_, 1, 120);
              }
              if (quantizer_.enabled()) {
    
                // Since our range is limited anyway, just grab the last byte
                uint32_t scaled = ((is >> 4) * range_) >> 8;
    
                // The quantizer uses a lookup codebook with 128 entries centered
                // about 0, so we use the range/scaled output to lookup a note
                // directly instead of changing to pitch first.
                int32_t pitch =
                  quantizer_.Lookup(64 + range_ / 2 - scaled + get_transpose()) + (get_root() << 7);
                sample = OC::DAC::pitch_to_scaled_voltage_dac(dac_channel, pitch, get_octave(), OC::DAC::get_voltage_scaling(dac_channel));
                history_sample = pitch + ((OC::DAC::kOctaveZero + get_octave()) * 12 << 7);
              } else {
                // We dont' need a calibrated value here, really
                int octave = get_octave();
                CONSTRAIN(octave, 0, 6);
                sample = OC::DAC::get_octave_offset(dac_channel, octave) + (get_transpose() << 7); 
                // range is actually 120 (10 oct) but 65535 / 128 is close enough
                sample += multiply_u32xu32_rshift32((static_cast<uint32_t>(range_) * 65535U) >> 7, is << 20);
                sample = USAT16(sample);
                history_sample = sample;
              }
            }
          }
          break;        

      default: {
          if (update) {
            
            int32_t transpose = get_transpose() + prev_transpose_cv_;
            int octave = get_octave() + prev_octave_cv_;
            int root = get_root() + prev_root_cv_;

            int32_t pitch = quantizer_.enabled()
                ? OC::ADC::raw_pitch_value(static_cast<ADC_CHANNEL>(source))
                : OC::ADC::pitch_value(static_cast<ADC_CHANNEL>(source));

            // repurpose channel CV input? -- 
            uint8_t _aux_cv_destination = get_aux_cv_dest();

            if (_aux_cv_destination != prev_destination_)
              clear_dest();
            prev_destination_ = _aux_cv_destination;
              
            if (!continuous && index != source) {
              // this doesn't really work all that well for continuous quantizing...
              // see below
          
              switch(_aux_cv_destination) {

                case QQ_DEST_NONE:
                break;
                case QQ_DEST_TRANSPOSE:
                  transpose += (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) + 63) >> 7;
                break;
                case QQ_DEST_ROOT:
                  root += (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) + 127) >> 8;
                break;
                case QQ_DEST_OCTAVE:
                  octave += (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) + 255) >> 9;
                break;
                case  QQ_DEST_MASK:
                  update_scale(false, (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) + 127) >> 8);
                break;
                default:
                break;
              }
            }

            // limit:
            CONSTRAIN(octave, -4, 4);
            CONSTRAIN(root, 0, 11);
            CONSTRAIN(transpose, -12, 12); 
            
            int32_t quantized = quantizer_.Process(pitch, root << 7, transpose);
            sample = temp_sample = OC::DAC::pitch_to_scaled_voltage_dac(dac_channel, quantized, octave + continuous_offset_, OC::DAC::get_voltage_scaling(dac_channel));

            // continuous mode needs special treatment to give useful results.
            // basically, update on note change only
            
            if (continuous && last_sample_ != sample) {

              bool _re_quantize = false;
              int _aux_cv = 0;

              if (index != source) {
     
                  switch(_aux_cv_destination) {
                    
                    case QQ_DEST_NONE:
                    break;
                    case QQ_DEST_TRANSPOSE:
                      _aux_cv = (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) + 63) >> 7;
                      if (_aux_cv != prev_transpose_cv_) {
                          transpose = get_transpose() + _aux_cv;
                          CONSTRAIN(transpose, -12, 12); 
                          prev_transpose_cv_ = _aux_cv;
                          _re_quantize = true;
                      }
                    break;
                    case QQ_DEST_ROOT:
                      _aux_cv = (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) + 127) >> 8;
                      if (_aux_cv != prev_root_cv_) {
                          root = get_root() + _aux_cv;
                          CONSTRAIN(root, 0, 11);
                          prev_root_cv_ = _aux_cv;
                          _re_quantize = true;
                      }
                    break;
                    case QQ_DEST_OCTAVE:
                      _aux_cv = (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) + 255) >> 9;
                      if (_aux_cv != prev_octave_cv_) {
                          octave = get_octave() + _aux_cv;
                          CONSTRAIN(octave, -4, 4);
                          prev_octave_cv_ = _aux_cv;
                          _re_quantize = true;
                      }
                    break;   
                    case QQ_DEST_MASK:
                      schedule_mask_rotate_ = (OC::ADC::value(static_cast<ADC_CHANNEL>(index)) + 127) >> 8;
                      update_scale(force_update_, schedule_mask_rotate_);
                    break;
                    default:
                    break; 
                  } 
                  // end switch
              }
                
              // offset when TR source = continuous ?
              int8_t _trigger_offset = 0;
              bool _trigger_update = false;
              if (OC::DigitalInputs::read_immediate(static_cast<OC::DigitalInput>(index))) {
                 _trigger_offset = (trigger_source == CHANNEL_TRIGGER_CONTINUOUS_UP) ? 1 : -1;
              }
              if (_trigger_offset != continuous_offset_)
                 _trigger_update = true;
              continuous_offset_ = _trigger_offset;
                 
              // run quantizer again -- presumably could be made more efficient...
              if (_re_quantize) 
                quantized = quantizer_.Process(pitch, root << 7, transpose);
              if (_re_quantize || _trigger_update) 
                sample = OC::DAC::pitch_to_scaled_voltage_dac(dac_channel, quantized, octave + continuous_offset_, OC::DAC::get_voltage_scaling(dac_channel));
            } 
            // end special treatment
                 
            history_sample = quantized + ((OC::DAC::kOctaveZero + octave + continuous_offset_) * 12 << 7);
          }
        }
    } // end switch  
    
    bool changed = continuous ? (last_sample_ != temp_sample) : (last_sample_ != sample);
      
    if (changed) {
      MENU_REDRAW = 1;
      last_sample_ = continuous ? temp_sample : sample;
    }
    
    OC::DAC::set(dac_channel, sample + get_fine());

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
    return get_mask();
  }

  void update_scale_mask(uint16_t mask, uint16_t dummy) {
    apply_value(CHANNEL_SETTING_MASK, mask); // Should automatically be updated
    last_mask_ = mask;
    force_update_ = true;
  }
  //

  uint8_t getTriggerState() const {
    return trigger_display_.getState();
  }

  uint32_t get_shift_register() const {
    return turing_machine_.get_shift_register();
  }

  uint32_t get_logistic_map_register() const {
    return logistic_map_.get_register();
  }

  uint32_t get_bytebeat_register() const {
    return bytebeat_.get_last_sample();
  }

  uint32_t get_int_seq_register() const {
    return int_seq_.get_register();
  }

  int16_t get_int_seq_k() const {
    return int_seq_.get_k();
  }

  int16_t get_int_seq_l() const {
    return int_seq_.get_l();
  }

  int16_t get_int_seq_i() const {
    return int_seq_.get_i();
  }

  int16_t get_int_seq_j() const {
    return int_seq_.get_j();
  }

  int16_t get_int_seq_n() const {
    return int_seq_.get_n();
  }

  int16_t get_int_seq_x() const {
    return int_seq_.get_x();
  }

  bool get_int_seq_pass_go() const {
   return int_seq_.get_pass_go();
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
    if (OC::Scales::SCALE_NONE != get_scale(DUMMY)) {
      *settings++ = CHANNEL_SETTING_ROOT;
      *settings++ = CHANNEL_SETTING_MASK;
    }
    *settings++ = CHANNEL_SETTING_SOURCE;
    switch (get_source()) {
      case CHANNEL_SOURCE_CV1:
      case CHANNEL_SOURCE_CV2:
      case CHANNEL_SOURCE_CV3:
      case CHANNEL_SOURCE_CV4:
        if (get_source() != get_channel_index())
         *settings++ = CHANNEL_SETTING_AUX_SOURCE_DEST;
      break;
      case CHANNEL_SOURCE_TURING:
        *settings++ = CHANNEL_SETTING_TURING_LENGTH;
        if (OC::Scales::SCALE_NONE != get_scale(DUMMY))
            *settings++ = CHANNEL_SETTING_TURING_MODULUS;
        *settings++ = CHANNEL_SETTING_TURING_RANGE;
        *settings++ = CHANNEL_SETTING_TURING_PROB;
        if (OC::Scales::SCALE_NONE != get_scale(DUMMY))
            *settings++ = CHANNEL_SETTING_TURING_RANGE_CV_SOURCE;
        *settings++ = CHANNEL_SETTING_TURING_PROB_CV_SOURCE;
      break;
      case CHANNEL_SOURCE_LOGISTIC_MAP:
        *settings++ = CHANNEL_SETTING_LOGISTIC_MAP_R;
        *settings++ = CHANNEL_SETTING_LOGISTIC_MAP_RANGE;
        *settings++ = CHANNEL_SETTING_LOGISTIC_MAP_R_CV_SOURCE;
        *settings++ = CHANNEL_SETTING_LOGISTIC_MAP_RANGE_CV_SOURCE;
      break;
      case CHANNEL_SOURCE_BYTEBEAT:
        *settings++ = CHANNEL_SETTING_BYTEBEAT_EQUATION;
        *settings++ = CHANNEL_SETTING_BYTEBEAT_RANGE;
        *settings++ = CHANNEL_SETTING_BYTEBEAT_P0;
        *settings++ = CHANNEL_SETTING_BYTEBEAT_P1;
        *settings++ = CHANNEL_SETTING_BYTEBEAT_P2;
        *settings++ = CHANNEL_SETTING_BYTEBEAT_EQUATION_CV_SOURCE;
        *settings++ = CHANNEL_SETTING_BYTEBEAT_RANGE_CV_SOURCE;
        *settings++ = CHANNEL_SETTING_BYTEBEAT_P0_CV_SOURCE;
        *settings++ = CHANNEL_SETTING_BYTEBEAT_P1_CV_SOURCE;
        *settings++ = CHANNEL_SETTING_BYTEBEAT_P2_CV_SOURCE;
      break;
      case CHANNEL_SOURCE_INT_SEQ:
        *settings++ = CHANNEL_SETTING_INT_SEQ_INDEX;
        *settings++ = CHANNEL_SETTING_INT_SEQ_MODULUS;
        *settings++ = CHANNEL_SETTING_INT_SEQ_RANGE;
        *settings++ = CHANNEL_SETTING_INT_SEQ_DIRECTION;
        *settings++ = CHANNEL_SETTING_INT_SEQ_BROWNIAN_PROB;
        *settings++ = CHANNEL_SETTING_INT_SEQ_LOOP_START;
        *settings++ = CHANNEL_SETTING_INT_SEQ_LOOP_LENGTH;
        *settings++ = CHANNEL_SETTING_INT_SEQ_STRIDE;
        *settings++ = CHANNEL_SETTING_INT_SEQ_STRIDE_CV_SOURCE;
        *settings++ = CHANNEL_SETTING_INT_SEQ_FRAME_SHIFT_PROB;
        *settings++ = CHANNEL_SETTING_INT_SEQ_FRAME_SHIFT_RANGE;
        *settings++ = CHANNEL_SETTING_INT_SEQ_INDEX_CV_SOURCE;
        *settings++ = CHANNEL_SETTING_INT_SEQ_MODULUS_CV_SOURCE;
        *settings++ = CHANNEL_SETTING_INT_SEQ_RANGE_CV_SOURCE;
        *settings++ = CHANNEL_SETTING_INT_SEQ_RESET_TRIGGER;
      break;
      default:
      break;
    }
    *settings++ = CHANNEL_SETTING_TRIGGER;
    if (get_trigger_source() < CHANNEL_TRIGGER_CONTINUOUS_UP) {
      *settings++ = CHANNEL_SETTING_CLKDIV;
      *settings++ = CHANNEL_SETTING_DELAY;
    }
    *settings++ = CHANNEL_SETTING_OCTAVE;
    *settings++ = CHANNEL_SETTING_TRANSPOSE;
    *settings++ = CHANNEL_SETTING_FINE;
  
    num_enabled_settings_ = settings - enabled_settings_;
  }

  static bool indentSetting(ChannelSetting s) {
    switch (s) {
      case CHANNEL_SETTING_TURING_LENGTH:
      case CHANNEL_SETTING_TURING_MODULUS:
      case CHANNEL_SETTING_TURING_RANGE:
      case CHANNEL_SETTING_TURING_PROB:
      case CHANNEL_SETTING_TURING_MODULUS_CV_SOURCE:
      case CHANNEL_SETTING_TURING_RANGE_CV_SOURCE:
      case CHANNEL_SETTING_TURING_PROB_CV_SOURCE:
      case CHANNEL_SETTING_LOGISTIC_MAP_R:
      case CHANNEL_SETTING_LOGISTIC_MAP_RANGE:
      case CHANNEL_SETTING_LOGISTIC_MAP_R_CV_SOURCE:
      case CHANNEL_SETTING_LOGISTIC_MAP_RANGE_CV_SOURCE:
      case CHANNEL_SETTING_BYTEBEAT_EQUATION:
      case CHANNEL_SETTING_BYTEBEAT_RANGE:
      case CHANNEL_SETTING_BYTEBEAT_P0:
      case CHANNEL_SETTING_BYTEBEAT_P1:
      case CHANNEL_SETTING_BYTEBEAT_P2:
      case CHANNEL_SETTING_BYTEBEAT_EQUATION_CV_SOURCE:
      case CHANNEL_SETTING_BYTEBEAT_RANGE_CV_SOURCE:
      case CHANNEL_SETTING_BYTEBEAT_P0_CV_SOURCE:
      case CHANNEL_SETTING_BYTEBEAT_P1_CV_SOURCE:
      case CHANNEL_SETTING_BYTEBEAT_P2_CV_SOURCE:      
      case CHANNEL_SETTING_INT_SEQ_INDEX:
      case CHANNEL_SETTING_INT_SEQ_MODULUS:
      case CHANNEL_SETTING_INT_SEQ_RANGE:
      case CHANNEL_SETTING_INT_SEQ_DIRECTION:
      case CHANNEL_SETTING_INT_SEQ_BROWNIAN_PROB:
      case CHANNEL_SETTING_INT_SEQ_LOOP_START:
      case CHANNEL_SETTING_INT_SEQ_LOOP_LENGTH:
      case CHANNEL_SETTING_INT_SEQ_FRAME_SHIFT_PROB:
      case CHANNEL_SETTING_INT_SEQ_FRAME_SHIFT_RANGE:
      case CHANNEL_SETTING_INT_SEQ_STRIDE:
      case CHANNEL_SETTING_INT_SEQ_INDEX_CV_SOURCE:
      case CHANNEL_SETTING_INT_SEQ_MODULUS_CV_SOURCE:
      case CHANNEL_SETTING_INT_SEQ_RANGE_CV_SOURCE:
      case CHANNEL_SETTING_INT_SEQ_STRIDE_CV_SOURCE:
      case CHANNEL_SETTING_INT_SEQ_RESET_TRIGGER:
      case CHANNEL_SETTING_CLKDIV:
      case CHANNEL_SETTING_DELAY:
        return true;
      default: break;
    }
    return false;
  }

  void RenderScreensaver(weegfx::coord_t x) const;

private:
  bool force_update_;
  bool instant_update_;
  int last_scale_;
  uint16_t last_mask_;
  int32_t last_sample_;
  uint8_t clock_;
  bool int_seq_reset_;
  int8_t continuous_offset_;
  int8_t channel_index_;
  int32_t schedule_mask_rotate_;
  int8_t prev_destination_;
  int8_t prev_octave_cv_;
  int8_t prev_transpose_cv_;
  int8_t prev_root_cv_;
  
  util::TriggerDelay<OC::kMaxTriggerDelayTicks> trigger_delay_;
  util::TuringShiftRegister turing_machine_;
  util::LogisticMap logistic_map_;
  peaks::ByteBeat bytebeat_ ;
  util::IntegerSequence int_seq_ ;
  braids::Quantizer quantizer_;
  OC::DigitalInputDisplay trigger_display_;

  int num_enabled_settings_;
  ChannelSetting enabled_settings_[CHANNEL_SETTING_LAST];

  OC::vfx::ScrollingHistory<int32_t, 5> scrolling_history_;

  bool update_scale(bool force, int32_t mask_rotate) {

    force_update_ = false;
    const int scale = get_scale(DUMMY);
    uint16_t mask = get_mask();

    if (mask_rotate)
      mask = OC::ScaleEditor<QuantizerChannel>::RotateMask(mask, OC::Scales::GetScale(scale).num_notes, mask_rotate);

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

const char* const channel_input_sources[CHANNEL_SOURCE_LAST] = {
  "CV1", "CV2", "CV3", "CV4", "Turing", "Lgstc", "ByteB", "IntSq"
};

const char* const aux_cv_dest[5] = {
  "-", "root", "oct", "trns", "mask"
};

SETTINGS_DECLARE(QuantizerChannel, CHANNEL_SETTING_LAST) {
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "Scale", OC::scale_names, settings::STORAGE_TYPE_U8 },
  { 0, 0, 11, "Root", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 },
  { 65535, 1, 65535, "Active notes", NULL, settings::STORAGE_TYPE_U16 },
  { CHANNEL_SOURCE_CV1, CHANNEL_SOURCE_CV1, CHANNEL_SOURCE_LAST - 1, "CV Source", channel_input_sources, settings::STORAGE_TYPE_U8 },
  { QQ_DEST_NONE, QQ_DEST_NONE, QQ_DEST_LAST - 1, "CV aux >", aux_cv_dest, settings::STORAGE_TYPE_U8 },
  { CHANNEL_TRIGGER_CONTINUOUS_DOWN, 0, CHANNEL_TRIGGER_LAST - 1, "Trigger source", OC::Strings::channel_trigger_sources, settings::STORAGE_TYPE_U8 },
  { 1, 1, 16, "Clock div", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, OC::kNumDelayTimes - 1, "Trigger delay", OC::Strings::trigger_delay_times, settings::STORAGE_TYPE_U8 },
  { 0, -5, 7, "Transpose", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -4, 4, "Octave", NULL, settings::STORAGE_TYPE_I8 },
  { 0, -999, 999, "Fine", NULL, settings::STORAGE_TYPE_I16 },
  { 16, 1, 32, "LFSR length", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "LFSR prb", NULL, settings::STORAGE_TYPE_U8 },
  { 24, 2, 121, "LFSR modulus", NULL, settings::STORAGE_TYPE_U8 },
  { 12, 1, 120, "LFSR range", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 4, "LFSR prb CV >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "LFSR mod CV >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "LFSR rng CV >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 128, 1, 255, "Logistic r", NULL, settings::STORAGE_TYPE_U8 },
  { 12, 1, 120, "Logistic range", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 4, "Log r   CV >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "Log rng CV >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 15, "Bytebeat eqn", OC::Strings::bytebeat_equation_names, settings::STORAGE_TYPE_U8 },
  { 12, 1, 120, "Bytebeat rng", NULL, settings::STORAGE_TYPE_U8 },
  { 8, 1, 255, "Bytebeat P0", NULL, settings::STORAGE_TYPE_U8 },
  { 12, 1, 255, "Bytebeat P1", NULL, settings::STORAGE_TYPE_U8 },
  { 14, 1, 255, "Bytebeat P2", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 4, "Bb eqn CV src", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "Bb rng CV src", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "Bb P0  CV src", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "Bb P1  CV src", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "Bb P2  CV src", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 9, "IntSeq", OC::Strings::integer_sequence_names, settings::STORAGE_TYPE_U4 },
  { 24, 2, 121, "IntSeq modul.", NULL, settings::STORAGE_TYPE_U8 },
  { 12, 1, 120, "IntSeq range", NULL, settings::STORAGE_TYPE_U8 },
  { 1, 0, 1, "IntSeq dir", OC::Strings::integer_sequence_dirs, settings::STORAGE_TYPE_U4 },
  { 0, 0, 255, "> Brownian prob", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, kIntSeqLen - 2, "IntSeq start", NULL, settings::STORAGE_TYPE_U8 },
  { 8, 2, kIntSeqLen, "IntSeq len", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 255, "IntSeq FS prob", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 5, "IntSeq FS rng", NULL, settings::STORAGE_TYPE_U4 },
  { 1, 1, kIntSeqLen - 1, "Fractal stride", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 4, "IntSeq CV   >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "IntSeq mod CV", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "IntSeq rng CV", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "F. stride CV >", OC::Strings::cv_input_names_none, settings::STORAGE_TYPE_U4 },
  { 0, 0, 4, "IntSeq reset", OC::Strings::trigger_input_names_none, settings::STORAGE_TYPE_U4 }
};
 
// WIP refactoring to better encapsulate and for possible app interface change
class QuadQuantizer {
public:
  void Init() {
    selected_channel = 0;
    cursor.Init(CHANNEL_SETTING_SCALE, CHANNEL_SETTING_LAST - 1);
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
    quantizer_channels[i].update_scale_mask(quantizer_channels[i].get_mask(), 0x0);
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
  quantizer_channels[0].Update(triggers, DAC_CHANNEL_A);
  quantizer_channels[1].Update(triggers, DAC_CHANNEL_B);
  quantizer_channels[2].Update(triggers, DAC_CHANNEL_C);
  quantizer_channels[3].Update(triggers, DAC_CHANNEL_D);
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

    menu::QuadTitleBar::DrawGateIndicator(i, channel.getTriggerState());
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
        menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, channel.get_rotated_scale_mask(), OC::Scales::GetScale(channel.get_scale(DUMMY)).num_notes);
        list_item.DrawNoValue<false>(value, attr);
        break;
      case CHANNEL_SETTING_TRIGGER:
      {
        if (channel.get_source() > CHANNEL_SOURCE_CV4)
           list_item.DrawValueMax(value, attr, CHANNEL_TRIGGER_TR4);
        else 
          list_item.DrawDefault(value, attr);
      }
        break;
      case CHANNEL_SETTING_SOURCE:
        if (CHANNEL_SOURCE_TURING == channel.get_source()) {
          int turing_length = channel.get_turing_length();
          int w = turing_length >= 16 ? 16 * 3 : turing_length * 3;

          menu::DrawMask<true, 16, 8, 1>(menu::kDisplayWidth, list_item.y, channel.get_shift_register(), turing_length);
          list_item.valuex = menu::kDisplayWidth - w - 1;
          list_item.DrawNoValue<true>(value, attr);
          break;
          // Fall through if not Turing
        }
      default:
        if (QuantizerChannel::indentSetting(static_cast<ChannelSetting>(setting)))
          list_item.x += menu::kIndentDx;
        if (setting == CHANNEL_SETTING_SOURCE && channel.get_trigger_source() > CHANNEL_TRIGGER_TR4)
          list_item.DrawValueMax(value, attr, CHANNEL_TRIGGER_TR4);
        else list_item.DrawDefault(value, attr);
      break;
    }
  }

  if (qq_state.scale_editor.active())
    qq_state.scale_editor.Draw();
}

void QQ_handleButtonEvent(const UI::Event &event) {

  if (UI::EVENT_BUTTON_LONG_PRESS == event.type && OC::CONTROL_BUTTON_DOWN == event.control)   
    QQ_downButtonLong(); 
      
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

        int event_value = event.value;

        switch (setting) {
          case CHANNEL_SETTING_TRIGGER:
          {
            if (selected.get_trigger_source() == CHANNEL_TRIGGER_TR4 && selected.get_source() > CHANNEL_SOURCE_CV4 && event.value > 0)
              event_value = 0x0;
          }
          break;
          case CHANNEL_SETTING_SOURCE: {
             if (selected.get_source() == CHANNEL_SOURCE_CV4 && selected.get_trigger_source() > CHANNEL_TRIGGER_TR4 && event.value > 0)
              event_value = 0x0;
          }
          break;
          default:
          break;
        }
        
        if (selected.change_value(setting, event_value))
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
      int scale = selected.get_scale(DUMMY);
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
  int scale = selected_channel.get_scale(DUMMY);
  int root = selected_channel.get_root();
  for (int i = 0; i < 4; ++i) {
    if (i != qq_state.selected_channel) {
      quantizer_channels[i].apply_value(CHANNEL_SETTING_ROOT, root);
      quantizer_channels[i].set_scale(scale);
    }
  }
}

void QQ_downButtonLong() {
  
  QuantizerChannel &selected_channel = quantizer_channels[qq_state.selected_channel];
  selected_channel.update_scale_mask(0xFFFF, 0x0);
}

int32_t history[5];
static const weegfx::coord_t kBottom = 60;

inline int32_t render_pitch(int32_t pitch, weegfx::coord_t x, weegfx::coord_t width) {
  CONSTRAIN(pitch, 0, 120 << 7);
  int32_t octave = pitch / (12 << 7);
  pitch -= (octave * 12 << 7);
  graphics.drawHLine(x, kBottom - ((pitch * 4) >> 7), width);
  return octave;
}

void QuantizerChannel::RenderScreensaver(weegfx::coord_t start_x) const {

  // History
  scrolling_history_.Read(history);
  weegfx::coord_t scroll_pos = (scrolling_history_.get_scroll_pos() * 6) >> 8;

  // Top: Show gate & CV (or register bits)
  menu::DrawGateIndicator(start_x + 1, 2, getTriggerState());
  const ChannelSource source = get_source();
  switch (source) {
    case CHANNEL_SOURCE_TURING:
      menu::DrawMask<true, 8, 8, 1>(start_x + 31, 1, get_shift_register(), get_turing_length());
      break;
    case CHANNEL_SOURCE_LOGISTIC_MAP:
      menu::DrawMask<true, 8, 8, 1>(start_x + 31, 1, get_logistic_map_register(), 32);
      break;
    case CHANNEL_SOURCE_BYTEBEAT:
      menu::DrawMask<true, 8, 8, 1>(start_x + 31, 1, get_bytebeat_register(), 8);
      break;
    case CHANNEL_SOURCE_INT_SEQ:
      // graphics.setPrintPos(start_x + 31 - 16, 4);
      graphics.setPrintPos(start_x + 8, 4);
      graphics.print(get_int_seq_k());
      // menu::DrawMask<true, 8, 8, 1>(start_x + 31, 1, get_int_seq_register(), 8);
      break;
    default: {
      graphics.setPixel(start_x + QQ_OFFSET_X - 16, 4);
      int32_t cv = OC::ADC::value(static_cast<ADC_CHANNEL>(source));
      cv = (cv * 24 + 2047) >> 12;
      if (cv < 0)
        graphics.drawRect(start_x + QQ_OFFSET_X - 16 + cv, 6, -cv, 2);
      else if (cv > 0)
        graphics.drawRect(start_x + QQ_OFFSET_X - 16, 6, cv, 2);
      else
        graphics.drawRect(start_x + QQ_OFFSET_X - 16, 6, 1, 2);
    }
    break;
  }

#ifdef QQ_DEBUG_SCREENSAVER
  graphics.drawVLinePattern(start_x + 31, 0, 64, 0x55);
#endif

  // Draw semitone intervals, 4px apart
  weegfx::coord_t x = start_x + 26;
  weegfx::coord_t y = kBottom;
  for (int i = 0; i < 12; ++i, y -= 4)
    graphics.setPixel(x, y);

  x = start_x + 1;
  render_pitch(history[0], x, scroll_pos); x += scroll_pos;
  render_pitch(history[1], x, 6); x += 6;
  render_pitch(history[2], x, 6); x += 6;
  render_pitch(history[3], x, 6); x += 6;

  int32_t octave = render_pitch(history[4], x, 6 - scroll_pos);
  graphics.drawBitmap8(start_x + 28, kBottom - octave * 4 - 1, OC::kBitmapLoopMarkerW, OC::bitmap_loop_markers_8 + OC::kBitmapLoopMarkerW);
}

void QQ_screensaver() {
#ifdef QQ_DEBUG_SCREENSAVER
  debug::CycleMeasurement render_cycles;
#endif

  quantizer_channels[0].RenderScreensaver(0);
  quantizer_channels[1].RenderScreensaver(32);
  quantizer_channels[2].RenderScreensaver(64);
  quantizer_channels[3].RenderScreensaver(96);

#ifdef QQ_DEBUG_SCREENSAVER
  graphics.drawHLine(0, menu::kMenuLineH, menu::kDisplayWidth);
  uint32_t us = debug::cycles_to_us(render_cycles.read());
  graphics.setPrintPos(0, 32);
  graphics.printf("%u",  us);
#endif
}

#ifdef QQ_DEBUG
void QQ_debug() {
  for (int i = 0; i < 4; ++i) { 
    uint8_t ypos = 10*(i + 1) + 2 ; 
    graphics.setPrintPos(2, ypos);
    graphics.print(quantizer_channels[i].get_int_seq_i());
    graphics.setPrintPos(30, ypos);
    graphics.print(quantizer_channels[i].get_int_seq_l());
    graphics.setPrintPos(58, ypos);
    graphics.print(quantizer_channels[i].get_int_seq_j());
    graphics.setPrintPos(80, ypos);
    graphics.print(quantizer_channels[i].get_int_seq_k());
    graphics.setPrintPos(104, ypos);
    graphics.print(quantizer_channels[i].get_int_seq_x());
 }
}
#endif // QQ_DEBUG

