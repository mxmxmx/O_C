// Copyright (c) 2014-2017 Max Stadler, Patrick Dowling
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
#include "util/util_turing.h"
#include "util/util_ringbuffer.h"
#include "util/util_integer_sequences.h"
#include "OC_DAC.h"
#include "OC_menus.h"
#include "OC_scales.h"
#include "OC_scale_edit.h"
#include "OC_strings.h"
#include "OC_visualfx.h"
#include "peaks_bytebeat.h"
#include "extern/dspinst.h"

namespace menu = OC::menu; // Ugh. This works for all .ino files

#define NUM_ASR_CHANNELS 0x4
#define ASR_MAX_ITEMS 256 // = ASR ring buffer size. 
#define ASR_HOLD_BUF_SIZE ASR_MAX_ITEMS / NUM_ASR_CHANNELS // max. delay size 
#define NUM_INPUT_SCALING 40 // # steps for input sample scaling (sb)

// CV input gain multipliers 
const int32_t multipliers[NUM_INPUT_SCALING] = {
  // 0.0 / 2.0 in 0.05 steps
  3277, 6554, 9830, 13107, 16384, 19661, 22938, 26214, 29491, 32768, 
  36045, 39322, 42598, 45875, 49152, 52429, 55706, 58982, 62259, 65536, 
  68813, 72090, 75366, 78643, 81920, 85197, 88474, 91750, 95027, 98304, 
  101581, 104858, 108134, 111411, 114688, 117964, 121242, 124518, 127795, 131072
};

const uint8_t MULT_ONE = 19; // == 65536, see above

enum ASRSettings {
  ASR_SETTING_SCALE,
  ASR_SETTING_OCTAVE, 
  ASR_SETTING_ROOT,
  ASR_SETTING_MASK,
  ASR_SETTING_INDEX,
  ASR_SETTING_MULT,
  ASR_SETTING_DELAY,
  ASR_SETTING_BUFFER_LENGTH,
  ASR_SETTING_CV_SOURCE,
  ASR_SETTING_CV4_DESTINATION,
  ASR_SETTING_TURING_LENGTH,
  ASR_SETTING_TURING_PROB,
  ASR_SETTING_TURING_CV_SOURCE,
  ASR_SETTING_BYTEBEAT_EQUATION,
  ASR_SETTING_BYTEBEAT_P0,
  ASR_SETTING_BYTEBEAT_P1,
  ASR_SETTING_BYTEBEAT_P2,
  ASR_SETTING_BYTEBEAT_CV_SOURCE,
  ASR_SETTING_INT_SEQ_INDEX,
  ASR_SETTING_INT_SEQ_MODULUS,
  ASR_SETTING_INT_SEQ_START,
  ASR_SETTING_INT_SEQ_LENGTH,
  ASR_SETTING_INT_SEQ_DIR,
  ASR_SETTING_FRACTAL_SEQ_STRIDE,
  ASR_SETTING_INT_SEQ_CV_SOURCE,
  ASR_SETTING_LAST
};

enum ASRChannelSource {
  ASR_CHANNEL_SOURCE_CV1,
  ASR_CHANNEL_SOURCE_TURING,
  ASR_CHANNEL_SOURCE_BYTEBEAT,
  ASR_CHANNEL_SOURCE_INTEGER_SEQUENCES,
  ASR_CHANNEL_SOURCE_LAST
};

enum ASR_CV4_DEST {
  ASR_DEST_OCTAVE,
  ASR_DEST_ROOT,
  ASR_DEST_TRANSPOSE,
  ASR_DEST_BUFLEN,
  ASR_DEST_INPUT_SCALING,
  ASR_DEST_LAST
};

typedef int16_t ASR_pitch;

class ASR : public settings::SettingsBase<ASR, ASR_SETTING_LAST> {
public:
  static constexpr size_t kHistoryDepth = 5;

  int get_scale(uint8_t dummy) const {
    return values_[ASR_SETTING_SCALE];
  }

  uint8_t get_buffer_length() const {
    return values_[ASR_SETTING_BUFFER_LENGTH];
  }
  
