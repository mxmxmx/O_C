#include "util/util_settings.h"
#include "OC_DAC.h"
#include "OC_menus.h"
#include "OC_scales.h"
#include "OC_scale_edit.h"
#include "OC_strings.h"
#include "OC_visualfx.h"
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
             int8_t _mult    =  get_mult();

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
            
             int32_t _quantized = quantizer_.Process(_pitch, _root << 7, TRANSPOSE_FIXED);

             if (!digitalReadFast(TR3)) _octave++;
             else if (!digitalReadFast(TR4)) _octave--;

             _quantized += (_octave * 12 << 7);
             // limit: 
             while (_quantized < 0) 
             {
                _quantized += 12 << 7;
             }

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

  OC::vfx::ScrollingHistory<uint16_t, kHistoryDepth> scrolling_history_[4];
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
      menu::DrawMask<false, 16, 8, 1>(menu::kDisplayWidth, list_item.y, asr.get_rotated_mask(), OC::Scales::GetScale(asr.get_scale()).num_notes);
      list_item.DrawNoValue<false>(asr.get_value(current), ASR::value_attr(current));
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
