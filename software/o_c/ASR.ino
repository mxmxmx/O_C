/*

ASR / ring buffer / quantization


*/

/*  ASR/quantization params */

#define MAXNOTES 7            // max. notes per scale (see below)
#define MAX_DELAY 63          // max. delay < > ring buffer is 256
#define RANGE 119             // [0-119] = 120 semitones < > 10 octaves 
#define S_RANGE 119<<5        // same thing, spread over 12 bit (ish)

int16_t  asr_params[6];       // ASR params :: scale, octave, offset, delay, nps, attenuation
uint16_t asr_outputs[4];      // ASR out
uint32_t CLK_COUNT  = 0;      // count clocks
uint8_t  THIS_SCALE = 0;      // track scale change

uint16_t semitones[RANGE+1];  // DAC output LUT


/*  scales ... look like this: */
/* add name to const String abc[MAXSCALES] (menu.ino) */

uint8_t scales[] = {
  
/* {0, 1, 2, 3, 4, 5,  6,  7,  8,  9, 10, 11}  == chromatic */ 
0, 2, 3, 5, 6, 8, 10,   // 0: locrian natural
0, 1, 3, 5, 6, 8, 10,   // 1: locrian
0, 2, 3, 5, 7, 9, 10,   // 2: dorian
0, 3, 5, 7, 10,12,12,   // 3: minor pentatonic
0, 2, 3, 5, 7, 8, 10,   // 4: minor
0, 1, 4, 7, 9, 12,12,   // 5: scriabin
0, 1, 4, 6, 7, 9, 11,   // 6: marva
0, 1, 4, 5, 8, 10,12,   // 7: indian
0, 2, 3, 5, 7, 8, 10,   // 8: aolean
0, 1, 3, 5, 7, 8, 10,   // 9: phrygian
0, 1, 3, 7, 8,12, 12,   // 10: gamelan
0, 2, 5, 7, 9, 10,12,   // 11: hex sus
0, 1, 4, 6, 7, 8, 11,   // 12: purvi
0, 1, 4, 5, 7, 8, 11,   // 13: bhairav
0, 1, 3, 7,10, 12,12,   // 14: pelog
0, 1, 3, 7, 9, 11,12,   // 15: whatever
0, 1, 5, 7, 8, 12,12,   // 16: etc
0, 1, 5, 6, 10,12,12,   // 17: xxx
0, 1, 2, 3, 10,12,12,   // 18: ?
};


                                
/* update ASR params + etc */
void _ASR() {

         /*  update asr_params < > scale, octave, offset, delay, nps, attenuation */
        uint8_t _scale     =  MAXNOTES * asr_params[0]; // id scale
        int8_t  _transpose =  cvval[3] + asr_params[1]; // octave +/-
        int32_t _sample    =  cvval[0] + asr_params[2]; // offset + CV
        int8_t  _index     =  cvval[1] + asr_params[3]; // index 
        int8_t  _num       =  cvval[2] + asr_params[4]; // # notes
        _sample *= (asr_params[5]);                     // att/mult
        _sample = _sample >> 4;
        /* if the scale changed, reset ASR index */
        if (asr_params[0] != THIS_SCALE) CLK_COUNT = 0;  
        /* hold? or quantize */      
        if (!digitalReadFast(TR2)) _hold(ASR, _index);                                 
        else {
             if (!digitalReadFast(TR3)) _transpose++;
             if (!digitalReadFast(TR4)) _transpose--;
             _sample = quant_sc(_sample, _scale, _transpose, _num); 
             /* update + output */
             updateASR_indexed(ASR, _sample, _index); 
        }
       
        THIS_SCALE = asr_params[0];    
        CLK_COUNT++;
        
        if (UImode)  {
              LAST_TRIG = millis();
              display_clock = true;      
        }
        
        else MENU_REDRAW = 1;               
}  


/*   -----------------------------------------------------     */