  void set_scale(int scale) {
    if (scale != get_scale(DUMMY)) {
      const OC::Scale &scale_def = OC::Scales::GetScale(scale);
      uint16_t mask = get_mask();
      if (0 == (mask & ~(0xffff << scale_def.num_notes)))
        mask |= 0x1;
      apply_value(ASR_SETTING_MASK, mask);
      apply_value(ASR_SETTING_SCALE, scale);
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
   
  uint16_t get_mask() const {
    return values_[ASR_SETTING_MASK];
  }

  int get_root() const {
    return values_[ASR_SETTING_ROOT];
  }

  int get_root(uint8_t DUMMY) const {
    return 0x0; // dummy
  }

  int get_index() const {
    return values_[ASR_SETTING_INDEX];
  }

  int get_octave() const {
    return values_[ASR_SETTING_OCTAVE];
  }

  bool octave_toggle() {
    octave_toggle_ = (~octave_toggle_) & 1u;
    return octave_toggle_;
  }

  bool poke_octave_toggle() const {
    return octave_toggle_;
  }

  int get_mult() const {
    return values_[ASR_SETTING_MULT];
  }

  int get_cv_source() const {
    return values_[ASR_SETTING_CV_SOURCE];
  }

  uint8_t get_cv4_destination() const {
    return values_[ASR_SETTING_CV4_DESTINATION];
  }

  uint8_t get_turing_length() const {
    return values_[ASR_SETTING_TURING_LENGTH];
  }

  uint8_t get_turing_display_length() {
    return turing_display_length_;
  }

  uint8_t get_turing_probability() const {
    return values_[ASR_SETTING_TURING_PROB];
  }
  
  uint8_t get_turing_CV() const {
    return values_[ASR_SETTING_TURING_CV_SOURCE];
  }

  uint32_t get_shift_register() const {
    return turing_machine_.get_shift_register();
  }

  uint16_t get_trigger_delay() const {
    return values_[ASR_SETTING_DELAY];
  }

  uint8_t get_bytebeat_equation() const {
    return values_[ASR_SETTING_BYTEBEAT_EQUATION];
  }

  uint8_t get_bytebeat_p0() const {
    return values_[ASR_SETTING_BYTEBEAT_P0];
  }

  uint8_t get_bytebeat_p1() const {
    return values_[ASR_SETTING_BYTEBEAT_P1];
  }

  uint8_t get_bytebeat_p2() const {
    return values_[ASR_SETTING_BYTEBEAT_P2];
  }

  uint8_t get_bytebeat_CV() const {
    return values_[ASR_SETTING_BYTEBEAT_CV_SOURCE];
  }

  uint8_t get_int_seq_index() const {
    return values_[ ASR_SETTING_INT_SEQ_INDEX];
  }

  uint8_t get_int_seq_modulus() const {
    return values_[ ASR_SETTING_INT_SEQ_MODULUS];
  }

  int16_t get_int_seq_start() const {
    return static_cast<int16_t>(values_[ASR_SETTING_INT_SEQ_START]);
  }

  int16_t get_int_seq_length() const {
    return static_cast<int16_t>(values_[ASR_SETTING_INT_SEQ_LENGTH] - 1);
  }

  bool get_int_seq_dir() const {
    return static_cast<bool>(values_[ASR_SETTING_INT_SEQ_DIR]);
  }

  int16_t get_fractal_seq_stride() const {
    return static_cast<int16_t>(values_[ASR_SETTING_FRACTAL_SEQ_STRIDE]);
  }

  uint8_t get_int_seq_CV() const {
    return values_[ASR_SETTING_INT_SEQ_CV_SOURCE];
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

  void toggle_delay_mechanics() {
    delay_type_ = (~delay_type_) & 1u;
  }

  bool get_delay_type() const {
    return delay_type_;
  }
  
  void manual_freeze() {
    freeze_switch_ = (~freeze_switch_) & 1u;
  }

  void clear_freeze() {
    freeze_switch_ = false;
  }

  bool freeze_state() {
    return freeze_switch_;
  }

  ASRSettings enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  void init() {
    
    force_update_ = false;
    last_scale_ = -0x1;
    delay_type_ = false;
    octave_toggle_ = false;
    freeze_switch_ = false;
    TR2_state_ = 0x1;
    set_scale(OC::Scales::SCALE_SEMI);
    last_mask_ = 0x0;
    quantizer_.Init();
    update_scale(true, 0x0);

    _ASR.Init();
    clock_display_.Init();
    for (auto &sh : scrolling_history_)
      sh.Init();
    update_enabled_settings();
    
    trigger_delay_.Init();
    turing_machine_.Init();
    turing_display_length_ = get_turing_length();
    bytebeat_.Init();
    int_seq_.Init(get_int_seq_start(), get_int_seq_length());
 }

  bool update_scale(bool force, int32_t mask_rotate) {
    
    const int scale = get_scale(DUMMY);
    uint16_t mask = get_mask();
    if (mask_rotate)
      mask = OC::ScaleEditor<ASR>::RotateMask(mask, OC::Scales::GetScale(scale).num_notes, mask_rotate);

    if (force || (last_scale_ != scale || last_mask_ != mask)) {

      last_scale_ = scale;
      last_mask_ = mask;
      quantizer_.Configure(OC::Scales::GetScale(scale), mask);
      return true;
    } else {
      return false;
    }
  }

  void force_update() {
    force_update_ = true;
  }

  // Wrappers for ScaleEdit
  void scale_changed() {
    force_update_ = true;
  }

  uint16_t get_scale_mask(uint8_t scale_select) const {
    return get_mask();
  }

  void update_scale_mask(uint16_t mask, uint16_t dummy) {
    apply_value(ASR_SETTING_MASK, mask); // Should automatically be updated
  }
  //

  uint16_t get_rotated_mask() const {
    return last_mask_;
  }

  void set_display_mask(uint16_t mask) {
    last_mask_ = mask;
  }

  void update_enabled_settings() {
    
    ASRSettings *settings = enabled_settings_;

    *settings++ = ASR_SETTING_ROOT;
    *settings++ = ASR_SETTING_MASK;
    *settings++ = ASR_SETTING_OCTAVE;
    *settings++ = ASR_SETTING_INDEX;
    *settings++ = ASR_SETTING_BUFFER_LENGTH;
    *settings++ = ASR_SETTING_DELAY;
    *settings++ = ASR_SETTING_MULT;
 
    *settings++ = ASR_SETTING_CV4_DESTINATION;
    *settings++ = ASR_SETTING_CV_SOURCE;
   
    switch (get_cv_source()) {
      case ASR_CHANNEL_SOURCE_TURING:
        *settings++ = ASR_SETTING_TURING_LENGTH;
        *settings++ = ASR_SETTING_TURING_PROB;
        *settings++ = ASR_SETTING_TURING_CV_SOURCE;
       break;
      case ASR_CHANNEL_SOURCE_BYTEBEAT:
        *settings++ = ASR_SETTING_BYTEBEAT_EQUATION;
        *settings++ = ASR_SETTING_BYTEBEAT_P0;
        *settings++ = ASR_SETTING_BYTEBEAT_P1;
        *settings++ = ASR_SETTING_BYTEBEAT_P2;
        *settings++ = ASR_SETTING_BYTEBEAT_CV_SOURCE;
      break;
       case ASR_CHANNEL_SOURCE_INTEGER_SEQUENCES:
        *settings++ = ASR_SETTING_INT_SEQ_INDEX;
        *settings++ = ASR_SETTING_INT_SEQ_MODULUS;
        *settings++ = ASR_SETTING_INT_SEQ_START;
        *settings++ = ASR_SETTING_INT_SEQ_LENGTH;
        *settings++ = ASR_SETTING_INT_SEQ_DIR;
        *settings++ = ASR_SETTING_FRACTAL_SEQ_STRIDE;
        *settings++ = ASR_SETTING_INT_SEQ_CV_SOURCE;
       break;
     default:
      break;
    } 

    num_enabled_settings_ = settings - enabled_settings_;
  }

  void updateASR_indexed(int32_t *_asr_buf, int32_t _sample, int16_t _index, bool _freeze) {
    
      int16_t _delay = _index, _offset;

      if (_freeze) {

        int8_t _buflen = get_buffer_length();
        if (get_cv4_destination() == ASR_DEST_BUFLEN) {
          _buflen += ((OC::ADC::value<ADC_CHANNEL_4>() + 31) >> 6);
          CONSTRAIN(_buflen, NUM_ASR_CHANNELS, ASR_HOLD_BUF_SIZE - 0x1);
        }
        _ASR.Freeze(_buflen);
      }
      else
        _ASR.Write(_sample);
      
      // update outputs:
      _offset = _delay;
      *(_asr_buf + DAC_CHANNEL_A) = _ASR.Poke(_offset++);
      // delay mechanics ... 
      _delay = delay_type_ ? 0x0 : _delay;
      // continue updating
      _offset +=_delay;
      *(_asr_buf + DAC_CHANNEL_B) = _ASR.Poke(_offset++);
      _offset +=_delay;
      *(_asr_buf + DAC_CHANNEL_C) = _ASR.Poke(_offset++);
      _offset +=_delay;
      *(_asr_buf + DAC_CHANNEL_D) = _ASR.Poke(_offset++);
  }

  inline void update() {

     bool update = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>();
     clock_display_.Update(1, update);

     trigger_delay_.Update();
    
     if (update)
      trigger_delay_.Push(OC::trigger_delay_ticks[get_trigger_delay()]);
      
     update = trigger_delay_.triggered();

     if (update) {        

         bool _freeze_switch, _freeze = digitalReadFast(TR2);
         int8_t _root  = get_root();
         int8_t _index = get_index() + ((OC::ADC::value<ADC_CHANNEL_2>() + 31) >> 6);
         int8_t _octave = get_octave();
         int8_t _transpose = 0;
         int8_t _mult = get_mult();
         int32_t _pitch = OC::ADC::raw_pitch_value(ADC_CHANNEL_1);
         int32_t _asr_buffer[NUM_ASR_CHANNELS];  

         bool forced_update = force_update_;
         force_update_ = false;
         update_scale(forced_update, (OC::ADC::value<ADC_CHANNEL_3>() + 127) >> 8);
         
         // cv4 destination, defaults to octave:
         switch(get_cv4_destination()) {

            case ASR_DEST_OCTAVE:
              _octave += (OC::ADC::value<ADC_CHANNEL_4>() + 255) >> 9;
            break;
            case ASR_DEST_ROOT:
              _root += (OC::ADC::value<ADC_CHANNEL_4>() + 127) >> 8;
              CONSTRAIN(_root, 0, 11);
            break;
            case ASR_DEST_TRANSPOSE:
              _transpose += (OC::ADC::value<ADC_CHANNEL_4>() + 63) >> 7;
              CONSTRAIN(_transpose, -12, 12); 
            break;
            case ASR_DEST_INPUT_SCALING:
              _mult += (OC::ADC::value<ADC_CHANNEL_4>() + 63) >> 7;
              CONSTRAIN(_mult, 0, NUM_INPUT_SCALING - 1);
            break;
            // CV for buffer length happens in updateASR_indexed
            default:
            break;
         }
         
         // freeze ? 
         if (_freeze < TR2_state_)
            freeze_switch_ = true;
         else if (_freeze > TR2_state_) {
            freeze_switch_ = false;
         }
         TR2_state_ = _freeze;
         // 
         _freeze_switch = freeze_switch_;

         // use built in CV sources? 
         if (!_freeze_switch) {
          
           switch (get_cv_source()) {
  
            case ASR_CHANNEL_SOURCE_TURING:
              {
                int16_t _length = get_turing_length();
                int16_t _probability = get_turing_probability();
  
                // _pitch can do other things now -- 
                switch (get_turing_CV()) {

                    case 1: // mult
                     _mult += ((_pitch + 63) >> 7);
                    break;
                    case 2:  // LEN, 1-32
                     _length += ((_pitch + 255) >> 9);
                     CONSTRAIN(_length, 1, 32);
                    break;
                    case 3:  // P
                     _probability += ((_pitch + 7) >> 4);
                     CONSTRAIN(_probability, 0, 255);
                    break;
                    default:
                    break;
                }
                
                turing_machine_.set_length(_length);
                turing_machine_.set_probability(_probability); 
                turing_display_length_ = _length;
                
                _pitch = turing_machine_.Clock();
                
                // scale LFSR output (0 - 4095) / compensate for length
                if (_length < 12)
                  _pitch = _pitch << (12 -_length);
                else 
                  _pitch = _pitch >> (_length - 12);
                _pitch &= 0xFFF;
              }
              break; 
              case ASR_CHANNEL_SOURCE_BYTEBEAT:
             {
               int32_t _bytebeat_eqn = get_bytebeat_equation() << 12;
               int32_t _bytebeat_p0 = get_bytebeat_p0() << 8;
               int32_t _bytebeat_p1 = get_bytebeat_p1() << 8;
               int32_t _bytebeat_p2 = get_bytebeat_p2() << 8;
    
                 // _pitch can do other things now -- 
                switch (get_bytebeat_CV()) {
    
                    case 1:  //  0-15
                     _bytebeat_eqn += (_pitch << 4);
                     _bytebeat_eqn = USAT16(_bytebeat_eqn);
                    break;
                    case 2:  // P0
                     _bytebeat_p0 += (_pitch << 4);
                     _bytebeat_p0 = USAT16(_bytebeat_p0);
                    break;
                    case 3:  // P1
                     _bytebeat_p1 += (_pitch << 4);
                     _bytebeat_p1 = USAT16(_bytebeat_p1);
                    break;
                    case 5:  // P4
                     _bytebeat_p2 += (_pitch << 4);
                     _bytebeat_p2 = USAT16(_bytebeat_p2);
                    break;
                    default: // mult
                     _mult += ((_pitch + 63) >> 7);
                    break;
                }
    
                bytebeat_.set_equation(_bytebeat_eqn);
                bytebeat_.set_p0(_bytebeat_p0);
                bytebeat_.set_p0(_bytebeat_p1);
                bytebeat_.set_p0(_bytebeat_p2);
    
                int32_t _bb = (static_cast<int16_t>(bytebeat_.Clock()) & 0xFFF);
                _pitch = _bb;  
              }
              break;
              case ASR_CHANNEL_SOURCE_INTEGER_SEQUENCES:
             {
               int16_t _int_seq_index = get_int_seq_index() ;
               int16_t _int_seq_modulus = get_int_seq_modulus() ;
               int16_t _int_seq_start = get_int_seq_start() ;
               int16_t _int_seq_length = get_int_seq_length();
               int16_t _fractal_seq_stride = get_fractal_seq_stride();
               bool _int_seq_dir = get_int_seq_dir();
  
               // _pitch can do other things now -- 
               switch (get_int_seq_CV()) {
  
                  case 1:  // integer sequence, 0-8
                   _int_seq_index += ((_pitch + 255) >> 9);
                   CONSTRAIN(_int_seq_index, 0, 8);
                  break;
                   case 2:  // sequence start point, 0 to kIntSeqLen - 2
                   _int_seq_start += ((_pitch + 15) >> 5);
                   CONSTRAIN(_int_seq_start, 0, kIntSeqLen - 2);
                  break;
                   case 3:  // sequence loop length, 1 to kIntSeqLen - 1
                   _int_seq_length += ((_pitch + 15) >> 5);
                   CONSTRAIN(_int_seq_length, 1, kIntSeqLen - 1);
                  break;
                   case 4:  // fractal sequence stride length, 1 to kIntSeqLen - 1
                   _fractal_seq_stride += ((_pitch + 15) >> 5);
                   CONSTRAIN(_fractal_seq_stride, 1, kIntSeqLen - 1);
                  break;
                   case 5:  // fractal sequence modulus
                   _int_seq_modulus += ((_pitch + 15) >> 5);
                   CONSTRAIN(_int_seq_modulus, 2, 121);
                  break;
                  default: // mult
                   _mult += ((_pitch + 63) >> 7);
                  break;
                }
             
                int_seq_.set_loop_start(_int_seq_start);
                int_seq_.set_loop_length(_int_seq_length);
                int_seq_.set_int_seq(_int_seq_index);
                int_seq_.set_int_seq_modulus(_int_seq_modulus);
                int_seq_.set_loop_direction(_int_seq_dir);
                int_seq_.set_fractal_stride(_fractal_seq_stride);
    
                int32_t _is = (static_cast<int16_t>(int_seq_.Clock()) & 0xFFF);
                _pitch = _is;  
              }
              break;      
              default:
              break;     
           }
         }
         else {
          // we hold, so we only need the multiplication CV:
            switch (get_cv_source()) {
    
              case ASR_CHANNEL_SOURCE_TURING:
                if (get_turing_CV() == 0x0)
                  _mult += ((_pitch + 63) >> 7);
              break;
              case ASR_CHANNEL_SOURCE_INTEGER_SEQUENCES:
                if (get_int_seq_CV() == 0x0)
                  _mult += ((_pitch + 63) >> 7);
              break;
              case ASR_CHANNEL_SOURCE_BYTEBEAT:
                if (get_bytebeat_CV() == 0x0)
                  _mult += ((_pitch + 63) >> 7);
              break;
              default:
              break;
           }
         }
         // limit gain factor.
         CONSTRAIN(_mult, 0, NUM_INPUT_SCALING - 0x1);
         // .. and index
         CONSTRAIN(_index, 0, ASR_HOLD_BUF_SIZE - 0x1);
         // push sample into ring-buffer and/or freeze buffer: 
         updateASR_indexed(_asr_buffer, _pitch, _index, _freeze_switch); 

         // get octave offset :
         if (!digitalReadFast(TR3)) 
            _octave++;
         else if (!digitalReadFast(TR4)) 
            _octave--;
         
         // quantize buffer outputs:
         for (int i = 0; i < NUM_ASR_CHANNELS; ++i) {

             int32_t _sample = _asr_buffer[i];
          
            // scale sample
             if (_mult != MULT_ONE) {  
               _sample = signed_multiply_32x16b(multipliers[_mult], _sample);
               _sample = signed_saturate_rshift(_sample, 16, 0);
             }

             _sample = quantizer_.Process(_sample, _root << 7, _transpose);
             _sample = OC::DAC::pitch_to_scaled_voltage_dac(static_cast<DAC_CHANNEL>(i), _sample, _octave, OC::DAC::get_voltage_scaling(i));
             scrolling_history_[i].Push(_sample);
             _asr_buffer[i] = _sample;
         }
       
        // ... and write to DAC
        for (int i = 0; i < NUM_ASR_CHANNELS; ++i) 
          OC::DAC::set(static_cast<DAC_CHANNEL>(i), _asr_buffer[i]);
        
        MENU_REDRAW = 0x1;
      }
      for (auto &sh : scrolling_history_)
        sh.Update();
  }

  uint8_t clockState() const {
    return clock_display_.getState();
  }

  const OC::vfx::ScrollingHistory<uint16_t, kHistoryDepth> &history(int i) const {
    return scrolling_history_[i];
  }

private:
  bool force_update_;
  bool delay_type_;
  bool octave_toggle_;
  bool freeze_switch_;
  int8_t TR2_state_;
  int last_scale_;
  uint16_t last_mask_;
  braids::Quantizer quantizer_;
  OC::DigitalInputDisplay clock_display_;
  util::TriggerDelay<OC::kMaxTriggerDelayTicks> trigger_delay_;
  util::TuringShiftRegister turing_machine_;
  util::RingBuffer<ASR_pitch, ASR_MAX_ITEMS> _ASR;
  int8_t turing_display_length_;
  peaks::ByteBeat bytebeat_ ;
  util::IntegerSequence int_seq_ ;
  OC::vfx::ScrollingHistory<uint16_t, kHistoryDepth> scrolling_history_[NUM_ASR_CHANNELS];
  int num_enabled_settings_;
  ASRSettings enabled_settings_[ASR_SETTING_LAST];
};

const char* const asr_input_sources[] = {
  "CV1", "TM", "ByteB", "IntSq"
};

const char* const asr_cv4_destinations[] = {
  "oct", "root", "trns", "buf.l", "igain"
};

const char* const bb_CV_destinations[] = {
  "igain", "eqn", "P0", "P1", "P2"
};

const char* const int_seq_CV_destinations[] = {
  "igain", "seq", "strt", "len", "strd", "mod"
};


SETTINGS_DECLARE(ASR, ASR_SETTING_LAST) {
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "Scale", OC::scale_names_short, settings::STORAGE_TYPE_U8 },
  { 0, -5, 5, "octave", NULL, settings::STORAGE_TYPE_I8 }, // octave
  { 0, 0, 11, "root", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 },
  { 65535, 1, 65535, "mask", NULL, settings::STORAGE_TYPE_U16 }, // mask
  { 0, 0, ASR_HOLD_BUF_SIZE - 1, "buf.index", NULL, settings::STORAGE_TYPE_U8 },
  { MULT_ONE, 0, NUM_INPUT_SCALING - 1, "input gain", OC::Strings::mult, settings::STORAGE_TYPE_U8 },
  { 0, 0, OC::kNumDelayTimes - 1, "trigger delay", OC::Strings::trigger_delay_times, settings::STORAGE_TYPE_U8 },
  { 4, 4, ASR_HOLD_BUF_SIZE - 1, "hold (buflen)", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, ASR_CHANNEL_SOURCE_LAST -1, "CV source", asr_input_sources, settings::STORAGE_TYPE_U4 },
  { 0, 0, ASR_DEST_LAST - 1, "CV4 dest. ->", asr_cv4_destinations, settings::STORAGE_TYPE_U4 },
  { 16, 1, 32, "> LFSR length", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, "> LFSR p", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 3, "> LFSR CV1", OC::Strings::TM_aux_cv_destinations, settings::STORAGE_TYPE_U8 }, // ??
  { 0, 0, 15, "> BB eqn", OC::Strings::bytebeat_equation_names, settings::STORAGE_TYPE_U8 },
  { 8, 1, 255, "> BB P0", NULL, settings::STORAGE_TYPE_U8 },
  { 12, 1, 255, "> BB P1", NULL, settings::STORAGE_TYPE_U8 },
  { 14, 1, 255, "> BB P2", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 4, "> BB CV1", bb_CV_destinations, settings::STORAGE_TYPE_U4 },
  { 0, 0, 9, "> IntSeq", OC::Strings::integer_sequence_names, settings::STORAGE_TYPE_U4 },
  { 24, 2, 121, "> IntSeq modul", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, kIntSeqLen - 2, "> IntSeq start", NULL, settings::STORAGE_TYPE_U8 },
  { 8, 2, kIntSeqLen, "> IntSeq len", NULL, settings::STORAGE_TYPE_U8 },
  { 1, 0, 1, "> IntSeq dir", OC::Strings::integer_sequence_dirs, settings::STORAGE_TYPE_U4 },
  { 1, 1, kIntSeqLen - 1, "> Fract stride", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 5, "> IntSeq CV1", int_seq_CV_destinations, settings::STORAGE_TYPE_U4 }
};

/* -------------------------------------------------------------------*/

class ASRState {
public:

  void Init() {
    cursor.Init(ASR_SETTING_SCALE, ASR_SETTING_LAST - 1);
    scale_editor.Init(false);
    left_encoder_value = OC::Scales::SCALE_SEMI;
  }
  
  inline bool editing() const {
    return cursor.editing();
  }

  inline int cursor_pos() const {
    return cursor.cursor_pos();
  }

  int left_encoder_value;
  menu::ScreenCursor<menu::kScreenLines> cursor;
  menu::ScreenCursor<menu::kScreenLines> cursor_state;
  OC::ScaleEditor<ASR> scale_editor;
};

ASRState asr_state;
ASR asr;

void ASR_init() {

  asr.InitDefaults();
  asr.init();
  asr_state.Init(); 
  asr.update_enabled_settings();
  asr_state.cursor.AdjustEnd(asr.num_enabled_settings() - 1);
}

size_t ASR_storageSize() {
  return ASR::storageSize();
}

size_t ASR_restore(const void *storage) {
  // init nicely
  size_t storage_size = asr.Restore(storage);
  asr_state.left_encoder_value = asr.get_scale(DUMMY); 
  asr.set_scale(asr_state.left_encoder_value);
  asr.clear_freeze();
  asr.set_display_mask(asr.get_mask());
  asr.update_enabled_settings();
  asr_state.cursor.AdjustEnd(asr.num_enabled_settings() - 1);
  return storage_size;
}

void ASR_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      asr_state.cursor.set_editing(false);
      asr_state.scale_editor.Close();
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
      asr.update_enabled_settings();
      asr_state.cursor.AdjustEnd(asr.num_enabled_settings() - 1);
      break;
  }
}

