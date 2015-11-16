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

#include <spi4teensy3.h>
#include <u8g_teensy.h>
#include <rotaryplus.h>
#include <EEPROM.h>

#define CS 10  // DAC CS 
#define RST 9  // DAC RST

#define CV1 19
#define CV2 18
#define CV3 20
#define CV4 17

#define TR1 0
#define TR2 1
#define TR3 2
#define TR4 3

#define encR1 15
#define encR2 16
#define butR  14

#define encL1 22
#define encL2 21
#define butL  23

#define but_top 5
#define but_bot 4

U8GLIB u8g(&u8g_dev_sh1106_128x64_2x_hw_spi, u8g_com_hw_spi_fn);

Rotary encoder[2] =
{
  {encL1, encL2}, 
  {encR1, encR2}
}; 

//  UI mode select
extern uint8_t UI_MODE;

/*  ------------------------ ASR ------------------------------------  */

#define MAX_VALUE 65535 // DAC fullscale 
#define MAX_ITEMS 256   // ASR ring buffer size
#define OCTAVES 10      // # octaves
uint16_t octaves[OCTAVES+1] = {0, 6553, 13107, 19661, 26214, 32768, 39321, 45875, 52428, 58981, 65535}; // in practice  
const uint16_t THEORY[OCTAVES+1] = {0, 6553, 13107, 19661, 26214, 32768, 39321, 45875, 52428, 58981, 65535}; // in theory  
extern const uint16_t _ZERO;

/*  ---------------------  CV   stuff  --------------------------------- */

#define _ADC_RATE 1000
#define _ADC_RES  12
#define numADC 4
int16_t cvval[numADC];                        // store cv values

// PIT timer : 
IntervalTimer ADC_timer;
volatile uint16_t _ADC = false;

void ADC_callback()
{ 
  _ADC = true; 
}

/*  --------------------- clk / buttons / ISR -------------------------   */

uint32_t _CLK_TIMESTAMP = 0;
uint32_t _BUTTONS_TIMESTAMP = 0;
const uint16_t TRIG_LENGTH = 150;
const uint16_t DEBOUNCE = 250;

volatile int CLK_STATE[4] = {0,0,0,0};
#define CLK_STATE1 (CLK_STATE[TR1])

void FASTRUN tr1_ISR() {  
    CLK_STATE[TR1] = true; 
}  // main clock

void FASTRUN tr2_ISR() {
  CLK_STATE[TR2] = true;
}

void FASTRUN tr3_ISR() {
  CLK_STATE[TR3] = true;
}

void FASTRUN tr4_ISR() {
  CLK_STATE[TR4] = true;
}

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

volatile boolean _ENC = false;
const uint16_t _ENC_RATE = 15000;

IntervalTimer ENC_timer;
void ENC_callback() 
{ 
  _ENC = true; 
} // encoder update 

struct App {
  const char *name;
  void (*init)();
  void (*loop)();
  void (*render_menu)();
  void (*top_button)();
  void (*lower_button)();
  void (*right_button)();
  void (*left_button)();
};

App available_apps[] = {
  {"ASR", ASR_init, _loop, ASR_menu, topButton, lowerButton, rightButton, leftButton},
  {"Harrington1200", H1200_init, H1200_loop, H1200_menu, H1200_topButton, H1200_lowerButton, H1200_rightButton, H1200_leftButton}
};
static const size_t APP_COUNT = sizeof(available_apps) / sizeof(available_apps[0]);

size_t current_app_index = 1;
volatile App *current_app = &available_apps[current_app_index];

void next_app() {
  set8565_CHA(_ZERO);
  set8565_CHB(_ZERO);
  set8565_CHC(_ZERO);
  set8565_CHD(_ZERO);

  current_app_index = (current_app_index + 1) % APP_COUNT;
  current_app = &available_apps[current_app_index];
  hello(current_app->name);
  delay(1250);
}

/*       ---------------------------------------------------------         */

void setup(){
  
  NVIC_SET_PRIORITY(IRQ_PORTB, 0); // TR1 = 0 = PTB16
  analogReadResolution(_ADC_RES);
  analogReadAveraging(0x10);
  spi4teensy3::init();
  delay(10);
  // pins 
  pinMode(butL, INPUT);
  pinMode(butR, INPUT);
  pinMode(but_top, INPUT);
  pinMode(but_bot, INPUT);
 
  pinMode(TR1, INPUT); // INPUT_PULLUP);
  pinMode(TR2, INPUT);
  pinMode(TR3, INPUT);
  pinMode(TR4, INPUT);
  
  // clock ISR 
  attachInterrupt(TR1, tr1_ISR, FALLING);
  attachInterrupt(TR2, tr2_ISR, FALLING);
  attachInterrupt(TR3, tr3_ISR, FALLING);
  attachInterrupt(TR4, tr4_ISR, FALLING);
  // encoder ISR 
  attachInterrupt(encL1, left_encoder_ISR, CHANGE);
  attachInterrupt(encL2, left_encoder_ISR, CHANGE);
  attachInterrupt(encR1, right_encoder_ISR, CHANGE);
  attachInterrupt(encR2, right_encoder_ISR, CHANGE);
  // ADC timer
  ADC_timer.begin(ADC_callback, _ADC_RATE);
  ENC_timer.begin(ENC_callback, _ENC_RATE);
  // set up DAC pins 
  pinMode(CS, OUTPUT);
  pinMode(RST,OUTPUT);
  // pull RST high 
  digitalWrite(RST, HIGH); 
  // set all outputs to zero 
  set8565_CHA(_ZERO);
  set8565_CHB(_ZERO);
  set8565_CHC(_ZERO);
  set8565_CHD(_ZERO);
  // splash screen, sort of ... 
  hello("O&C");
  // calibrate? else use EEPROM; else use things in theory :
/*
  if (!digitalRead(butL))  calibrate_main();
  else if (EEPROM.read(0x2) > 0) read_settings(); 
  else in_theory(); // uncalibrated DAC code 
*/
  in_theory();
  delay(1250);   
  // initialize ASR 
  init_DACtable();

  for (size_t i = 0; i < APP_COUNT; ++i)
    available_apps[i].init();
}


/*  ---------    main loop  --------  */

//uint32_t testclock;

void loop(){

  while (1) {
    volatile App *app = current_app;
    while (current_app == app)
      app->loop();
  }
}




