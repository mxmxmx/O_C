#include "util/util_settings.h"
#include "util/util_trigger_delay.h"
#include "util/util_turing.h"
#include "peaks_bytebeat.h"
#include "util/util_integer_sequences.h"
#include "OC_DAC.h"
#include "OC_menus.h"
#include "OC_scales.h"
#include "OC_scale_edit.h"
#include "OC_strings.h"
#include "OC_visualfx.h"
#include "extern/dspinst.h"

namespace menu = OC::menu; // Ugh. This works for all .ino files

#define SCALED_ADC(channel, shift) \
(((OC::ADC::value<channel>() + (0x1 << (shift - 1)) - 1) >> shift))

#define TRANSPOSE_FIXED 0x0

// CV input gain multipliers 
const int32_t multipliers[20] = {6554, 13107, 19661, 26214, 32768, 39322, 45875, 52429, 58982, 65536, 72090, 78643, 85197, 91750, 98304, 104858, 111411, 117964, 124518, 131072
};

enum ASRSettings {
  ASR_SETTING_SCALE,
  ASR_SETTING_OCTAVE, 
  ASR_SETTING_ROOT,
  ASR_SETTING_MASK,
  ASR_SETTING_INDEX,
  ASR_SETTING_MULT,
  ASR_SETTING_DELAY,
  ASR_SETTING_CV_SOURCE,
  ASR_SETTING_TURING_LENGTH,
  ASR_SETTING_TURING_PROB,
  ASR_SETTING_TURING_CV_SOURCE,
  ASR_SETTING_BYTEBEAT_EQUATION,
  ASR_SETTING_BYTEBEAT_P0,
  ASR_SETTING_BYTEBEAT_P1,
  ASR_SETTING_BYTEBEAT_P2,
  ASR_SETTING_BYTEBEAT_CV_SOURCE,
  ASR_SETTING_INT_SEQ_INDEX,
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

#define ASR_MAX_ITEMS 256   // ASR ring buffer size

typedef struct ASRbuf
{
    uint8_t     first;
    uint8_t     last;
    uint8_t     items;
    int32_t data[ASR_MAX_ITEMS];

} ASRbuf;

class ASR : public settings::SettingsBase<ASR, ASR_SETTING_LAST> {
public:
  static constexpr size_t kHistoryDepth = 5;

  int get_scale() const {
    return values_[ASR_SETTING_SCALE];
  }

  void set_scale(int scale) {
    if (scale != get_scale()) {
      const OC::Scale &scale_def = OC::Scales::GetScale(scale);
      uint16_t mask = get_mask();
      if (0 == (mask & ~(0xffff << scale_def.num_notes)))
        mask |= 0x1;
      apply_value(ASR_SETTING_MASK, mask);
      apply_value(ASR_SETTING_SCALE, scale);
    }
  }

  uint16_t get_mask() const {
    return values_[ASR_SETTING_MASK];
  }

  int get_root() const {
    return values_[ASR_SETTING_ROOT];
  }

  int get_index() const {
    return values_[ASR_SETTING_INDEX];
  }

  int get_octave() const {
    return values_[ASR_SETTING_OCTAVE];
  }

  int get_mult() const {
    return values_[ASR_SETTING_MULT];
  }

