// Copyright (c) 2016 Patrick Dowling, 2017 Max Stadler & Tim Churches
//
// Author: Patrick Dowling (pld@gurkenkiste.com)
// Enhancements: Max Stadler and Tim Churches
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
// Very simple "reference" voltage app

#include "OC_apps.h"
#include "OC_menus.h"
#include "OC_strings.h"
#include "util/util_settings.h"
#include "OC_autotuner.h"
#include "src/drivers/FreqMeasure/OC_FreqMeasure.h"

static constexpr float kC0Frequency = 16.3515; // Frequency for C0 (4 octaves below middle C, which is C4).
const uint8_t NUM_REF_CHANNELS = DAC_CHANNEL_LAST;

enum ReferenceSetting {
  REF_SETTING_OCTAVE,
  REF_SETTING_SEMI,
  REF_SETTING_RANGE,
  REF_SETTING_RATE,
  REF_SETTING_NOTES_OR_BPM,
  REF_SETTING_PPQN,
  REF_SETTING_AUTOTUNE,
  REF_SETTING_AUTOTUNE_ERROR,
  REF_SETTING_DUMMY,
  #ifdef BUCHLA_SUPPORT
    REF_SETTING_VOLTAGE_SCALING,
  #endif 
  REF_SETTING_LAST
};

enum ChannelPpqn {
  CHANNEL_PPQN_1,
  CHANNEL_PPQN_2,
  CHANNEL_PPQN_4,
  CHANNEL_PPQN_8,
  CHANNEL_PPQN_16,
  CHANNEL_PPQN_24,
  CHANNEL_PPQN_32,
  CHANNEL_PPQN_48,
  CHANNEL_PPQN_64,
  CHANNEL_PPQN_96,
  CHANNEL_PPQN_LAST
};

enum AUTO_ERROR {
  ERROR_0_050,
  ERROR_0_125,
  ERROR_0_250,
  ERROR_0_500,
  ERROR_1_000,
  ERROR_2_000,
  ERROR_4_000,
  ERROR_LAST
};

class ReferenceChannel : public settings::SettingsBase<ReferenceChannel, REF_SETTING_LAST> {
public:
  void Init(DAC_CHANNEL dac_channel) {
    InitDefaults();

    rate_phase_ = 0;
    mod_offset_ = 0;
    last_pitch_ = 0;
    dac_channel_ = dac_channel;
  
    update_enabled_settings();
  }

  int get_octave() const {
    return values_[REF_SETTING_OCTAVE];
  }

  int get_channel() const {
    return dac_channel_;
  }

  int32_t get_semitone() const {
    return values_[REF_SETTING_SEMI];
  }

  int get_range() const {
    return values_[REF_SETTING_RANGE];
  }

  uint32_t get_rate() const {
    return values_[REF_SETTING_RATE];
  }

  uint8_t get_notes_or_bpm() const {
    return values_[REF_SETTING_NOTES_OR_BPM];
  }

  ChannelPpqn get_channel_ppqn() const {
    return static_cast<ChannelPpqn>(values_[REF_SETTING_PPQN]);
  } 

  void run_autotuner() {
    // todo....
  }

  uint8_t get_voltage_scaling() const {
    #ifdef BUCHLA_SUPPORT
      return values_[REF_SETTING_VOLTAGE_SCALING];
    #else
      return 0x0;
    #endif
  }
 
  void Update() {

    int octave = get_octave();
    int range = get_range();
    if (range) {
      rate_phase_ += OC_CORE_TIMER_RATE;
      if (rate_phase_ >= get_rate() * 1000000UL) {
        rate_phase_ = 0;
        mod_offset_ = 1 - mod_offset_;
      }
      octave += mod_offset_ * range;
    } else {
      rate_phase_ = 0;
      mod_offset_ = 0;
    }

    int32_t semitone = get_semitone();
    OC::DAC::set(dac_channel_, OC::DAC::semitone_to_scaled_voltage_dac(dac_channel_, semitone, octave, get_voltage_scaling()));
    last_pitch_ = (semitone + octave * 12) << 7;       
  }

