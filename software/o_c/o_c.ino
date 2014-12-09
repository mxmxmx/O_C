

/* 

ornament + crime // 4xCV DAC8565  // "ASR" 

--------------------------------
TR 1 = clock
TR 2 = hold
TR 3 = oct +
TR 4 = oct -

CV 1 = sample in
CV 2 = index CV
CV 3 = # notes (constrain)
CV 4 = octaves/offset

left  encoder = scale select
right encoder = param. select

button 1 (top) =  oct +
button 2       =  oct -
--------------------------------
TD: 
- check 'hold' ?

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

Rotary encoder[2] = {{encL1, encL2}, {encR1, encR2}}; 

/*  menu variables */
volatile uint8_t display_clock;
extern uint8_t MENU_REDRAW;
extern int16_t asr_display_params[6];
extern uint8_t UImode;
extern uint32_t LAST_UI;

/*  ------------------------ ASR ------------------------------------  */

#define MAX_VALUE 65535 // DAC fullscale 
#define MAX_ITEMS 256   // ASR ring buffer size
#define OCTAVES 10      // # octaves
uint16_t octaves[OCTAVES+1] = {0, 6553, 13107, 19661, 26214, 32768, 39321, 45875, 52428, 58981, 65535}; // in practice  
const uint16_t THEORY[OCTAVES+1] = {0, 6553, 13107, 19661, 26214, 32768, 39321, 45875, 52428, 58981, 65535}; // in theory  

typedef struct ASRbuf {
  
    uint8_t     first;
    uint8_t     last;
    uint8_t     items;
    uint16_t data[MAX_ITEMS];

} ASRbuf;

ASRbuf *ASR;

/*  ---------------------  CV   stuff  --------------------------------- */

#define ADC_rate 5000
#define numADC 4
int16_t cvval[numADC];  // store cv values
uint8_t cvmap[numADC] = {CV1, CV1, CV3, CV4}; // map ADC pins
IntervalTimer ADC_timer;
volatile boolean _ADC = false;

void ADC_callback() { _ADC = true; }

/*  --------------------- clk / buttons / ISR -------------------------   */

uint32_t LAST_TRIG = 0;
uint32_t LAST_BUT = 0;
const uint8_t TRIG_LENGTH = 150;
const uint8_t DEBOUNCE = 750;

volatile boolean CLK_STATE1;

void clk_ISR1() {  CLK_STATE1 = true; }  // main clock

enum the_buttons {
  
  BUTTON_TOP,
  BUTTON_BOTTOM,
  BUTTON_LEFT,
  BUTTON_RIGHT

};  

/*       ---------------------------------------------------------         */

void setup(){

  analogReadResolution(12);
  analogReadAveraging(4);
  spi4teensy3::init();
  
  /* pins + ISR */
  pinMode(butL, INPUT);
  pinMode(butR, INPUT);
  pinMode(but_top, INPUT);
  pinMode(but_bot, INPUT);
 
  pinMode(TR1, INPUT); // INPUT_PULLUP);
  pinMode(TR2, INPUT);
  pinMode(TR3, INPUT);
  pinMode(TR4, INPUT);
  
  /* clock ISR */
  attachInterrupt(TR1, clk_ISR1, FALLING);
  /* encoder ISR */
  attachInterrupt(encL1, left_encoder_ISR, CHANGE);
  attachInterrupt(encL2, left_encoder_ISR, CHANGE);
  attachInterrupt(encR1, right_encoder_ISR, CHANGE);
  attachInterrupt(encR2, right_encoder_ISR, CHANGE);
  /* ADC timer */
  ADC_timer.begin(ADC_callback, ADC_rate);
  /* set up DAC pins  */
  pinMode(CS, OUTPUT);
  pinMode(RST,OUTPUT);
  /* pull RST high */
  digitalWrite(RST, HIGH); 
  /* set all outputs to zero */
  delay(10);
  set8565_CHA(0);
  set8565_CHB(0);
  set8565_CHC(0);
  set8565_CHD(0);
  /* calibrate? else use EEPROM; else use things in theory */
  if (!digitalRead(butL))  calibrate_main();
  else if (EEPROM.read(0x2) > 0) readDACcode(); 
  else in_theory();
  /* splash screen, sort of */
  hello();  
  delay(1250);   
  /* initialize ASR */
  init_DACtable();
  ASR = (ASRbuf*)malloc(sizeof(ASRbuf));
  init_ASR(ASR);  
}


/*  ---------    main loop  --------  */

void loop(){

  UI();

  if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); }  
   
  if (millis() - LAST_BUT > DEBOUNCE) update_ENC();
  
  if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); } 
    
  if (_ADC) CV();
  
  if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); } 
  
  buttons(BUTTON_TOP);
  
  if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); } 
  
  buttons(BUTTON_BOTTOM);
  
  if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); } 
  
  buttons(BUTTON_LEFT);
  
  if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); } 
  
  buttons(BUTTON_RIGHT);
  
  if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); }  
   
  if (UImode) timeout(); 
 
  if (CLK_STATE1) {  CLK_STATE1 = false; _ASR(); } 
 
  
}