  int get_cv_source() const {
    return values_[ASR_SETTING_CV_SOURCE];
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

  void pushASR(struct ASRbuf* _ASR, int32_t _sample) {
 
        _ASR->items++;
        _ASR->data[_ASR->last] = _sample;
        _ASR->last = (_ASR->last+1); 
  }

  void popASR(struct ASRbuf* _ASR) {
 
        _ASR->first=(_ASR->first+1); 
        _ASR->items--;
  }

  ASRSettings enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  void init() {
    
    force_update_ = false;
    last_scale_ = -1;
    last_root_ = 0;
    last_mask_ = 0;
    quantizer_.Init();
    update_scale(true, 0);

    _ASR = (ASRbuf*)malloc(sizeof(ASRbuf));

    _ASR->first = 0;
    _ASR->last  = 0;  
    _ASR->items = 0;

    for (int i = 0; i < ASR_MAX_ITEMS; i++) {
        pushASR(_ASR, 0);    
    }

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
    
    const int scale = get_scale();
    uint16_t mask = get_mask();
    if (mask_rotate)
      mask = OC::ScaleEditor<ASR>::RotateMask(mask, OC::Scales::GetScale(scale).num_notes, mask_rotate);

    if (force || (last_scale_ != scale || last_mask_ != mask)) {

      // Change of scale resets clock, but changing mask doesn't have to
      if (force || last_scale_ != scale)
        clocks_cnt_ = 0;

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

  uint16_t get_scale_mask() const {
    return get_mask();
  }

  void update_scale_mask(uint16_t mask) {
    apply_value(ASR_SETTING_MASK, mask); // Should automatically be updated
  }
  //

  uint16_t get_rotated_mask() const {
    return last_mask_;
  }

  void update_enabled_settings() {
    
    ASRSettings *settings = enabled_settings_;

    //*settings++ = ASR_SETTING_SCALE;
    //*settings++ = ASR_SETTING_OCTAVE;
    *settings++ = ASR_SETTING_ROOT;
    *settings++ = ASR_SETTING_MASK;
    *settings++ = ASR_SETTING_INDEX; 
    *settings++ = ASR_SETTING_MULT;
    *settings++ = ASR_SETTING_DELAY;
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

  void updateASR_indexed(struct ASRbuf* _ASR, int32_t _s, int16_t _index) {

        uint8_t out;
        uint16_t _clocks = clocks_cnt_;
        int16_t _delay = _index;
        int16_t _max_delay = _clocks>>2; 
        
        popASR(_ASR);            // remove sample (oldest) 
        pushASR(_ASR, _s);       // push new sample into buffer (last) 
        
        // don't mix up scales 
        if (_delay < 0) _delay = 0;
        else if (_delay > _max_delay) _delay = _max_delay;       
          
        out  = (_ASR->last)-1;
        out -= _delay;
        asr_outputs[0] = _ASR->data[out--];
        out -= _delay;
        asr_outputs[1] = _ASR->data[out--];
        out -= _delay;
        asr_outputs[2] = _ASR->data[out--];
        out -= _delay;
        asr_outputs[3] = _ASR->data[out--];
  }

  void _hold(struct ASRbuf* _ASR, int16_t _index) {
  
        uint8_t out, _hold[4];
        int16_t _clocks = clocks_cnt_;
        int16_t _delay = _index;
        int16_t _max_delay = _clocks>>2; 

        // don't mix up scales 
        if (_delay < 0) _delay = 0;
        else if (_delay > _max_delay) _delay = _max_delay;       
      
        out  = (_ASR->last)-1;
        _hold[0] = out -= _delay;
        asr_outputs[0] = _ASR->data[out--];
        _hold[1] = out -= _delay;
        asr_outputs[1] = _ASR->data[out--];
        _hold[2] = out -= _delay;
        asr_outputs[2] = _ASR->data[out--];
        _hold[3] = out -= _delay;
        asr_outputs[3] = _ASR->data[out--];

        // hold :
        _ASR->data[_hold[0]] = asr_outputs[3];  
        _ASR->data[_hold[1]] = asr_outputs[0];
        _ASR->data[_hold[2]] = asr_outputs[1];
        _ASR->data[_hold[3]] = asr_outputs[2];

        // octave up/down
        int _offset = 0;
        if (!digitalReadFast(TR3)) 
          _offset++;
        else if (!digitalReadFast(TR4)) 
          _offset--;

        if (_offset) {
          _offset = _offset * 12 << 7;
          for (int i = 0; i < 4; i++)
            asr_outputs[i] += _offset;
        }       
    }  

  inline void update() {

     bool forced_update = force_update_;
     force_update_ = false;
     update_scale(forced_update, (OC::ADC::value<ADC_CHANNEL_3>() + 127) >> 8);

     bool update = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>();
     clock_display_.Update(1, update);

     trigger_delay_.Update();
    
     if (update)
      trigger_delay_.Push(OC::trigger_delay_ticks[get_trigger_delay()]);
      
     update = trigger_delay_.triggered();

      if (update) {        
   
        int8_t _root  = get_root();
        int8_t _index = get_index();
        
        clocks_cnt_++;

        if (_root != last_root_) 
          clocks_cnt_ = 0;

        last_root_ = _root;

        _index +=  SCALED_ADC(ADC_CHANNEL_2, 5);

        if (!digitalReadFast(TR2)) 
          _hold(_ASR, _index);   
        else {

             int8_t  _octave =  SCALED_ADC(ADC_CHANNEL_4, 9) + get_octave();
             int8_t _mult    =  get_mult();
             int32_t _pitch  = OC::ADC::raw_pitch_value(ADC_CHANNEL_1);

             switch (get_cv_source()) {
                case ASR_CHANNEL_SOURCE_TURING:
                  {
                  int16_t _length = get_turing_length();
                  int16_t _probability = get_turing_probability();
  
                  // _pitch can do other things now -- 
                  switch (get_turing_CV()) {
  
                      case 1:  // LEN, 4-32
                       _length += ((_pitch + 127) >> 7);
                       CONSTRAIN(_length, 4, 32);
                      break;
                       case 2:  // P
                       _probability += ((_pitch + 15) >> 4);
                       CONSTRAIN(_probability, 0, 255);
                      break;
                      default: // mult
                       _mult += ((_pitch + 255) >> 8);
                      break;
                  }
                  
                  turing_machine_.set_length(_length);
                  turing_machine_.set_probability(_probability); 
                  turing_display_length_ = _length;
    
                  int32_t _shift_register = (static_cast<int16_t>(turing_machine_.Clock()) & 0xFFF); 
                  
                 // return useful values for small values of _length:
                 if (_length < 12) {
                     _shift_register = _shift_register << (12 -_length);
                     _shift_register = _shift_register & 0xFFF;
                  }
                  
                  _pitch = _shift_register;  
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
                       _mult += ((_pitch + 255) >> 8);
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
                 int16_t _int_seq_start = get_int_seq_start() ;
                 int16_t _int_seq_length = get_int_seq_length();
                 int16_t _fractal_seq_stride = get_fractal_seq_stride();
                 bool _int_seq_dir = get_int_seq_dir();

                  // _pitch can do other things now -- 
                  switch (get_int_seq_CV()) {
  
                      case 1:  // integer sequence, 0-7
                       _int_seq_index += ((_pitch + 127) >> 9);
                       CONSTRAIN(_int_seq_index, 0, 7);
                      break;
                       case 2:  // sequence start point, 0-254
                       _int_seq_start += ((_pitch + 15) >> 8);
                       CONSTRAIN(_int_seq_start, 0, 254);
                      break;
                       case 3:  // sequence loop length, 1-255
                       _int_seq_length += ((_pitch + 15) >> 8);
                       CONSTRAIN(_int_seq_length, 1, 255);
                      break;
                       case 4:  // fractal sequence stride length, 1-255
                       _fractal_seq_stride += ((_pitch + 15) >> 8);
                       CONSTRAIN(_fractal_seq_stride, 1, 255);
                      break;
                      default: // mult
                       _mult += ((_pitch + 255) >> 8);
                      break;
                  }
                 
                  int_seq_.set_loop_start(_int_seq_start);
                  int_seq_.set_loop_length(_int_seq_length);
                  int_seq_.set_int_seq(_int_seq_index);
                  int_seq_.set_loop_direction(_int_seq_dir);
                  int_seq_.set_fractal_stride(_fractal_seq_stride);

                  int32_t _is = (static_cast<int16_t>(int_seq_.Clock()) & 0xFFF);
                   
                  _pitch = _is;  
                 }
                  break;      

                  default:
                  break;
            } 

            CONSTRAIN(_mult, 0, 19);
             
            // scale incoming CV
             if (_mult != 9) {
               _pitch = signed_multiply_32x16b(multipliers[_mult], _pitch);
               _pitch = signed_saturate_rshift(_pitch, 16, 0);
             }
             
             int32_t _quantized = quantizer_.Process(_pitch, _root << 7, TRANSPOSE_FIXED);

             if (!digitalReadFast(TR3)) 
                _octave++;
             else if (!digitalReadFast(TR4)) 
                _octave--;

             _quantized += (_octave * 12 << 7);

             updateASR_indexed(_ASR, _quantized, _index); 
         }

        for (int i = 0; i < 4; ++i) {
          int32_t sample = OC::DAC::pitch_to_dac(static_cast<DAC_CHANNEL>(i), asr_outputs[i], 0);
          scrolling_history_[i].Push(sample);
          OC::DAC::set(static_cast<DAC_CHANNEL>(i), sample);
        }

        MENU_REDRAW = 1;
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
  int last_scale_;
  int last_root_;
  uint16_t last_mask_;
  uint32_t clocks_cnt_;
  braids::Quantizer quantizer_;
  int32_t asr_outputs[4];  
  ASRbuf *_ASR;
  OC::DigitalInputDisplay clock_display_;
  util::TriggerDelay<OC::kMaxTriggerDelayTicks> trigger_delay_;
  util::TuringShiftRegister turing_machine_;
  int8_t turing_display_length_;
  peaks::ByteBeat bytebeat_ ;
  util::IntegerSequence int_seq_ ;
  OC::vfx::ScrollingHistory<uint16_t, kHistoryDepth> scrolling_history_[4];

  int num_enabled_settings_;
  ASRSettings enabled_settings_[ASR_SETTING_LAST];
};

const char* const mult[20] = {
  "0.1", "0.2", "0.3", "0.4", "0.5", "0.6", "0.7", "0.8", "0.9", "1.0", "1.1", "1.2", "1.3", "1.4", "1.5", "1.6", "1.7", "1.8", "1.9", "2.0"
};

const char* const asr_input_sources[] = {
  "CV1", "TM", "ByteB", "IntSq"
};

const char* const tm_CV_destinations[] = {
  "M/A", "LEN", "P(x)"
};

const char* const bb_CV_destinations[] = {
  "M/A", "EQN", "P0", "P1", "P2"
};


const char* const int_seq_CV_destinations[] = {
  "M/A", "seq", "strt", "len", "strd"
};


SETTINGS_DECLARE(ASR, ASR_SETTING_LAST) {
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "Scale", OC::scale_names_short, settings::STORAGE_TYPE_U8 },
  { 0, -5, 5, "octave", NULL, settings::STORAGE_TYPE_I8 }, // octave
  { 0, 0, 11, "root", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 },
  { 65535, 1, 65535, "active notes", NULL, settings::STORAGE_TYPE_U16 }, // mask
  { 0, 0, 63, "index", NULL, settings::STORAGE_TYPE_I8 },
  { 9, 0, 19, "mult/att", mult, settings::STORAGE_TYPE_U8 },
  { 0, 0, OC::kNumDelayTimes - 1, "Trigger delay", OC::Strings::trigger_delay_times, settings::STORAGE_TYPE_U4 },
  { 0, 0, ASR_CHANNEL_SOURCE_LAST -1 , "CV source", asr_input_sources, settings::STORAGE_TYPE_U4 },
  { 16, 4, 32, " > LFSR length", NULL, settings::STORAGE_TYPE_U8 },
  { 128, 0, 255, " > LFSR p", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 2, " > LFSR CV1", tm_CV_destinations, settings::STORAGE_TYPE_U4 },
  { 0, 0, 15, "> BB eqn", OC::Strings::bytebeat_equation_names, settings::STORAGE_TYPE_U8 },
  { 8, 1, 255, "> BB P0", NULL, settings::STORAGE_TYPE_U8 },
  { 12, 1, 255, "> BB P1", NULL, settings::STORAGE_TYPE_U8 },
  { 14, 1, 255, "> BB P2", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 4, " > BB CV1", bb_CV_destinations, settings::STORAGE_TYPE_U4 },
  { 0, 0, 7, "> IntSeq", OC::Strings::integer_sequence_names, settings::STORAGE_TYPE_U4 },
  { 0, 0, 254, "> IntSeq start", NULL, settings::STORAGE_TYPE_U8 },
  { 8, 2, 256, "> IntSeq len", NULL, settings::STORAGE_TYPE_U8 },
  { 1, 0, 1, "> IntSeq dir", OC::Strings::integer_sequence_dirs, settings::STORAGE_TYPE_U4 },
  { 1, 1, 255, "> Fract stride", NULL, settings::STORAGE_TYPE_U8 },
  { 0, 0, 4, " > IntSeq CV1", int_seq_CV_destinations, settings::STORAGE_TYPE_U4 },   
};

/* -------------------------------------------------------------------*/

class ASRState {
public:

  void Init() {
    cursor.Init(ASR_SETTING_SCALE, ASR_SETTING_LAST - 1);
    scale_editor.Init();
    left_encoder_value = 0;
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
  asr.update_enabled_settings();
  asr_state.cursor.AdjustEnd(asr.num_enabled_settings() - 1);
  return asr.Restore(storage);
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

  asr.change_value(ASR_SETTING_OCTAVE, 1);
}

void ASR_lowerButton() {

   asr.change_value(ASR_SETTING_OCTAVE, -1); 
}

void ASR_rightButton() {

  switch (asr.enabled_setting_at(asr_state.cursor_pos())) {

      case ASR_SETTING_MASK: {
        int scale = asr.get_scale();
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

  if (asr_state.left_encoder_value != asr.get_scale())
    asr.set_scale(asr_state.left_encoder_value);
}

void ASR_leftButtonLong() {

  int scale = asr_state.left_encoder_value;
  asr.set_scale(asr_state.left_encoder_value);
  if (scale != OC::Scales::SCALE_NONE) 
      asr_state.scale_editor.Edit(&asr, scale);
}

size_t ASR_save(void *storage) {
  return asr.Save(storage);
}

void ASR_menu() {

  menu::TitleBar<0, 4, 0>::Draw();

  int scale = asr_state.left_encoder_value;
  graphics.movePrintPos(weegfx::Graphics::kFixedFontW, 0);
  graphics.print(OC::scale_names[scale]);
  if (asr.get_scale() == scale)
    graphics.drawBitmap8(1, menu::QuadTitleBar::kTextY, 4, OC::bitmap_indicator_4x8);

  menu::TitleBar<0, 4, 0>::SetColumn(3);
  graphics.pretty_print(asr.get_octave());

  uint8_t clock_state = (asr.clockState() + 3) >> 2;
  if (clock_state)
    graphics.drawBitmap8(121, 2, 4, OC::bitmap_gate_indicators_8 + (clock_state << 2));

  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(asr_state.cursor);
  menu::SettingsListItem list_item;
  
  while (settings_list.available()) {

    const int setting = asr.enabled_setting_at(settings_list.Next(list_item));
    const int value = asr.get_value(setting);
    const settings::value_attr &attr = ASR::value_attr(setting); 

    switch (setting) {

      case ASR_SETTING_MASK:
      menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, asr.get_rotated_mask(), OC::Scales::GetScale(asr.get_scale()).num_notes);
      list_item.DrawNoValue<false>(value, attr);
      break;
      case ASR_SETTING_CV_SOURCE:
      if (asr.get_cv_source() == ASR_CHANNEL_SOURCE_TURING) {
       
          int turing_length = asr.get_turing_display_length(); // asr.get_turing_length();
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

  for (int i = 0; i < 4; ++i) {
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
