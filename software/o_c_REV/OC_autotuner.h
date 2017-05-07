#ifndef OC_AUTOTUNER_H
#define OC_AUTOTUNER_H

#include "OC_autotune.h"


const char* const AT_steps[] = {
  " 0V", " 0V", "-3V", "-2V", "-1V", " 0V", "+1V", "+2V", "+3V", "+4V", "+5V", "+6V", "-" 
};

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

template <typename Owner>
class Autotuner {

public:

  void Init() {
    owner_ = nullptr;
    cursor_pos_ = 0;
    channel_ = 0;
    calibration_data_ = 0;
    auto_calibration_available_ = 0;
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
  int8_t channel_;
  uint8_t calibration_data_;
  uint8_t auto_tune_running_status_;
  bool auto_calibration_available_;

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
          case 0xFF:
          graphics.print("auto");
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
        float freq = owner_->get_auto_frequency();
        if (freq == 0)
          graphics.print("...wait");
        else
          graphics.printf("%7.3f", freq);
        }
        break;
        case AT_RUN:
        if (owner_->auto_tune_step() == 0x1) {
          graphics.print(" ");
          graphics.print(OC::Strings::channel_id[channel_]);
          graphics.print(" --> start");
        }
        else {
          graphics.print(AT_steps[owner_->auto_tune_step()]);
          if (!owner_->_ready())
            graphics.print(" > ...");
          else 
            graphics.printf(" > %7.3f", owner_->get_auto_frequency());
        }
        break;
        case AT_ERROR:
        graphics.print("run --> ");
        graphics.print("error!");
        break;
        case AT_DONE:
        graphics.print("ok! --> ");
        graphics.print(OC::Strings::channel_id[channel_]);
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
          // to do: this also should reset the use_auto_calibration_ field
          OC::DAC::reset_all_auto_channel_calibration_data();
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
  }

  template <typename Owner>
  void Autotuner<Owner>::change_value(int offset) {

    switch (cursor_pos_) {
      case DATA_SELECT:
      // todo
      break;
      case AUTOTUNE: 
      {
        if (auto_tune_running_status_ < AT_RUN) {
          int _status = auto_tune_running_status_ + offset;
          CONSTRAIN(_status, 0, AT_READY);
          auto_tune_running_status_ = _status;
          owner_->autotuner_arm(_status);
        }
        else if (auto_tune_running_status_ == AT_ERROR || auto_tune_running_status_ == AT_DONE) {
          auto_tune_running_status_ = 0x0;
          owner_->autotuner_arm(auto_tune_running_status_);
        }
      }
      break;
      default:
      break;
    }
  }
  
  template <typename Owner>
  void Autotuner<Owner>::handleButtonUp(const UI::Event &event) {
  }
  
  template <typename Owner>
  void Autotuner<Owner>::handleButtonDown(const UI::Event &event) {
    
    if (cursor_pos_ == AUTOTUNE && auto_tune_running_status_ == AT_READY) {
      owner_->autotuner_run();
      auto_tune_running_status_ = AT_RUN;
    }
  }
  
  template <typename Owner>
  void Autotuner<Owner>::handleButtonLeft(const UI::Event &) {
  }
  
  template <typename Owner>
  void Autotuner<Owner>::Begin() {
    
    const OC::Autotune_data &autotune_data = OC::AUTOTUNE::GetAutotune_data(channel_);
    calibration_data_ = autotune_data.use_auto_calibration_;
    if (calibration_data_ > 0x0)
      auto_calibration_available_ = true;
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
