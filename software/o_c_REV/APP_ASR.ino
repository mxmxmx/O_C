#include "util/util_settings.h"
#include "OC_menus.h"
#include "OC_scales.h"
#include "OC_scale_edit.h"
#include "OC_strings.h"
#include "extern/dspinst.h"

namespace menu = OC::menu; // Ugh. This works for all .ino files

#define SCALED_ADC(channel, shift) \
(0x1+(OC::ADC::value<channel>() >> shift))

#define TRANSPOSE_FIXED 0x0
#define CALIBRATION_DEFAULT_STEP 6553
#define CALIBRATION_DEFAULT_OFFSET 4890

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
  ASR_SETTING_LAST
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

  void pushASR(struct ASRbuf* _ASR, uint16_t _sample) {
 
        _ASR->items++;
        _ASR->data[_ASR->last] = _sample;
        _ASR->last = (_ASR->last+1); 
  }

  void popASR(struct ASRbuf* _ASR) {
 
        _ASR->first=(_ASR->first+1); 
        _ASR->items--;
  }

  void init() {
    
    force_update_ = false;
    last_scale_ = -1;
    last_root_ = 0;
    last_mask_ = 0;
    quantizer_.Init();
    update_scale(true);

    _ASR = (ASRbuf*)malloc(sizeof(ASRbuf));

    _ASR->first = 0;
    _ASR->last  = 0;  
    _ASR->items = 0;

    for (int i = 0; i < ASR_MAX_ITEMS; i++) {
        pushASR(_ASR, 0);    
    }

    clock_display_.Init();
  }

  bool update_scale(bool force) {
    const int scale = get_scale();
    const uint16_t mask = get_mask();
    if (force || (last_scale_ != scale || last_mask_ != mask)) {
      last_scale_ = scale;
      last_mask_ = mask;
      clocks_cnt_ = 0; // prevent ring buffer
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
        int8_t _offset = 0;
        if (!digitalReadFast(TR3)) 
          _offset++;
        else if (!digitalReadFast(TR4)) 
          _offset--;

        if (_offset) {

           uint8_t _octave;
           uint32_t _pitch;
           for (int i = 0; i < 4; i++) {
                // imprecise, but good enough ... ? 
                _pitch = asr_outputs[i];
                if (_pitch > CALIBRATION_DEFAULT_OFFSET)  {
                    _octave = (_pitch - CALIBRATION_DEFAULT_OFFSET) / CALIBRATION_DEFAULT_STEP;
                    if (_octave > 0 && _octave < 9) 
                        asr_outputs[i] += OC::calibration_data.dac.octaves[_octave + _offset] - OC::calibration_data.dac.octaves[_octave];
                }
           } 
        }       
    }  

  inline void update() {

     bool forced_update = force_update_;
     force_update_ = false;
     update_scale(forced_update);

     bool update = OC::DigitalInputs::clocked<OC::DIGITAL_INPUT_1>();
     clock_display_.Update(1, update);

      if (update) {        
   
        int8_t _root  = get_root();
        int8_t _index = get_index();
        
        clocks_cnt_++;

        if (_root != last_root_) clocks_cnt_ = 0;

        last_root_ = _root;

        _index +=  SCALED_ADC(ADC_CHANNEL_2, 5);

        if (!digitalReadFast(TR2)) _hold(_ASR, _index);   
        else {

             int8_t  _octave =  SCALED_ADC(ADC_CHANNEL_4, 9) + get_octave();
             int32_t _pitch  =  OC::ADC::value<ADC_CHANNEL_1>();
             int8_t _mult    =  get_mult() + (SCALED_ADC(ADC_CHANNEL_3, 8) - 1);  // when no signal, ADC should default to zero

             if (_mult < 0)
                _mult = 0;
             else if (_mult > 19)
                _mult = 19;
        
            // scale incoming CV
             if (_mult != 9) {
               _pitch = signed_multiply_32x16b(multipliers[_mult], _pitch);
               _pitch = signed_saturate_rshift(_pitch, 16, 0);
             }
             
             _pitch = (_pitch * 120 << 7) >> 12; // Convert to range with 128 steps per semitone
             _pitch += 3 * 12 << 7; // offset for LUT range
    
             int32_t _quantized = quantizer_.Process(_pitch, (_root + 60) << 7, TRANSPOSE_FIXED);

             if (!digitalReadFast(TR3)) _octave++;
             else if (!digitalReadFast(TR4)) _octave--;

             int32_t sample = DAC::pitch_to_dac(_quantized, _octave);

            updateASR_indexed(_ASR, sample, _index); 
         }
        // write to DAC:
        DAC::set<DAC_CHANNEL_A>(asr_outputs[0]); //  >> out 1 
        DAC::set<DAC_CHANNEL_B>(asr_outputs[1]); //  >> out 2 
        DAC::set<DAC_CHANNEL_C>(asr_outputs[2]); //  >> out 3  
        DAC::set<DAC_CHANNEL_D>(asr_outputs[3]); //  >> out 4 
        MENU_REDRAW = 1;
      }
  }

  uint8_t clockState() const {
    return clock_display_.getState();
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
};

const char* const mult[20] = {
  "0.1", "0.2", "0.3", "0.4", "0.5", "0.6", "0.7", "0.8", "0.9", "1.0", "1.1", "1.2", "1.3", "1.4", "1.5", "1.6", "1.7", "1.8", "1.9", "2.0"
};

SETTINGS_DECLARE(ASR, ASR_SETTING_LAST) {
  { OC::Scales::SCALE_SEMI, 0, OC::Scales::NUM_SCALES - 1, "Scale", OC::scale_names_short, settings::STORAGE_TYPE_U8 },
  { 0, -5, 5, "Octave", NULL, settings::STORAGE_TYPE_I8 }, // octave
  { 0, 0, 11, "Root", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 },
  { 65535, 1, 65535, "Active notes", NULL, settings::STORAGE_TYPE_U16 }, // mask
  { 0, 0, 63, "Index", NULL, settings::STORAGE_TYPE_I8 },
  { 9, 0, 19, "Mult/att", mult, settings::STORAGE_TYPE_U8 },
};

struct ASRState {
 
  int left_encoder_value;

  menu::ScreenCursor<menu::kScreenLines> cursor;
  OC::ScaleEditor<ASR> scale_editor;
};

ASRState asr_state;
ASR asr;

void ASR_init() {

  asr.InitDefaults();
  asr.init();
  asr_state.left_encoder_value  = 0;
  asr_state.cursor.Init(ASR_SETTING_ROOT, ASR_SETTING_LAST - 1);
  asr_state.scale_editor.Init();
}

size_t ASR_storageSize() {
  return ASR::storageSize();
}

size_t ASR_restore(const void *storage) {
  return asr.Restore(storage);
}

void ASR_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      asr_state.left_encoder_value = asr.get_scale();
      asr_state.cursor.set_editing(false);
      asr_state.scale_editor.Close();
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
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
    if (asr_state.cursor.editing()) {
      if (ASR_SETTING_MASK != asr_state.cursor.cursor_pos()) {
        asr.change_value(asr_state.cursor.cursor_pos(), event.value);
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

  if (asr_state.cursor.cursor_pos() != ASR_SETTING_MASK) {
    asr_state.cursor.toggle_editing();
  } else {
    int scale = asr.get_scale();
    if (OC::Scales::SCALE_NONE != scale)
      asr_state.scale_editor.Edit(&asr, scale);
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
    const int current = settings_list.Next(list_item);
    if (ASR_SETTING_MASK != current) {
      list_item.DrawDefault(asr.get_value(current), ASR::value_attr(current));
    } else {
      menu::DrawMask<false, 16>(list_item.y, asr.get_mask(), OC::Scales::GetScale(asr.get_scale()).num_notes);
      list_item.DrawNoValue<false>(asr.get_value(current), ASR::value_attr(current));
    }
  }

  if (asr_state.scale_editor.active())  
    asr_state.scale_editor.Draw();
}