  int num_enabled_settings() const {
    return num_enabled_settings_;
  }

  ReferenceSetting enabled_setting_at(int index) const {
    return enabled_settings_[index];
  }

  void update_enabled_settings() {
    ReferenceSetting *settings = enabled_settings_;
    *settings++ = REF_SETTING_OCTAVE;
    *settings++ = REF_SETTING_SEMI;
    *settings++ = REF_SETTING_RANGE;
    *settings++ = REF_SETTING_RATE;
    *settings++ = REF_SETTING_AUTOTUNE;
    *settings++ = REF_SETTING_AUTOTUNE_ERROR;

    if (DAC_CHANNEL_D == dac_channel_) {
      *settings++ = REF_SETTING_NOTES_OR_BPM;
      *settings++ = REF_SETTING_PPQN;
    }
    else {
      *settings++ = REF_SETTING_DUMMY;
      *settings++ = REF_SETTING_DUMMY;
    }
    
    #ifdef BUCHLA_SUPPORT
      *settings++ = REF_SETTING_VOLTAGE_SCALING;
    #endif
     num_enabled_settings_ = settings - enabled_settings_;
  }

  void RenderScreensaver(weegfx::coord_t start_x, uint8_t chan) const;

private:
  uint32_t rate_phase_;
  int mod_offset_;
  int32_t last_pitch_;
  DAC_CHANNEL dac_channel_;

  int num_enabled_settings_;
  ReferenceSetting enabled_settings_[REF_SETTING_LAST];
};

const char* const notes_or_bpm[2] = {
 "notes",  "bpm", 
};

const char* const ppqn_labels[10] = {
 " 1",  " 2", " 4", " 8", "16", "24", "32", "48", "64", "96",  
};

const char* const error[] = {
  "0.050", "0.125", "0.250", "0.500", "1.000", "2.000", "4.000"
};

SETTINGS_DECLARE(ReferenceChannel, REF_SETTING_LAST) {
  { 0, -3, 6, "Octave", nullptr, settings::STORAGE_TYPE_I8 },
  { 0, 0, 11, "Semitone", OC::Strings::note_names_unpadded, settings::STORAGE_TYPE_U8 },
  { 0, -3, 3, "Mod range oct", nullptr, settings::STORAGE_TYPE_U8 },
  { 0, 0, 30, "Mod rate (s)", nullptr, settings::STORAGE_TYPE_U8 },
  { 0, 0, 1, "Notes/BPM :", notes_or_bpm, settings::STORAGE_TYPE_U8 },
  { CHANNEL_PPQN_4, CHANNEL_PPQN_1, CHANNEL_PPQN_LAST - 1, "> ppqn", ppqn_labels, settings::STORAGE_TYPE_U8 },
  { 0, 0, 0, "--> autotune", NULL, settings::STORAGE_TYPE_U8 },
  { 3, 0, ERROR_LAST - 1, "> error (Hz)", error, settings::STORAGE_TYPE_U8 },
  { 0, 0, 0, "-", NULL, settings::STORAGE_TYPE_U4 }, // dummy
  #ifdef BUCHLA_SUPPORT
  { 0, 0, 2, "V/octave", OC::voltage_scalings, settings::STORAGE_TYPE_U8 }
  #endif
};

class ReferencesApp {
public:
  ReferencesApp() { }
  
  OC::Autotuner<ReferenceChannel> autotuner;

  void Init() {
    int dac_channel = DAC_CHANNEL_A;
    for (auto &channel : channels_)
      channel.Init(static_cast<DAC_CHANNEL>(dac_channel++));

    ui.selected_channel = DAC_CHANNEL_D;
    ui.cursor.Init(0, channels_[DAC_CHANNEL_D].num_enabled_settings() - 1);

    freq_sum_ = 0;
    freq_count_ = 0;
    frequency_ = 0;
    freq_decicents_deviation_ = 0;
    freq_octave_ = 0;
    freq_note_ = 0;
    freq_decicents_residual_ = 0;
    autotuner.Init();
  }

