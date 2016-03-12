/*
* ornament + crime // 4xCV DAC8565  // "ASR" 
*
* --------------------------------
* TR 1 = clock
* TR 2 = hold
* TR 3 = oct +
* TR 4 = oct -
*
* CV 1 = sample in
* CV 2 = index CV
* CV 3 = # notes (constrain)
* CV 4 = octaves/offset
*
* left  encoder = scale select
* right encoder = param. select
* 
* button 1 (top) =  oct +
* button 2       =  oct -
* --------------------------------
*
*/

#include <ADC.h>
#include <spi4teensy3.h>
#include <rotaryplus.h>
#include <EEPROM.h>

#include "OC_apps.h"
#include "OC_core.h"
#include "OC_debug.h"
#include "OC_gpio.h"
#include "OC_ADC.h"
#include "OC_calibration.h"
#include "OC_digital_inputs.h"
#include "OC_ui.h"
#include "DAC.h"
#include "util/EEPROMStorage.h"
#include "util/util_button.h"
#include "util/util_pagestorage.h"
#include "util_framebuffer.h"
#include "util_ui.h"
#include "page_display_driver.h"
#include "weegfx.h"
#include "SH1106_128x64_driver.h"

//#define ENABLE_DEBUG_PINS
#include "util/util_debugpins.h"

FrameBuffer<SH1106_128x64_Driver::kFrameSize, 2> frame_buffer;
PagedDisplayDriver<SH1106_128x64_Driver> display_driver;
weegfx::Graphics graphics;

unsigned long LAST_REDRAW_TIME = 0;
bool SELECT_APP = false;
uint_fast8_t UI_MODE = UI::DISPLAY_MENU;
uint_fast8_t MENU_REDRAW = true;

Rotary encoder[2] =
{
  Rotary(encL1, encL2),
  Rotary(encR1, encR2)
}; 

/*  --------------------- clk / buttons / ISR -------------------------   */

uint32_t _CLK_TIMESTAMP = 0;
uint32_t _BUTTONS_TIMESTAMP = 0;
static const uint16_t TRIG_LENGTH = 150;
static const uint16_t DEBOUNCE = 250;

uint32_t _UI_TIMESTAMP;

enum the_buttons 
{  
  BUTTON_TOP,
  BUTTON_BOTTOM,
  BUTTON_LEFT,
  BUTTON_RIGHT
};

enum encoders
{
  LEFT,
  RIGHT
};

volatile uint_fast8_t _ENC = false;
static const uint32_t _ENC_RATE = 15000;

/* encoders isr */
void FASTRUN left_encoder_ISR() 
{
  encoder[LEFT].process();
}

void FASTRUN right_encoder_ISR() 
{
  encoder[RIGHT].process();
}

/*  ------------------------ core timer ISR ---------------------------   */
IntervalTimer CORE_timer;
uint32_t ENC_timer_counter = 0;

volatile bool OC::CORE::app_isr_enabled = false;
volatile uint32_t OC::CORE::ticks = 0;
SmoothedValue<uint32_t, 16> OC::CORE::ISR_cycles;

void FASTRUN CORE_timer_ISR() {
  DEBUG_PIN_SCOPE(DEBUG_PIN_2);
  debug::CycleMeasurement cycles;

  if (display_driver.Flush())
    frame_buffer.read();

  DAC::Update();

  if (display_driver.frame_valid()) {
    display_driver.Update();
  } else {
    if (frame_buffer.readable())
      display_driver.Begin(frame_buffer.readable_frame());
  }

  // The ADC scan uses async startSingleRead/readSingle and single channel each
  // loop, so should be fast enough even at 60us (check ADC::busy_waits() == 0)
  // to verify. Effectively, the scan rate is ISR / 4 / ADC::kAdcSmoothing
  // 100us: 10kHz / 4 / 4 ~ .6kHz
  // 60us: 16.666K / 4 / 4 ~ 1kHz
  // kAdcSmoothing == 4 has some (maybe 1-2LSB) jitter but seems "Good Enough".
  OC::ADC::Scan();

  if (ENC_timer_counter < _ENC_RATE / OC_CORE_TIMER_RATE - 1) {
    ++ENC_timer_counter;
  } else {
    ENC_timer_counter = 0;
    _ENC = true;
  }
  ++OC::CORE::ticks;

  if (OC::CORE::app_isr_enabled)
    OC::APPS::ISR();

  OC::CORE::ISR_cycles.push(cycles.read());
}

/*       ---------------------------------------------------------         */

void setup() {
  
  NVIC_SET_PRIORITY(IRQ_PORTB, 0); // TR1 = 0 = PTB16
  spi4teensy3::init();
  delay(10);

  // pins 
  pinMode(butL, INPUT);
  pinMode(butR, INPUT);
  pinMode(but_top, INPUT);
  pinMode(but_bot, INPUT);
  buttons_init();

  DebugPins::Init();
  OC::DigitalInputs::Init();

  // encoder ISR 
  attachInterrupt(encL1, left_encoder_ISR, CHANGE);
  attachInterrupt(encL2, left_encoder_ISR, CHANGE);
  attachInterrupt(encR1, right_encoder_ISR, CHANGE);
  attachInterrupt(encR2, right_encoder_ISR, CHANGE);

  Serial.begin(9600);
  delay(500);

  // Ok, technically we're using the calibration_data before it's loaded...
  OC::ADC::Init(&OC::calibration_data.adc);
  DAC::Init(&OC::calibration_data.dac);

  frame_buffer.Init();
  display_driver.Init();
  graphics.Init();
 
  GRAPHICS_BEGIN_FRAME(true);
  GRAPHICS_END_FRAME();

  CORE_timer.begin(CORE_timer_ISR, OC_CORE_TIMER_RATE);

  calibration_load();
  SH1106_128x64_Driver::AdjustOffset(OC::calibration_data.display_offset);

  // Display splash screen
  OC::hello();
  bool use_defaults = !digitalRead(but_top) && !digitalRead(but_bot);
  if (button_left.read_immediate())
    calibration_menu();
  else if (button_right.read_immediate())
    SELECT_APP = true;

  // initialize 
  OC::APPS::Init(use_defaults);
  OC::CORE::app_isr_enabled = true;
}

/*  ---------    main loop  --------  */

void FASTRUN loop() {
  _UI_TIMESTAMP = millis();
  UI_MODE = UI::DISPLAY_MENU;

  while (1) {
    // don't change current_app while it's running
    if (SELECT_APP) {
      OC::APPS::Select();
      SELECT_APP = false;
    }

    // Refresh display
    if (MENU_REDRAW) {
      if (UI::DISPLAY_MENU == UI_MODE)
        OC::current_app->draw_menu();
      else
        OC::current_app->draw_screensaver();
      // MENU_REDRAW reset in GRAPHICS_END_FRAME if drawn
    }

    // Run current app
    OC::current_app->loop();

    // Check UI timeouts for screensaver/forced redraw
    unsigned long now = millis();
    if (UI::DISPLAY_MENU == UI_MODE) {
      if (now - _UI_TIMESTAMP > SCREENSAVER_TIMEOUT_MS) {
        UI_MODE = UI::DISPLAY_SCREENSAVER;
        MENU_REDRAW = 1;
        OC::current_app->handleEvent(OC::APP_EVENT_SCREENSAVER);
      }
    }

    if (now - LAST_REDRAW_TIME > REDRAW_TIMEOUT_MS)
      MENU_REDRAW = 1;
  }
}
