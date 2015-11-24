/* 
*
* hello etc ... display stuff 
*
*/

extern void TONNETZ_renderMenu();

uint8_t MENU_CURRENT = 2; 
int16_t SCALE_SEL = 0;
uint8_t X_OFF = 104; // display x offset
uint8_t OFFSET = 0x0;  // display y offset
uint8_t UI_MODE = 0;
uint8_t MENU_REDRAW = 0;
const uint16_t TIMEOUT = 15000; // time out menu (in ms)

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
 
  if (millis() - _UI_TIMESTAMP > TIMEOUT) { UI_MODE = SCREENSAVER; MENU_REDRAW = 1; }
  else if (millis() - _CLK_TIMESTAMP > TRIG_LENGTH) {        
         MENU_REDRAW = 1;
         display_clock = false;
  }  
} 

/* -----------   draw menu  --------------- */ 

#define U8G_DRAW(fun) \
do { \
  u8g.firstPage(); \
  do { \
    fun(); \
  } while (u8g.nextPage()); \
} while (0)

void UI() {
  if (  MENU_REDRAW != 0 ) {
    switch(UI_MODE) {
      case SCREENSAVER:
        U8G_DRAW(current_app->screensaver);
        break;
      case MENU:
        U8G_DRAW(current_app->render_menu);
        break;
      case CALIBRATE:
        U8G_DRAW(calibrate);
        break;
      default: break;
    }
    MENU_REDRAW = 0;
  }
}  

/* -----------  splash  ------------------- */  

void hello() {
  u8g.setFont(u8g_font_6x12);
  u8g.firstPage();
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  u8g.firstPage();  
  do {
    u8g.setDefaultForegroundColor();
    u8g.drawBox(0, 0, 128, 14);
    u8g.setDefaultBackgroundColor();
    u8g.setPrintPos(4, 2); u8g.print("ornament&crime+");

    u8g.setDefaultForegroundColor();
    u8g.setPrintPos(4, 20); u8g.print("L : calibrate");
    u8g.setPrintPos(4, 33); u8g.print("R : choose app");
    u8g.setPrintPos(4, 52); u8g.print(__DATE__);
  } while( u8g.nextPage() ); 
}  

/* ----------- screensaver ----------------- */
void screensaver() {
   u8g.setFont(u8g_font_6x12);
   u8g.setColorIndex(1);
   u8g.setFontRefHeightText();
   u8g.setFontPosTop();
  
   uint8_t x, y, width = 10;
   for(int i = 0; i < 4; i++ ) { 
     x = i*37;
     y = asr_outputs[i] >> 10; 
     y++; 
     u8g.drawBox(x, 64-y, width, width); // replace second 'width' with y for bars.
   }
}

/* --------------------- main menu loop ------------------------  */

void ASR_menu() {

      u8g.setFont(u8g_font_6x12);
      u8g.setColorIndex(1);
      u8g.setFontRefHeightText();
      u8g.setFontPosTop();

        uint8_t i, h, w, x, y;
        h = 11; // == OFFSET + u8g.getFontAscent()-u8g.getFontDescent();
        w = 128;
        x = X_OFF;
        y = OFFSET;
        // print parameters 0-1                       
        // scale name: 
        u8g.setPrintPos(0x0, y);    
        if ((SCALE_SEL == asr_params[0]) || (!SCALE_CHANGE)) u8g.print(">");
        else u8g.print('\xb7');
        u8g.drawStr(10, y, abc[SCALE_SEL]);
        // octave +/- 
        if ((asr_params[1]) >= 0) {            
           u8g.setPrintPos(x-6, y); 
           u8g.print("+");
           u8g.setPrintPos(x, y); 
           u8g.print(asr_params[1]);
         }
         else {     
           u8g.setPrintPos(x-6, y); 
           u8g.print(asr_params[1]);
         }     
         
        u8g.drawLine(0, 13, 128, 13);
        
        // draw user menu, params 2-5 : 
        for(i = 2; i < MENU_ITEMS; i++) {   
       
            y = i*h-4;  // = offset 
            u8g.setDefaultForegroundColor();
            
            // print cursor          
            if (i == MENU_CURRENT) {              
                  u8g.drawBox(0, y, w, h);           
                  u8g.setDefaultBackgroundColor();
            }
            // print param name
            u8g.drawStr(10, y, menu_strings[i]);  
            // print param values     
            if (i == 5) {  // map att/mult
                u8g.setPrintPos(x,y); 
                uint8_t x = asr_params[5]-7;
                u8g.print(map_param5[x]);       
            } 
            else {                            
                if (asr_display_params[i] < 0) u8g.setPrintPos(x-6, y);
                else u8g.setPrintPos(x,y);
                u8g.print(asr_display_params[i]);
            }  
      u8g.setDefaultForegroundColor();           
    }
}
