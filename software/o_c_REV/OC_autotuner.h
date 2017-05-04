#ifndef OC_AUTOTUNER_H
#define OC_AUTOTUNER_H

namespace OC {

template <typename Owner>
class Autotuner {

public:

  void Init() {
    owner_ = nullptr;
    cursor_pos_ = 0;
    channel_ = 0;
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

  void Begin();
  void move_cursor(int offset);
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
   
    if (OC::CONTROL_ENCODER_L == event.control) {
      move_cursor(event.value);
    } 
  }
  
  template <typename Owner>
  void Autotuner<Owner>::move_cursor(int offset) {
  }
  
  template <typename Owner>
  void Autotuner<Owner>::handleButtonUp(const UI::Event &event) {
  }
  
  template <typename Owner>
  void Autotuner<Owner>::handleButtonDown(const UI::Event &event) {
  }
  
  template <typename Owner>
  void Autotuner<Owner>::handleButtonLeft(const UI::Event &) {
  }
  
  template <typename Owner>
  void Autotuner<Owner>::Begin() {
    cursor_pos_ = 0x0;
  }
  
  template <typename Owner>
  void Autotuner<Owner>::Close() {
    ui.SetButtonIgnoreMask();
    owner_ = nullptr;
  }
}; // namespace OC

#endif // OC_AUTOTUNER_H
