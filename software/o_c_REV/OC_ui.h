#ifndef OC_UI_H_
#define OC_UI_H_

#include "weegfx.h"

namespace OC {

void hello();
void debug_menu();

}; // namespace OC

namespace UI {

enum DisplayMode {  
 DISPLAY_SCREENSAVER,
 DISPLAY_MENU
}; 

}; // namespace UI

extern uint_fast8_t UI_MODE;
extern uint_fast8_t MENU_REDRAW;

void init_circle_lut();
void visualize_pitch_classes(uint8_t *normalized, weegfx::coord_t centerx, weegfx::coord_t centery);

extern weegfx::Graphics graphics;

#endif // OC_UI_H_