  void ISR() {
    for (auto &channel : channels_)
      channel.Update();

      if (FreqMeasure.available()) {
        // average several readings together
        freq_sum_ = freq_sum_ + FreqMeasure.read();
        freq_count_ = freq_count_ + 1;
        if (milliseconds_since_last_freq_ > 1000) {
          frequency_ = FreqMeasure.countToFrequency(freq_sum_ / freq_count_);
          freq_sum_ = 0;
          freq_count_ = 0;
          milliseconds_since_last_freq_ = 0;
          freq_decicents_deviation_ = round(12000.0 * log2f(frequency_ / kC0Frequency)) + 500;
          freq_octave_ = -2 + ((freq_decicents_deviation_)/ 12000) ;
          freq_note_ = (freq_decicents_deviation_ - ((freq_octave_ + 2) * 12000)) / 1000;
          freq_decicents_residual_ = ((freq_decicents_deviation_ - ((freq_octave_ - 1) * 12000)) % 1000) - 500;
        }
      }
      
  }

  ReferenceChannel &selected_channel() {
    return channels_[ui.selected_channel];
  }

  struct {
    int selected_channel;
    menu::ScreenCursor<menu::kScreenLines> cursor;
  } ui;

  ReferenceChannel channels_[DAC_CHANNEL_LAST];

  float get_frequency( ) {
    return(frequency_) ;
  }

  float get_ppqn() {
    float ppqn_ = 4.0 ;
    switch(channels_[DAC_CHANNEL_D].get_channel_ppqn()){
      case CHANNEL_PPQN_1:
        ppqn_ = 1.0;
        break;
      case CHANNEL_PPQN_2:
        ppqn_ = 2.0;
        break;
      case CHANNEL_PPQN_4:
        ppqn_ = 4.0;
        break;
      case CHANNEL_PPQN_8:
        ppqn_ = 8.0;
        break;
      case CHANNEL_PPQN_16:
        ppqn_ = 16.0;
        break;
      case CHANNEL_PPQN_24:
        ppqn_ = 24.0;
        break;
      case CHANNEL_PPQN_32:
        ppqn_ = 32.0;
        break;
      case CHANNEL_PPQN_48:
        ppqn_ = 48.0;
        break;
      case CHANNEL_PPQN_64:
        ppqn_ = 64.0;
        break;
      case CHANNEL_PPQN_96:
        ppqn_ = 96.0;
        break;
      default:
        ppqn_ = 8.0 ;
        break;
    }
    return(ppqn_);
  }

  float get_bpm( ) {
    return((60.0 * frequency_)/get_ppqn()) ;
  }

  bool get_notes_or_bpm( ) {
    return(static_cast<bool>(channels_[DAC_CHANNEL_D].get_notes_or_bpm())) ;
  }

  float get_cents_deviation( ) {
    return(static_cast<float>(freq_decicents_deviation_) / 10.0) ;
  }
  
  float get_cents_residual( ) {
    return(static_cast<float>(freq_decicents_residual_) / 10.0) ;
  }
  
  int8_t get_octave( ) {
    return(freq_octave_) ;
  }
  
  int8_t get_note( ) {
    return(freq_note_) ;
  }

private:
  double freq_sum_;
  uint32_t freq_count_;
  float frequency_ ;
  elapsedMillis milliseconds_since_last_freq_;
  int32_t freq_decicents_deviation_;
  int8_t freq_octave_ ;
  int8_t freq_note_;
  int32_t freq_decicents_residual_;
};

ReferencesApp references_app;

// App stubs
void REFS_init() {
  references_app.Init();
}

size_t REFS_storageSize() {
  return NUM_REF_CHANNELS * ReferenceChannel::storageSize();
}

size_t REFS_save(void *storage) {
  size_t used = 0;
  for (size_t i = 0; i < NUM_REF_CHANNELS; ++i) {
    used += references_app.channels_[i].Save(static_cast<char*>(storage) + used);
  }
  return used;
}

