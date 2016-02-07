#include <Arduino.h>
#include "OC_ADC.h"
#include "util_ui.h"

extern void POLYLFO_debug();

namespace OC {

enum DebugMenu {
  DEBUG_MENU_ADC,
  DEBUG_MENU_POLYLFO,
  DEBUG_MENU_LAST
};

static void debug_display_adc() {

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
  { DEBUG_MENU_ADC, " ADC", debug_display_adc },
  { DEBUG_MENU_POLYLFO, " POLYLFO", POLYLFO_debug }
};


void debug_menu() {

  DebugMenu current_menu = DEBUG_MENU_ADC;
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
        current_menu = DEBUG_MENU_ADC;
    }
  }
}

};