void ASR_loop() {
}

void ASR_isr() {
  asr.update();
}

void ASR_handleButtonEvent(const UI::Event &event) {
  if (asr_state.scale_editor.active()) {
    asr_state.scale_editor.HandleButtonEvent(event);
    return;
  }

  if (UI::EVENT_BUTTON_PRESS == event.type) {
    switch (event.control) {
      case OC::CONTROL_BUTTON_UP:
        ASR_topButton();
        break;
      case OC::CONTROL_BUTTON_DOWN:
        ASR_lowerButton();
        break;
      case OC::CONTROL_BUTTON_L:
        ASR_leftButton();
        break;
      case OC::CONTROL_BUTTON_R:
        ASR_rightButton();
        break;
    }
  } else {
    if (OC::CONTROL_BUTTON_L == event.control)
      ASR_leftButtonLong();
    else if (OC::CONTROL_BUTTON_DOWN == event.control)
      ASR_downButtonLong();  
  }
}

void ASR_handleEncoderEvent(const UI::Event &event) {

  if (asr_state.scale_editor.active()) {
    asr_state.scale_editor.HandleEncoderEvent(event);
    return;
  }

  if (OC::CONTROL_ENCODER_L == event.control) {
    
    int value = asr_state.left_encoder_value + event.value;
    CONSTRAIN(value, 0, OC::Scales::NUM_SCALES - 1);
    asr_state.left_encoder_value = value;
    
  } else if (OC::CONTROL_ENCODER_R == event.control) {

    if (asr_state.editing()) {  

    ASRSettings setting = asr.enabled_setting_at(asr_state.cursor_pos());

    if (ASR_SETTING_MASK != setting) {

          if (asr.change_value(setting, event.value))
             asr.force_update();
             
          switch(setting) {
  
            case ASR_SETTING_CV_SOURCE:
              asr.update_enabled_settings();
              asr_state.cursor.AdjustEnd(asr.num_enabled_settings() - 1);
              // hack/hide extra options when default CV source is selected
              if (!asr.get_cv_source()) 
                asr_state.cursor.Scroll(asr_state.cursor_pos());
            break;
            default:
            break;
          }
        }
    } else {
      asr_state.cursor.Scroll(event.value);
    }
  }
}


