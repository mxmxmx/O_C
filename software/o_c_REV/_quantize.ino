#define MAXNOTES 7                    // max. notes per scale (see below)

/*  scales ... look like this: */
/*  names must be added to const String abc[MAXSCALES] (menu.ino) */

uint8_t scales[] = { // RAM
  
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

/* number of scales */
const int8_t MAXSCALES = (sizeof(scales)/7);

/* names for scales go here (names shouldn't be longer than 12 characters) */
const char *abc[MAXSCALES] =    {
  
  "locrian nat.", //0
  "locrian",      //1
  "dorian",       //2
  "pent. minor",  //3
  "minor",        //4
  "scriabin",     //5
  "marva",        //6
  "indian",       //7
  "aolean",       //8
  "phrygian",     //9
  "gamelan",      //10
  "hex sus",      //11
  "purvi",        //12
  "bhairav",      //13
  "pelog",        //14
  "whatever",     //15
  "etc",          //16
  "xxx",          //17
  "zzz"           //18
};

/* quantize note */

uint16_t quant_sc(int16_t _sample, uint8_t _scale, int8_t _transpose, int8_t _npsc) {
  
     int8_t _octave, _note, _out; 

     //which octave?
     if (_sample < 0) {
                         _octave = 0;
                         _note   = 0;
     }
     else if  (_sample < S_RANGE) { // oct 0-8
       
                        _note   = _sample >> 5;
                        _octave = _note / 12; 
                        _note  -= (_octave << 3) + (_octave << 2); // == % 12
     }
     else {
                        _octave = 9; // oct 9
                        _note   = (_sample >> 5) % 12;                
     }
     // transpose:
     _octave += _transpose;
     if (_octave > 9) _octave = 9;
     else if (_octave < 0) _octave = 0;
         
     //  limit # notes per scale:   
     if (_npsc < 2)  _npsc = 2;      
     else if (_npsc > MAXNOTES)  _npsc = MAXNOTES;
     
     //  get proximate note in scale:
     _out = getnote(_note, _scale, _npsc) + _octave*12;     
     
     if (_out > RANGE) _out -= 12;
     return semitones[_out];  
}  

/* ---------- get proximate note in scale ---------- */

uint8_t getnote(uint8_t _note, uint8_t _scale, uint8_t _npsc) { 
   
    uint8_t i = 0;
    uint8_t temp = 0;
    uint8_t *sc_ptr = (uint8_t *)scales;
    // select scale: _scale = 0, 7, 14, 21, etc :
    sc_ptr += _scale*MAXNOTES;
    // constrain :  
    if (_note > 11) {
          _note -= 12; 
          temp   = 12;
    }
    // find note :
    while(i < _npsc) { 
          if (_note == *sc_ptr) {
              break;}
          i++; sc_ptr++;
    } 
    // return or repeat 
    if (i < _npsc) return (*sc_ptr)+temp;
    else {
          _note += 1; 
          return getnote(_note, _scale, _npsc);
    }   
}  
