#include <Arduino.h>
#include <algorithm>
#include "OC_digital_inputs.h"
#include "OC_gpio.h"

/*static*/
volatile uint32_t OC::DigitalInputs::clocked_[DIGITAL_INPUT_LAST];

void FASTRUN tr1_ISR() {  
  OC::DigitalInputs::clock<OC::DIGITAL_INPUT_1>();
}  // main clock

void FASTRUN tr2_ISR() {
  OC::DigitalInputs::clock<OC::DIGITAL_INPUT_2>();
}

void FASTRUN tr3_ISR() {
  OC::DigitalInputs::clock<OC::DIGITAL_INPUT_3>();
}

void FASTRUN tr4_ISR() {
  OC::DigitalInputs::clock<OC::DIGITAL_INPUT_4>();
}

/*static*/
void OC::DigitalInputs::Init() {

  static const struct {
    uint8_t pin;
    void (*isr_fn)();
  } pins[DIGITAL_INPUT_LAST] =  {
    {TR1, tr1_ISR},
    {TR2, tr2_ISR},
    {TR3, tr3_ISR},
    {TR4, tr4_ISR},
  };

  for (auto pin : pins) {
    pinMode(pin.pin, OC_GPIO_TRx_PINMODE);
    attachInterrupt(pin.pin, pin.isr_fn, FALLING);
  }

  std::fill(clocked_, clocked_ + DIGITAL_INPUT_LAST, 0);
}