void ASR_topButton() {
  if (asr.octave_toggle())
    asr.change_value(ASR_SETTING_OCTAVE, 1);
  else 
    asr.change_value(ASR_SETTING_OCTAVE, -1);
}

void ASR_lowerButton() {
   asr.manual_freeze();
}

void ASR_rightButton() {

  switch (asr.enabled_setting_at(asr_state.cursor_pos())) {

      case ASR_SETTING_MASK: {
        int scale = asr.get_scale(DUMMY);
        if (OC::Scales::SCALE_NONE != scale)
          asr_state.scale_editor.Edit(&asr, scale);
        }
      break;
      default:
        asr_state.cursor.toggle_editing();
      break;  
  }
}

void ASR_leftButton() {

  if (asr_state.left_encoder_value != asr.get_scale(DUMMY))
    asr.set_scale(asr_state.left_encoder_value);
}

void ASR_leftButtonLong() {

  int scale = asr_state.left_encoder_value;
  asr.set_scale(asr_state.left_encoder_value);
  if (scale != OC::Scales::SCALE_NONE) 
      asr_state.scale_editor.Edit(&asr, scale);
}

void ASR_downButtonLong() {
   asr.toggle_delay_mechanics();
}

size_t ASR_save(void *storage) {
  return asr.Save(storage);
}