size_t REFS_restore(const void *storage) {
  size_t used = 0;
  for (size_t i = 0; i < NUM_REF_CHANNELS; ++i) {
    used += references_app.channels_[i].Restore(static_cast<const char*>(storage) + used);
    references_app.channels_[i].update_enabled_settings();
  }
  references_app.ui.cursor.AdjustEnd(references_app.channels_[0].num_enabled_settings() - 1);
  return used;
}

void REFS_isr() {
  return references_app.ISR();
}

void REFS_handleAppEvent(OC::AppEvent event) {
  switch (event) {
    case OC::APP_EVENT_RESUME:
      references_app.ui.cursor.set_editing(false);
      FreqMeasure.begin();
      references_app.autotuner.Close();
      break;
    case OC::APP_EVENT_SUSPEND:
    case OC::APP_EVENT_SCREENSAVER_ON:
    case OC::APP_EVENT_SCREENSAVER_OFF:
      break;
  }
}

void REFS_loop() {
}

void REFS_menu() {
  menu::QuadTitleBar::Draw();
  for (uint_fast8_t i = 0; i < NUM_REF_CHANNELS; ++i) {
    menu::QuadTitleBar::SetColumn(i);
    graphics.print((char)('A' + i));
  }
  menu::QuadTitleBar::Selected(references_app.ui.selected_channel);

  const auto &channel = references_app.selected_channel();
  menu::SettingsList<menu::kScreenLines, 0, menu::kDefaultValueX> settings_list(references_app.ui.cursor);
  menu::SettingsListItem list_item;

  while (settings_list.available()) {
    const int setting = 
      channel.enabled_setting_at(settings_list.Next(list_item));
    const int value = channel.get_value(setting);
    const settings::value_attr &attr = ReferenceChannel::value_attr(setting);

    switch (setting) {
      case REF_SETTING_AUTOTUNE:
      case REF_SETTING_DUMMY:
         list_item.DrawNoValue<false>(value, attr);
      break;
      default:
        list_item.DrawDefault(value, attr);
      break;
    }
  }
  // autotuner ...
  if (references_app.autotuner.active())
    references_app.autotuner.Draw(); 
}

void print_voltage(int octave, int fraction) {
  graphics.printf("%01d", octave);
  graphics.movePrintPos(-1, 0); graphics.print('.');
  graphics.movePrintPos(-2, 0); graphics.printf("%03d", fraction);
}

void ReferenceChannel::RenderScreensaver(weegfx::coord_t start_x, uint8_t chan) const {

  // Mostly borrowed from QQ

  weegfx::coord_t x = start_x + 26;
  weegfx::coord_t y = 34 ; // was 60
  // for (int i = 0; i < 5 ; ++i, y -= 4) // was i < 12
    graphics.setPixel(x, y);

  int32_t pitch = last_pitch_ ;
  int32_t unscaled_pitch = last_pitch_ ;

  switch (references_app.channels_[chan].get_voltage_scaling()) {
      case 1: // 1.2V/oct
          pitch = (pitch * 19661) >> 14 ;
          break;
      case 2: // 2V/oct
          pitch = pitch << 1 ;
          break;
      default: // 1V/oct
          break;
    }

  pitch += (OC::DAC::kOctaveZero * 12) << 7;
  unscaled_pitch += (OC::DAC::kOctaveZero * 12) << 7;

  
  CONSTRAIN(pitch, 0, 120 << 7);

  int32_t octave = pitch / (12 << 7);
  int32_t unscaled_octave = unscaled_pitch / (12 << 7);
  pitch -= (octave * 12 << 7);
  unscaled_pitch -= (unscaled_octave * 12 << 7);
  int semitone = pitch >> 7;
  int unscaled_semitone = unscaled_pitch >> 7;

  y = 34 - unscaled_semitone * 2; // was 60, multiplier was 4
  if (unscaled_semitone < 6)
    graphics.setPrintPos(start_x + menu::kIndentDx, y - 7);
  else
    graphics.setPrintPos(start_x + menu::kIndentDx, y);
  graphics.print(OC::Strings::note_names_unpadded[unscaled_semitone]);

  graphics.drawHLine(start_x + 16, y, 8);
  graphics.drawBitmap8(start_x + 28, 34 - unscaled_octave * 2 - 1, OC::kBitmapLoopMarkerW, OC::bitmap_loop_markers_8 + OC::kBitmapLoopMarkerW); // was 60

  // Try and round to 3 digits
  switch (references_app.channels_[chan].get_voltage_scaling()) {
      case 1: // 1.2V/oct
          semitone = (semitone * 10000 + 40) / 100;
          break;
      case 2: // 2V/oct
      default: // 1V/oct
          semitone = (semitone * 10000 + 50) / 120;
          break;
    }
  
  semitone %= 1000;
  octave -= OC::DAC::kOctaveZero;


  // We want [sign]d.ddd = 6 chars in 32px space; with the current font width
  // of 6px that's too tight, so squeeze in the mini minus...
  y = menu::kTextDy;
  graphics.setPrintPos(start_x + menu::kIndentDx, y);
  if (octave >= 0) {
    print_voltage(octave, semitone);
  } else {
    graphics.drawHLine(start_x, y + 3, 2);
    if (semitone)
      print_voltage(-octave - 1, 1000 - semitone);
    else
      print_voltage(-octave, 0);
  }
}

