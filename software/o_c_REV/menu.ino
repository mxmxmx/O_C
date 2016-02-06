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
      default: break;
    }
    // MENU_REDRAW = 0; set in GRAPHICS_END_FRAME if drawn
  }
}  

/* -----------  splash  ------------------- */  

void hello() {
  GRAPHICS_BEGIN_FRAME(true);
  //graphics.setFont(u8g_font_6x12);
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

static const size_t kScopeDepth = 64;

uint16_t scope_history[DAC::kHistoryDepth];
uint16_t averaged_scope_history[DAC_CHANNEL_LAST][kScopeDepth];
size_t averaged_scope_tail = 0;
DAC_CHANNEL scope_update_channel = DAC_CHANNEL_A;

template <size_t size>
inline uint16_t calc_average(const uint16_t *data) {
  uint32_t sum = 0;
  size_t n = size;
  while (n--)
    sum += *data++;
  return sum / size;
}

void scope() {
  GRAPHICS_BEGIN_FRAME(false);
  scope_render();
  GRAPHICS_END_FRAME();
}

void scope_render() {
  switch (scope_update_channel) {
    case DAC_CHANNEL_A:
      DAC::getHistory<DAC_CHANNEL_A>(scope_history);
      averaged_scope_history[DAC_CHANNEL_A][averaged_scope_tail] = ((65535U - calc_average<DAC::kHistoryDepth>(scope_history)) >> 11) & 0x1f;
      scope_update_channel = DAC_CHANNEL_B;
      break;
    case DAC_CHANNEL_B:
      DAC::getHistory<DAC_CHANNEL_B>(scope_history);
      averaged_scope_history[DAC_CHANNEL_B][averaged_scope_tail] = ((65535U - calc_average<DAC::kHistoryDepth>(scope_history)) >> 11) & 0x1f;
      scope_update_channel = DAC_CHANNEL_C;
      break;
    case DAC_CHANNEL_C:
      DAC::getHistory<DAC_CHANNEL_C>(scope_history);
      averaged_scope_history[DAC_CHANNEL_C][averaged_scope_tail] = ((65535U - calc_average<DAC::kHistoryDepth>(scope_history)) >> 11) & 0x1f;
      scope_update_channel = DAC_CHANNEL_D;
      break;
    case DAC_CHANNEL_D:
      DAC::getHistory<DAC_CHANNEL_D>(scope_history);
      averaged_scope_history[DAC_CHANNEL_D][averaged_scope_tail] = ((65535U - calc_average<DAC::kHistoryDepth>(scope_history)) >> 11) & 0x1f;
      scope_update_channel = DAC_CHANNEL_A;
      averaged_scope_tail = (averaged_scope_tail + 1) % kScopeDepth;
      break;
    default: break;
  }

  for (weegfx::coord_t x = 0; x < (weegfx::coord_t)kScopeDepth - 1; ++x) {
    size_t index = (x + averaged_scope_tail + 1) % kScopeDepth;
    graphics.setPixel(x, 0 + averaged_scope_history[DAC_CHANNEL_A][index]);
    graphics.setPixel(64 + x, 0 + averaged_scope_history[DAC_CHANNEL_B][index]);
    graphics.setPixel(x, 32 + averaged_scope_history[DAC_CHANNEL_C][index]);
    graphics.setPixel(64 + x, 32 + averaged_scope_history[DAC_CHANNEL_D][index]);
  }
}

/* --------------------- main menu loop ------------------------  */

void ASR_menu() {
  GRAPHICS_BEGIN_FRAME(false);
  //graphics.setFont(u8g_font_6x12);

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
