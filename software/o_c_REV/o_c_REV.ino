// Copyright (c) 2015, 2016 Max Stadler, Patrick Dowling
//
// Original Author : Max Stadler
// Heavily modified: Patrick Dowling (pld@gurkenkiste.com)
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

// Main startup/loop for O&C firmware

#include <ADC.h>
#include <spi4teensy3.h>
#include <EEPROM.h>

#include "OC_apps.h"
#include "OC_core.h"
#include "OC_debug.h"
#include "OC_gpio.h"
#include "OC_ADC.h"
#include "OC_calibration.h"
#include "OC_digital_inputs.h"
#include "OC_menus.h"
#include "OC_ui.h"
#include "DAC.h"
#include "util_framebuffer.h"
#include "page_display_driver.h"
#include "weegfx.h"
#include "SH1106_128x64_driver.h"
#include "util/util_debugpins.h"

FrameBuffer<SH1106_128x64_Driver::kFrameSize, 2> frame_buffer;
PagedDisplayDriver<SH1106_128x64_Driver> display_driver;
weegfx::Graphics graphics;

unsigned long LAST_REDRAW_TIME = 0;
uint_fast8_t MENU_REDRAW = true;
OC::UiMode ui_mode = OC::UI_MODE_MENU;

/*  --------------------- UI timer ISR -------------------------   */

IntervalTimer UI_timer;

void FASTRUN UI_timer_ISR() {
  debug::CycleMeasurement cycles;
  OC::ui.Poll();
  OC::DEBUG::UI_cycles.push(cycles.read());
}

/*  ------------------------ core timer ISR ---------------------------   */
IntervalTimer CORE_timer;
volatile bool OC::CORE::app_isr_enabled = false;
volatile uint32_t OC::CORE::ticks = 0;

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

#ifndef OC_UI_SEPARATE_ISR
  TODO needs a counter
  UI_timer_ISR();
#endif

  ++OC::CORE::ticks;
  if (OC::CORE::app_isr_enabled)
    OC::APPS::ISR();

  OC::DEBUG::ISR_cycles.push(cycles.read());
}

/*       ---------------------------------------------------------         */

void setup() {
  
  NVIC_SET_PRIORITY(IRQ_PORTB, 0); // TR1 = 0 = PTB16
  spi4teensy3::init();
  delay(10);

  Serial.begin(9600);
  delay(500);
  SERIAL_PRINTLN("* O&C BOOTING...");

  OC::DEBUG::Init();
  OC::DigitalInputs::Init();
  OC::ADC::Init(&OC::calibration_data.adc); // Yes, it's using the calibration_data before it's loaded...
  DAC::Init(&OC::calibration_data.dac);

  frame_buffer.Init();
  display_driver.Init();
  graphics.Init();
 
  GRAPHICS_BEGIN_FRAME(true);
  GRAPHICS_END_FRAME();

  calibration_load();
  SH1106_128x64_Driver::AdjustOffset(OC::calibration_data.display_offset);

  OC::ui.Init();

  SERIAL_PRINTLN("* Starting CORE ISR @%luus", OC_CORE_TIMER_RATE);
  CORE_timer.begin(CORE_timer_ISR, OC_CORE_TIMER_RATE);
  CORE_timer.priority(OC_CORE_TIMER_PRIO);

#ifdef OC_UI_SEPARATE_ISR
  SERIAL_PRINTLN("* Starting UI ISR @%luus", OC_UI_TIMER_RATE);
  UI_timer.begin(UI_timer_ISR, OC_UI_TIMER_RATE);
  UI_timer.priority(OC_UI_TIMER_PRIO);
#endif

  // Display splash screen
  bool use_defaults = false;
  ui_mode = OC::ui.Splashscreen(use_defaults);

  // initialize apps
  OC::APPS::Init(use_defaults);
  OC::CORE::app_isr_enabled = true;
}

/*  ---------    main loop  --------  */

void FASTRUN loop() {

  if (ui_mode == OC::UI_MODE_CALIBRATE)
    OC::ui.Calibrate();

  while (true) {

    // don't change current_app while it's running
    if (OC::UI_MODE_SELECT_APP == ui_mode) {
      OC::ui.SelectApp();
      ui_mode = OC::UI_MODE_MENU;
    }

    // Refresh display
    if (MENU_REDRAW) {
      GRAPHICS_BEGIN_FRAME(false); // Don't busy wait
        if (OC::UI_MODE_MENU == ui_mode) {
          debug::CycleMeasurement cycles;
          OC::current_app->DrawMenu();
          OC::DEBUG::MENU_draw_cycles.push(cycles.read());
        } else {
          OC::current_app->DrawScreensaver();
        }
        MENU_REDRAW = 0;
        LAST_REDRAW_TIME = millis();
      GRAPHICS_END_FRAME();
    }

    // Run current app
    OC::current_app->loop();

    // UI events
    OC::UiMode mode = OC::ui.DispatchEvents(OC::current_app);

    // State transition for app
    if (mode != ui_mode) {
      if (OC::UI_MODE_SCREENSAVER == mode)
        OC::current_app->HandleAppEvent(OC::APP_EVENT_SCREENSAVER_ON);
      else
        OC::current_app->HandleAppEvent(OC::APP_EVENT_SCREENSAVER_OFF);
      ui_mode = mode;
    }

    if (millis() - LAST_REDRAW_TIME > REDRAW_TIMEOUT_MS)
      MENU_REDRAW = 1;
  }
}
