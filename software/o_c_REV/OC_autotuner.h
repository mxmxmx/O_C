#ifndef OC_AUTOTUNER_H
#define OC_AUTOTUNER_H

#include "OC_autotune.h"
#include "OC_options.h"

#if defined(BUCHLA_4U) && !defined(IO_10V)
const char* const AT_steps[] = {
  "0.0V", "1.2V", "2.4V", "3.6V", "4.8V", "6.0V", "7.2V", "8.4V", "9.6V", "10.8V", " " 
};
#elif defined(IO_10V)
const char* const AT_steps[] = {
  "0.0V", "1.0V", "2.0V", "3.0V", "4.0V", "5.0V", "6.0V", "7.0V", "8.0V", "9.0V", " " 
};
#else
const char* const AT_steps[] = {
  "-3V", "-2V", "-1V", " 0V", "+1V", "+2V", "+3V", "+4V", "+5V", "+6V", " " 
};
#endif

namespace OC {

enum AUTO_MENU_ITEMS {
  DATA_SELECT,
  AUTOTUNE,
  AUTORUN,
  AUTO_MENU_ITEMS_LAST
};

enum AT_STATUS {
   AT_OFF,
   AT_READY,
   AT_RUN,
   AT_ERROR,
   AT_DONE,
   AT_LAST,
};

enum AUTO_CALIBRATION_STEP {
  DAC_VOLT_0_ARM,
  DAC_VOLT_0_BASELINE,
  DAC_VOLT_TARGET_FREQUENCIES,
  DAC_VOLT_3m, 
  DAC_VOLT_2m, 
  DAC_VOLT_1m, 
  DAC_VOLT_0, 
  DAC_VOLT_1, 
  DAC_VOLT_2, 
  DAC_VOLT_3, 
  DAC_VOLT_4, 
  DAC_VOLT_5, 
  DAC_VOLT_6,
  AUTO_CALIBRATION_STEP_LAST
};

template <typename Owner>
class Autotuner {

public:

  void Init() {
    owner_ = nullptr;
    cursor_pos_ = 0;
    data_select_ = 0;
    channel_ = 0;
    calibration_data_ = 0;
    auto_tune_running_status_ = 0;
  }

  bool active() const {
    return nullptr != owner_;
  }

  void Open(Owner *owner) {
    owner_ = owner;
    channel_ = owner_->get_channel();
    Begin();
  }

  void Close();
  void Draw();
  void HandleButtonEvent(const UI::Event &event);
  void HandleEncoderEvent(const UI::Event &event);
  
private:

  Owner *owner_;
  size_t cursor_pos_;
  size_t data_select_;
  int8_t channel_;
  uint8_t calibration_data_;
  uint8_t auto_tune_running_status_;

  void Begin();
  void move_cursor(int offset);
  void change_value(int offset);
  void handleButtonLeft(const UI::Event &event);
  void handleButtonUp(const UI::Event &event);
  void handleButtonDown(const UI::Event &event);
};

  template <typename Owner>
  void Autotuner<Owner>::Draw() {
    
    weegfx::coord_t w = 128;
    weegfx::coord_t x = 0;
    weegfx::coord_t y = 0;
    weegfx::coord_t h = 64;

    graphics.clearRect(x, y, w, h);
    graphics.drawFrame(x, y, w, h);
    graphics.setPrintPos(x + 2, y + 3);
    graphics.print(OC::Strings::channel_id[channel_]);

    x = 16; y = 15;

    if (owner_->autotuner_error()) {
      auto_tune_running_status_ = AT_ERROR;
      owner_->reset_autotuner();
    }
    else if (owner_->autotuner_completed()) {
      auto_tune_running_status_ = AT_DONE;
      owner_->autotuner_reset_completed();
    }
    
    for (size_t i = 0; i < (AUTO_MENU_ITEMS_LAST - 0x1); ++i, y += 20) {
        //
      graphics.setPrintPos(x + 2, y + 4);
      
      if (i == DATA_SELECT) {
        graphics.print("use --> ");
        
        switch(calibration_data_) {
          case 0x0:
          graphics.print("(dflt.)");
          break;
          case 0x01:
          graphics.print("auto.");
          break;
          default:
          graphics.print("dflt.");
          break;
        }
      }
      else if (i == AUTOTUNE) {
        
        switch (auto_tune_running_status_) {
        //to display progress, if running
        case AT_OFF:
        graphics.print("run --> ");
        graphics.print(" ... ");
        break;
        case AT_READY: {
        graphics.print("arm > ");
        float _freq = owner_->get_auto_frequency();
        if (_freq == 0.0f)
          graphics.printf("wait ...");
        else 
          graphics.printf("%7.3f", _freq);
        }
        break;
        case AT_RUN:
        {
          int _octave = owner_->get_octave_cnt();
          if (_octave > 1 && _octave < OCTAVES) {
            for (int i = 0; i <= _octave; i++, x += 6)
              graphics.drawBitmap8(x + 18, y + 4, 4, OC::bitmap_indicator_4x8);
          }
          else if (owner_->auto_tune_step() == DAC_VOLT_0_BASELINE || owner_->auto_tune_step() == DAC_VOLT_TARGET_FREQUENCIES) // this goes too quick, so ... 
            graphics.print(" 0.0V baseline");
          else {
            graphics.print(AT_steps[owner_->auto_tune_step() - DAC_VOLT_3m]);
            if (!owner_->_ready())
              graphics.print(" ");
            else 
              graphics.printf(" > %7.3f", owner_->get_auto_frequency());
          }
        }
        break;
        case AT_ERROR:
        graphics.print("run --> ");
        graphics.print("error!");
        break;
        case AT_DONE: 
        graphics.print(OC::Strings::channel_id[channel_]);
        graphics.print("  --> a-ok!");
        calibration_data_ = owner_->data_available();
        break;
        default:
        break;
        }
      }
    }
 
    x = 16; y = 15;
    for (size_t i = 0; i < (AUTO_MENU_ITEMS_LAST - 0x1); ++i, y += 20) {
      
      graphics.drawFrame(x, y, 95, 16);
      // cursor:
      if (i == cursor_pos_) 
        graphics.invertRect(x - 2, y - 2, 99, 20);
    }
  }
  
