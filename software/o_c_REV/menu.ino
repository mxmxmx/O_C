/* 
*
* hello etc ... display stuff 
*
*/

uint8_t MENU_CURRENT = 2; 
int16_t SCALE_SEL = 0;
uint8_t X_OFF = 104; // display x offset
uint8_t OFFSET = 0x0;  // display y offset
uint8_t UI_MODE = 0;
uint8_t MENU_REDRAW = 0;
static const uint16_t SCREENSAVER_TIMEOUT_MS = 15000; // time out menu (in ms)

const int8_t MENU_ITEMS = 6;

/* ASR parameters: scale, octave, offset, delay, #/scale, attenuation */ 
int16_t asr_display_params[] = {0, 0, 0, 0, 0, 0};
/* limits for menu parameters */
const int16_t param_limits[][2] = {{0,0}, {-5,5}, {-999,999}, {0,63}, {2,MAXNOTES}, {7,26}}; 

/*  names for menu items */
const char *menu_strings[MENU_ITEMS] = {

   "", 
   "", 
   "offset", 
   "asr_index", 
   "#/scale", 
   "att/mult" 
};
                  
/* pseudo .x for display */                                 
const char *map_param5[] =    
{
  "0.1", "0.2", "0.3", "0.4", "0.5", "0.6", "0.7", "0.8", "0.9", "1.0",
  "1.1", "1.2", "1.3", "1.4", "1.5", "1.6", "1.7", "1.8", "1.9", "2.0"
};

/* ----------- time out UI ---------------- */ 

void timeout() {
 
  if (millis() - _UI_TIMESTAMP > SCREENSAVER_TIMEOUT_MS) {
    UI_MODE = SCREENSAVER;
    MENU_REDRAW = 1;
  } else if (display_clock && millis() - _CLK_TIMESTAMP > TRIG_LENGTH) {        
     MENU_REDRAW = 1;
     display_clock = false;
  }  
}

/* -----------   draw menu  --------------- */ 

void UI() {
  if (  MENU_REDRAW != 0 ) {
    switch(UI_MODE) {
      case SCREENSAVER:
        current_app->draw_screensaver();
        break;
      case MENU:
        current_app->draw_menu();
        break;
      case CALIBRATE:
        calibrate();
        break;
      default: break;
    }
    // MENU_REDRAW = 0; set in GRAPHICS_END_FRAME if drawn
  }
}  

/* -----------  splash  ------------------- */  

void hello() {
  GRAPHICS_BEGIN_FRAME(true);
  graphics.setFont(u8g_font_6x12);
  graphics.setPrintPos(4, 2); graphics.print("ornaments&crimes");
  graphics.invertRect(0, 0, 128, 14);

  graphics.setDefaultForegroundColor();
  graphics.setPrintPos(4, 20); graphics.print("L : calibrate");
  graphics.setPrintPos(4, 33); graphics.print("R : choose app");
  graphics.setPrintPos(4, 52); graphics.print(__DATE__);
  GRAPHICS_END_FRAME();
}  

/* ----------- screensaver ----------------- */
void screensaver() {
  GRAPHICS_BEGIN_FRAME(false);

  weegfx::coord_t x = 8;
  uint8_t y, width = 8;
  for(int i = 0; i < 4; i++, x += 32 ) { 
    y = DAC::value(i) >> 10; 
    y++; 
    graphics.drawRect(x, 64-y, width, width); // replace second 'width' with y for bars.
  }

  GRAPHICS_END_FRAME();
}

uint16_t scope_history[DAC::kHistoryDepth];

void scope() {
  GRAPHICS_BEGIN_FRAME(false);

  DAC::getHistory<DAC_CHANNEL_A>(scope_history);
  for (weegfx::coord_t x = 0; x < 64; ++x)
    graphics.setPixel(x, 0 + (scope_history[x] >> 11));

  DAC::getHistory<DAC_CHANNEL_B>(scope_history);
  for (weegfx::coord_t x = 0; x < 64; ++x)
    graphics.setPixel(64 + x, 0 + (scope_history[x] >> 11));

  DAC::getHistory<DAC_CHANNEL_C>(scope_history);
  for (weegfx::coord_t x = 0; x < 64; ++x)
    graphics.setPixel(x, 32 + (scope_history[x] >> 11));

  DAC::getHistory<DAC_CHANNEL_D>(scope_history);
  for (weegfx::coord_t x = 0; x < 64; ++x)
    graphics.setPixel(64 + x, 32 + (scope_history[x] >> 11));
/*
  graphics.drawHLine(0, 0 + (DAC::value(0) >> 11), 64);
  graphics.drawHLine(64, 0 + (DAC::value(1) >> 11), 64);
  graphics.drawHLine(0, 32 + (DAC::value(2) >> 11), 64);
  graphics.drawHLine(64, 32 + (DAC::value(3) >> 11), 64);
*/
  GRAPHICS_END_FRAME();
}

/* --------------------- main menu loop ------------------------  */

void ASR_menu() {
  GRAPHICS_BEGIN_FRAME(false);

  graphics.setFont(u8g_font_6x12);

  uint8_t i, h, w, x, y;
  h = 11; // == OFFSET + u8g.getFontAscent()-u8g.getFontDescent();
  w = 128;
  x = X_OFF;
  y = OFFSET;

  // print parameters 0-1                       
  // scale name: 
  graphics.setPrintPos(0x0, y);    
  if ((SCALE_SEL == asr_params[0]) || (!SCALE_CHANGE))
    graphics.print(">");
  else
    graphics.print('-');
  graphics.print(abc[SCALE_SEL]);

  // octave +/- 
  graphics.setPrintPos(x, y); 
  graphics.pretty_print(asr_params[1]);

  graphics.drawHLine(0, 13, 128);
        
  // draw user menu, params 2-5 : 
  for(i = 2; i < MENU_ITEMS; i++) {   

    y = i*h-4;  // = offset 
           
    // print param name
    graphics.drawStr(10, y + 2, menu_strings[i]);  
    // print param values     
    graphics.setPrintPos(x, y + 2);
    if (i == 5) {  // map att/mult
      uint8_t x = asr_params[5]-7;
      graphics.print(map_param5[x]);       
    } 
    else {   
      graphics.print(asr_display_params[i]);
    }  

    // print cursor          
    if (i == MENU_CURRENT) {              
      graphics.invertRect(0, y, w, h);           
    }
  }

  GRAPHICS_END_FRAME();
}