void ASR_menu() {

  menu::TitleBar<0, 4, 0>::Draw();

  int scale = asr_state.left_encoder_value;
  graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);
  graphics.print(OC::scale_names[scale]);

  if (asr.freeze_state())
    graphics.drawBitmap8(1, menu::QuadTitleBar::kTextY, 4, OC::bitmap_hold_indicator_4x8); 
  else if (asr.get_scale(DUMMY) == scale)
    graphics.drawBitmap8(1, menu::QuadTitleBar::kTextY, 4, OC::bitmap_indicator_4x8);
    
  if (asr.poke_octave_toggle()) {
    graphics.setPrintPos(110, 2);
    graphics.print("+");
  }

  if (asr.get_delay_type())
    graphics.drawBitmap8(118, menu::QuadTitleBar::kTextY, 4, OC::bitmap_hold_indicator_4x8);
  else 
    graphics.drawBitmap8(118, menu::QuadTitleBar::kTextY, 4, OC::bitmap_indicator_4x8);

  uint8_t clock_state = (asr.clockState() + 3) >> 2;
  if (clock_state)
    graphics.drawBitmap8(124, 2, 4, OC::bitmap_gate_indicators_8 + (clock_state << 2));

  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(asr_state.cursor);
  menu::SettingsListItem list_item;
  
  while (settings_list.available()) {

    const int setting = asr.enabled_setting_at(settings_list.Next(list_item));
    const int value = asr.get_value(setting);
    const settings::value_attr &attr = ASR::value_attr(setting); 

    switch (setting) {

      case ASR_SETTING_MASK:
      menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, asr.get_rotated_mask(), OC::Scales::GetScale(asr.get_scale(DUMMY)).num_notes);
      list_item.DrawNoValue<false>(value, attr);
      break;
      case ASR_SETTING_CV_SOURCE:
      if (asr.get_cv_source() == ASR_CHANNEL_SOURCE_TURING) {
       
          int turing_length = asr.get_turing_display_length();
          int w = turing_length >= 16 ? 16 * 3 : turing_length * 3;

          menu::DrawMask<true, 16, 8, 1>(menu::kDisplayWidth, list_item.y, asr.get_shift_register(), turing_length);
          list_item.valuex = menu::kDisplayWidth - w - 1;
          list_item.DrawNoValue<true>(value, attr);
       } else
       list_item.DrawDefault(value, attr);  
      break;
      default:
      list_item.DrawDefault(value, attr);
      break;
    }
  }
  if (asr_state.scale_editor.active())
    asr_state.scale_editor.Draw(); 
}