  template <typename Owner>
  void Autotuner<Owner>::HandleButtonEvent(const UI::Event &event) {
    
     if (UI::EVENT_BUTTON_PRESS == event.type) {
      switch (event.control) {
        case OC::CONTROL_BUTTON_UP:
          handleButtonUp(event);
          break;
        case OC::CONTROL_BUTTON_DOWN:
          handleButtonDown(event);
          break;
        case OC::CONTROL_BUTTON_L:
          handleButtonLeft(event);
          break;    
        case OC::CONTROL_BUTTON_R:
          owner_->reset_autotuner();
          Close();
          break;
        default:
          break;
      }
    }
    else if (UI::EVENT_BUTTON_LONG_PRESS == event.type) {
       switch (event.control) {
        case OC::CONTROL_BUTTON_UP:
          // screensaver 
        break;
        case OC::CONTROL_BUTTON_DOWN:
          OC::DAC::reset_all_auto_channel_calibration_data();
          calibration_data_ = 0x0;
        break;
        case OC::CONTROL_BUTTON_L: 
        break;
        case OC::CONTROL_BUTTON_R:
         // app menu
        break;  
        default:
        break;
      }
    }
  }
  
  template <typename Owner>
  void Autotuner<Owner>::HandleEncoderEvent(const UI::Event &event) {
   
    if (OC::CONTROL_ENCODER_R == event.control) {
      move_cursor(event.value);
    }
    else if (OC::CONTROL_ENCODER_L == event.control) {
      change_value(event.value); 
    }
  }
  
  template <typename Owner>
  void Autotuner<Owner>::move_cursor(int offset) {

    if (auto_tune_running_status_ < AT_RUN) {
      int cursor_pos = cursor_pos_ + offset;
      CONSTRAIN(cursor_pos, 0, AUTO_MENU_ITEMS_LAST - 0x2);  
      cursor_pos_ = cursor_pos;
      //
      if (cursor_pos_ == DATA_SELECT)
          auto_tune_running_status_ = AT_OFF;
    }
    else if (auto_tune_running_status_ == AT_ERROR || auto_tune_running_status_ == AT_DONE)
      auto_tune_running_status_ = AT_OFF;
  }

  template <typename Owner>
  void Autotuner<Owner>::change_value(int offset) {

    switch (cursor_pos_) {
      case DATA_SELECT:
      {
        uint8_t data = owner_->data_available();
        if (!data) { // no data -- 
          calibration_data_ = 0x0;
          data_select_ = 0x0;
        }
        else {
          int _data_sel = data_select_ + offset;
          CONSTRAIN(_data_sel, 0, 0x1);  
          data_select_ = _data_sel;
          if (_data_sel == 0x0) {
            calibration_data_ = 0xFF;
            owner_->use_default();
          }
          else {
            calibration_data_ = 0x01;
            owner_->use_auto_calibration();
          }
        }
      }
      break;
      case AUTOTUNE: 
      {
        if (auto_tune_running_status_ < AT_RUN) {
          int _status = auto_tune_running_status_ + offset;
          CONSTRAIN(_status, 0, AT_READY);
          auto_tune_running_status_ = _status;
          owner_->autotuner_arm(_status);
        }
        else if (auto_tune_running_status_ == AT_ERROR || auto_tune_running_status_ == AT_DONE)
          auto_tune_running_status_ = 0x0;
      }
      break;
      default:
      break;
    }
  }
  
  template <typename Owner>
  void Autotuner<Owner>::handleButtonUp(const UI::Event &event) {

    if (cursor_pos_ == AUTOTUNE && auto_tune_running_status_ == AT_OFF) {
      // arm the tuner
      auto_tune_running_status_ = AT_READY;
      owner_->autotuner_arm(auto_tune_running_status_);
    }
    else {
      owner_->reset_autotuner();
      auto_tune_running_status_ = AT_OFF;
    }
  }
  
  template <typename Owner>
  void Autotuner<Owner>::handleButtonDown(const UI::Event &event) {
    
    if (cursor_pos_ == AUTOTUNE && auto_tune_running_status_ == AT_READY) {
      owner_->autotuner_run();
      auto_tune_running_status_ = AT_RUN;
    }
    else if (auto_tune_running_status_ == AT_ERROR) {
      owner_->reset_autotuner();
      auto_tune_running_status_ = AT_OFF;
    }  
  }
  
  template <typename Owner>
  void Autotuner<Owner>::handleButtonLeft(const UI::Event &) {
  }
  
  template <typename Owner>
  void Autotuner<Owner>::Begin() {
    
    const OC::Autotune_data &autotune_data = OC::AUTOTUNE::GetAutotune_data(channel_);
    calibration_data_ = autotune_data.use_auto_calibration_;
    
    if (calibration_data_ == 0x01) // auto cal. data is in use
      data_select_ = 0x1;
    else
      data_select_ = 0x00;
    cursor_pos_ = 0x0;
    auto_tune_running_status_ = 0x0;
  }
  
  template <typename Owner>
  void Autotuner<Owner>::Close() {
    ui.SetButtonIgnoreMask();
    owner_ = nullptr;
  }
}; // namespace OC

#endif // OC_AUTOTUNER_H