void REFS_screensaver() {
  references_app.channels_[0].RenderScreensaver( 0, 0);
  references_app.channels_[1].RenderScreensaver(32, 1);
  references_app.channels_[2].RenderScreensaver(64, 2);
  references_app.channels_[3].RenderScreensaver(96, 3);
  graphics.setPrintPos(2, 44);
  graphics.printf("TR4 %7.3f Hz", references_app.get_frequency()) ;
  graphics.setPrintPos(2, 56);
  if (references_app.get_notes_or_bpm()) {
    graphics.printf("%7.2f bpm %2.0fppqn", references_app.get_bpm(), references_app.get_ppqn());
  } else if(references_app.get_frequency() >= kC0Frequency) {
    graphics.printf("%+i %s %+7.1fc", references_app.get_octave(), OC::Strings::note_names[references_app.get_note()], references_app.get_cents_residual()) ;
  }
}

void REFS_handleButtonEvent(const UI::Event &event) {

  if (references_app.autotuner.active()) {
    references_app.autotuner.HandleButtonEvent(event);
    return;
  }
  
  if (OC::CONTROL_BUTTON_R == event.control) {

    auto &selected_channel = references_app.selected_channel();
    switch (selected_channel.enabled_setting_at(references_app.ui.cursor.cursor_pos())) {
      case REF_SETTING_AUTOTUNE:
      references_app.autotuner.Open(&selected_channel);
      break;
      case REF_SETTING_DUMMY:
      break;
      default:
      references_app.ui.cursor.toggle_editing();
      break;
    }
  }
}

void REFS_handleEncoderEvent(const UI::Event &event) {

  if (references_app.autotuner.active()) {
    references_app.autotuner.HandleEncoderEvent(event);
    return;
  }
  
  if (OC::CONTROL_ENCODER_L == event.control) {
    int selected = references_app.ui.selected_channel + event.value;
    CONSTRAIN(selected, 0, NUM_REF_CHANNELS - 0x1);
    references_app.ui.selected_channel = selected;
    references_app.ui.cursor.AdjustEnd(references_app.selected_channel().num_enabled_settings() - 1);
  } else if (OC::CONTROL_ENCODER_R == event.control) {
    if (references_app.ui.cursor.editing()) {
        auto &selected_channel = references_app.selected_channel();
        ReferenceSetting setting = selected_channel.enabled_setting_at(references_app.ui.cursor.cursor_pos());
        if (setting == REF_SETTING_DUMMY) 
          references_app.ui.cursor.set_editing(false);
        selected_channel.change_value(setting, event.value);
        selected_channel.update_enabled_settings();
    } else {
      references_app.ui.cursor.Scroll(event.value);
    }
  }
}