void init_ASR(struct ASRbuf* _ASR) {
         
    _ASR->first = 0;
    _ASR->last  = 0;  
    _ASR->items = 0;

    for (int i = 0; i < MAX_ITEMS; i++) {
        pushASR(_ASR, 0);    
    }
    asr_params[1] = asr_display_params[1] = 0; // octave offset
    asr_params[4] = asr_display_params[4] = MAXNOTES; // init parameters: nps
    asr_params[5] = 16; // att/mult
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
    popASR(_ASR);            // remove sample (oldest) 
    pushASR(_ASR, _sample);  // push new sample into buffer (last) 
    
    /* don't mix up scales */
    if (_delay < 0) _delay = 0;
    else if (_delay > (CLK_COUNT>>2)) _delay = CLK_COUNT>>2;       
      
    out  = (_ASR->last)-1;
    out -= _delay;
    asr_outputs[0] = _ASR->data[out--];
    out -= _delay;
    asr_outputs[1] = _ASR->data[out--];
    out -= _delay;
    asr_outputs[2] = _ASR->data[out--];
    out -= _delay;
    asr_outputs[3] = _ASR->data[out--];
    /* write to DACs: top left (B) - > top right (A) - > bottom left (D) - > bottom right (C) */
    set8565_CHB(asr_outputs[0]); // ch B >> out 1 
    set8565_CHA(asr_outputs[1]); // ch A >> out 2 
    set8565_CHD(asr_outputs[2]); // ch D >> out 3  
    set8565_CHC(asr_outputs[3]); // ch C >> out 4 
    return;
}

/* quantize note */

uint16_t quant_sc(int16_t _sample, uint8_t _scale, int8_t _transpose, int8_t _npsc) {
  
     int8_t _octave, _note, _out; 

     /* which octave? */
     if (_sample < 0) {
                         _octave = 0;
                         _note   = 0;
     }
     
     else if  (_sample < S_RANGE) {
       
                        _note   = _sample >> 5;
                        _octave = _note / 12; 
                        _note  -= (_octave << 3) + (_octave << 2); // == % 12
     }
     
     else {
                        _octave = 9;
                        _note   = (_sample >> 5) % 12;  
              
     }
     /* transpose */
     _octave += _transpose;
     if (_octave > 9) _octave = 9;
     else if (_octave < 0) _octave = 0;
         
     /*  check limits ? # notes per scale */    
     if (_npsc < 2)  _npsc = 2;      
     else if (_npsc > MAXNOTES)  _npsc = MAXNOTES;
     
     /*  get nearest note in scale */
     _out = getnote(_note, _scale, _npsc) + _octave*12;     
     
     if (_out > RANGE) _out -= 12;
     return semitones[_out];  
}  

/*  get nearest note in scale  */

uint8_t getnote(uint8_t _note, uint8_t _scale, uint8_t _npsc) { // _scale = 0, 7, 14, 21 .. etc
   
    uint8_t i = 0;
    uint8_t temp = 0;
    uint8_t (*sc_ptr) = scales;
    sc_ptr += _scale;
      
    if (_note > 11) {
          _note -= 12; 
          temp   = 12;
    }
    
    while(i < _npsc) { 
          if (_note == *sc_ptr) {
              break;}
          i++; sc_ptr++;
    } 
   
    if (i < _npsc) return  (*sc_ptr)+temp;
    else {
          _note += 1; 
          return getnote(_note, _scale, _npsc);
          }   
}  

/* ---------- don't update ringbuffer, just move the 4 values ------------- */

void _hold(struct ASRbuf* _ASR, int8_t _delay) {
  
    uint8_t out, keep0, keep1, keep2, keep3;
    /* don't mix up scales */
    if (_delay < 0) _delay = 0;
    else if (_delay > (CLK_COUNT>>2)) _delay = CLK_COUNT>>2;       
      
    out  = (_ASR->last)-1;
    
    keep0 = out -= _delay;
    asr_outputs[0] = _ASR->data[out--];
    keep1 = out -= _delay;
    asr_outputs[1] = _ASR->data[out--]; 
    keep2 = out -= _delay;
    asr_outputs[2] = _ASR->data[out--];
    keep3 = out -= _delay;
    asr_outputs[3] = _ASR->data[out--];
    /* ASR out */
    set8565_CHB(asr_outputs[0]); // ch B >> out 1 
    set8565_CHA(asr_outputs[1]); // ch A >> out 2 
    set8565_CHD(asr_outputs[2]); // ch D >> out 3  
    set8565_CHC(asr_outputs[3]); // ch C >> out 4 
    /* now hold */
    _ASR->data[keep0-1] = asr_outputs[3];  
    _ASR->data[keep1-1] = asr_outputs[0];
    _ASR->data[keep2-1] = asr_outputs[1];
    _ASR->data[keep3-1] = asr_outputs[2];
}  


