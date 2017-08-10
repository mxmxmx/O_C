#include <Arduino.h>
#include <algorithm>
#include "OC_digital_inputs.h"
#include "OC_gpio.h"
#include "OC_options.h"

/*static*/
uint32_t OC::DigitalInputs::clocked_mask_;

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

  clocked_mask_ = 0;
  std::fill(clocked_, clocked_ + DIGITAL_INPUT_LAST, 0);

  // Assume the priority of pin change interrupts is lower or equal to the
  // thread where ::Scan function is called. Otherwise a safer mechanism is
  // required to avoid conflicts (LDREX/STREX or store ARM_DWT_CYCCNT in the
  // array to check for changes.
  //
  // A really nice approach would be to use the FTM timer mechanism and avoid
  // the ISR altogether, but this only works for one of the pins. Using more
  // explicit interrupt grouping might also improve the handling a bit (if
  // implemented on the DX)
  //
  // It's still not guaranteed that 4 simultaneous triggers will be handled
  // exactly simultaneously though, but that's micro-timing dependent even if
  // the pins have higher prio.

  //  NVIC_SET_PRIORITY(IRQ_PORTB, 0); // TR1 = 0 = PTB16
  // Defaults is 0, or set OC_GPIO_ISR_PRIO for all ports
}

void OC::DigitalInputs::reInit() {
  // re-init TR4, to avoid conflict with the FTM
  #ifdef FLIP_180
    pinMode(TR1, OC_GPIO_TRx_PINMODE);
    attachInterrupt(TR1, tr1_ISR, FALLING);
  #else
    pinMode(TR4, OC_GPIO_TRx_PINMODE);
    attachInterrupt(TR4, tr4_ISR, FALLING);
  #endif
}

/*static*/
void OC::DigitalInputs::Scan() {
  clocked_mask_ =
    ScanInput<DIGITAL_INPUT_1>() |
    ScanInput<DIGITAL_INPUT_2>() |
    ScanInput<DIGITAL_INPUT_3>() |
    ScanInput<DIGITAL_INPUT_4>();
}