uint16_t channel_history[ASR::kHistoryDepth];

void ASR_screensaver() {

// Possible variants (w x h)
// 4 x 32x64 px
// 4 x 64x32 px
// "Somehow" overlapping?
// Normalize history to within one octave? That would make steps more visisble for small ranges
// "Zoomed view" to fit range of history...

  for (int i = 0; i < NUM_ASR_CHANNELS; ++i) {
    asr.history(i).Read(channel_history);
    uint32_t scroll_pos = asr.history(i).get_scroll_pos() >> 5;
    
    int pos = 0;
    weegfx::coord_t x = i * 32, y;

    y = 63 - ((channel_history[pos++] >> 10) & 0x3f);
    graphics.drawHLine(x, y, scroll_pos);
    x += scroll_pos;
    graphics.drawVLine(x, y, 3);

    weegfx::coord_t last_y = y;
    for (int c = 0; c < 3; ++c) {
      y = 63 - ((channel_history[pos++] >> 10) & 0x3f);
      graphics.drawHLine(x, y, 8);
      if (y == last_y)
        graphics.drawVLine(x, y, 2);
      else if (y < last_y)
        graphics.drawVLine(x, y, last_y - y + 1);
      else
        graphics.drawVLine(x, last_y, y - last_y + 1);
      x += 8;
      last_y = y;
//      graphics.drawVLine(x, y, 3);
    }

    y = 63 - ((channel_history[pos++] >> 10) & 0x3f);
//    graphics.drawHLine(x, y, 8 - scroll_pos);
    graphics.drawRect(x, y, 8 - scroll_pos, 2);
    if (y == last_y)
      graphics.drawVLine(x, y, 3);
    else if (y < last_y)
      graphics.drawVLine(x, y, last_y - y + 1);
    else
      graphics.drawVLine(x, last_y, y - last_y + 1);
//    x += 8 - scroll_pos;
//    graphics.drawVLine(x, y, 3);
  }
}

#ifdef ASR_DEBUG
void ASR_debug() {
  for (int i = 0; i < 1; ++i) { 
    uint8_t ypos = 10*(i + 1) + 2 ; 
    graphics.setPrintPos(2, ypos);
    graphics.print(asr.get_int_seq_i());
    graphics.setPrintPos(32, ypos);
    graphics.print(asr.get_int_seq_l());
    graphics.setPrintPos(62, ypos);
    graphics.print(asr.get_int_seq_j());
    graphics.setPrintPos(92, ypos);
    graphics.print(asr.get_int_seq_k());
    graphics.setPrintPos(122, ypos);
    graphics.print(asr.get_int_seq_n());
 }
}
#endif // ASR_DEBUG

