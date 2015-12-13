/*
*
* ASR / ring buffer / quantization
*
*/

/*  ASR+quantization params */

#define MAXNOTES 7                    // max. notes per scale (see below)
#define MAX_DELAY 63                  // max. delay < > ring buffer is 256
#define RANGE 119                     // [0-119] = 120 semitones < > 10 octaves 
#define S_RANGE 119<<5                // same thing, spread over 12 bit (ish)

extern int16_t asr_display_params[6]; // ASR params
extern uint8_t MENU_REDRAW;           // 
volatile uint16_t display_clock;      // 
int16_t  asr_params[6];               // ASR params :: scale, octave, offset, delay, nps, attenuation
uint16_t asr_outputs[4];              // ASR out
uint32_t CLK_COUNT  = 0;              // count clocks
uint8_t  THIS_SCALE = 0;              // track scale change

uint16_t semitones[RANGE+1];          // DAC output LUT

typedef struct ASRbuf
{
    uint8_t     first;
    uint8_t     last;
    uint8_t     items;
    uint16_t data[MAX_ITEMS];

} ASRbuf;

ASRbuf *ASR;
                                
/* update ASR params + etc */

void FASTRUN _ASR() {

         /*  update asr_params < > scale, octave, offset, delay, nps, attenuation */
        uint8_t _scale     =  asr_params[0];            // id scale
        int8_t  _transpose =  cvval[3] + asr_params[1]; // octave +/-
        int32_t _sample    =  cvval[0] + asr_params[2]; // offset + CV
        int8_t  _index     =  cvval[1] + asr_params[3]; // index 
        int8_t  _num       =  cvval[2] + asr_params[4]; // # notes
    
        _sample *= (asr_params[5]); // att/mult
        _sample = _sample >> 4;
        // if the scale changed, reset ASR index
        if (_scale != THIS_SCALE) CLK_COUNT = 0;  
        // hold? or quantize       
        if (!digitalReadFast(TR2)) _hold(ASR, _index);                                 
        else {
             if (!digitalReadFast(TR3)) _transpose++;
             if (!digitalReadFast(TR4)) _transpose--;
             _sample = quant_sc(_sample, _scale, _transpose, _num); 
             // update + output 
             updateASR_indexed(ASR, _sample, _index); 
        }
       
        THIS_SCALE = _scale;    
        CLK_COUNT++;
        
        if (UI_MODE)  {
              _CLK_TIMESTAMP = millis();
              display_clock = true;      
        }
        else MENU_REDRAW = 1;             
}  

/*   -----------------------------------------------------     */

void ASR_init() {

    ASR = (ASRbuf*)malloc(sizeof(ASRbuf));

    ASR->first = 0;
    ASR->last  = 0;  
    ASR->items = 0;

    for (int i = 0; i < MAX_ITEMS; i++) {
        pushASR(ASR, 0);    
    }
    asr_params[1] = asr_display_params[1] = 0; // octave offset
    asr_params[4] = asr_display_params[4] = MAXNOTES; // init parameters: nps
    asr_params[5] = 16; // att/mult
}  

/* Resuming app */

void ASR_resume() {
}

/* add new sample: */

void pushASR(struct ASRbuf* _ASR, uint16_t _sample) {
 
        _ASR->items++;
        _ASR->data[_ASR->last] = _sample;
        _ASR->last = (_ASR->last+1); 
}

/* pop first element (oldest): */

void popASR(struct ASRbuf* _ASR) {
 
        _ASR->first=(_ASR->first+1); 
        _ASR->items--;
}

/* ASR + ringbuffer */

void updateASR_indexed(struct ASRbuf* _ASR, uint16_t _sample, int8_t _delay) {
  
    uint8_t out;
    int16_t _max_delay = CLK_COUNT>>2; 
    
    popASR(_ASR);            // remove sample (oldest) 
    pushASR(_ASR, _sample);  // push new sample into buffer (last) 
    
    // don't mix up scales 
    if (_delay < 0) _delay = 0;
    else if (_delay > _max_delay) _delay = _max_delay;       
      
    out  = (_ASR->last)-1;
    out -= _delay;
    asr_outputs[0] = _ASR->data[out--];
    out -= _delay;
    asr_outputs[1] = _ASR->data[out--];
    out -= _delay;
    asr_outputs[2] = _ASR->data[out--];
    out -= _delay;
    asr_outputs[3] = _ASR->data[out--];
    // write to DAC:
    set8565_CHA(asr_outputs[0]); //  >> out 1 
    set8565_CHB(asr_outputs[1]); //  >> out 2 
    set8565_CHC(asr_outputs[2]); //  >> out 3  
    set8565_CHD(asr_outputs[3]); //  >> out 4 
}

/* ---------- don't update ringbuffer, just move the 4 values ------------- */

void _hold(struct ASRbuf* _ASR, int8_t _delay) {
  
  
        uint8_t out, _hold[4];
        int16_t _max_delay = CLK_COUNT>>2; 

        // don't mix up scales 
        if (_delay < 0) _delay = 0;
        else if (_delay > _max_delay) _delay = _max_delay;       
      
        out  = (_ASR->last)-1;
        _hold[0] = out -= _delay;
        asr_outputs[0] = _ASR->data[out--];
        _hold[1] = out -= _delay;
        asr_outputs[1] = _ASR->data[out--];
        _hold[2] = out -= _delay;
        asr_outputs[2] = _ASR->data[out--];
        _hold[3] = out -= _delay;
        asr_outputs[3] = _ASR->data[out--];
    
       // write to DAC:
       set8565_CHA(asr_outputs[0]); //  >> out 1 
       set8565_CHB(asr_outputs[1]); //  >> out 2 
       set8565_CHC(asr_outputs[2]); //  >> out 3  
       set8565_CHD(asr_outputs[3]); //  >> out 4 
       // hold :
       _ASR->data[_hold[0]] = asr_outputs[3];  
       _ASR->data[_hold[1]] = asr_outputs[0];
       _ASR->data[_hold[2]] = asr_outputs[1];
       _ASR->data[_hold[3]] = asr_outputs[2];
}  
