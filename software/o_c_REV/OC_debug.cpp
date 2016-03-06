#include <Arduino.h>
#include "OC_ADC.h"
#include "OC_config.h"
#include "OC_debug.h"
#include "util_ui.h"
#include "extern/dspinst.h"

extern void POLYLFO_debug();
extern void LORENZ_debug();

namespace OC {

enum DebugMenu {
  DEBUG_MENU_CORE,
  DEBUG_MENU_GFX,
  DEBUG_MENU_ADC,
  DEBUG_MENU_POLYLFO,
  DEBUG_MENU_LORENZ,
  DEBUG_MENU_LAST
};

static void debug_menu_core() {

  uint32_t cycles = OC::CORE::ISR_cycles.value();
  uint32_t isr_us = multiply_u32xu32_rshift32(cycles, (1ULL << 32) / (F_CPU / 1000000));

  graphics.setPrintPos(2, 22);
  graphics.printf("%uMHz, ISR %uus", F_CPU / 1000 / 1000, OC_CORE_TIMER_RATE);
  graphics.setPrintPos(2, 32);
  graphics.printf("ISR us: %u", isr_us);
  graphics.setPrintPos(2, 42);
  graphics.printf("ISR %% : %u", (isr_us * 100) /  OC_CORE_TIMER_RATE);
}

static void debug_menu_gfx() {
  graphics.drawFrame(0, 0, 128, 64);

  graphics.setPrintPos(0, 12);
  graphics.print("W");
}

static void debug_menu_adc() {
    graphics.setPrintPos(2, 12);
    graphics.print("CV1: "); graphics.pretty_print(OC::ADC::value<ADC_CHANNEL_1>(), 6);

    graphics.setPrintPos(2, 22);
    graphics.print("CV2: "); graphics.pretty_print(OC::ADC::value<ADC_CHANNEL_2>(), 6);

    graphics.setPrintPos(2, 32);
    graphics.print("CV3: "); graphics.pretty_print(OC::ADC::value<ADC_CHANNEL_3>(), 6);

    graphics.setPrintPos(2, 42);
    graphics.print("CV4: "); graphics.pretty_print(OC::ADC::value<ADC_CHANNEL_4>(), 6);

//      graphics.setPrintPos(2, 42);
//      graphics.print((long)OC::ADC::busy_waits());
//      graphics.setPrintPos(2, 42); graphics.print(OC::ADC::fail_flag0());
//      graphics.setPrintPos(2, 52); graphics.print(OC::ADC::fail_flag1());
}

struct {
  DebugMenu menu;
  const char *title;
  void (*display_fn)();
}
const debug_menus[DEBUG_MENU_LAST] = {
  { DEBUG_MENU_CORE, " CORE", debug_menu_core },
  { DEBUG_MENU_GFX, " GFX", debug_menu_gfx },
  { DEBUG_MENU_ADC, " ADC", debug_menu_adc },
  { DEBUG_MENU_POLYLFO, " POLYLFO", POLYLFO_debug },
  { DEBUG_MENU_LORENZ, " LORENZ", LORENZ_debug },
};


void debug_menu() {

  DebugMenu current_menu = DEBUG_MENU_CORE;
  while (true) {

    GRAPHICS_BEGIN_FRAME(false);
      graphics.setPrintPos(2, 2);
      graphics.print((int)current_menu + 1); graphics.print("/"); graphics.print((int)DEBUG_MENU_LAST);
      graphics.print(debug_menus[current_menu].title);
      debug_menus[current_menu].display_fn();
    GRAPHICS_END_FRAME();

    button_right.read();
    if (button_right.event())
      break;

    button_left.read();
    if (button_left.event()) {
      if (current_menu < DEBUG_MENU_LAST - 1)
        current_menu = static_cast<DebugMenu>(current_menu + 1);
      else
        current_menu = DEBUG_MENU_CORE;
    }
  }
}

};
