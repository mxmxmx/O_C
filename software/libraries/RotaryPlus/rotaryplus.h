/*
 * Rotary encoder library for Arduino.
 */

#ifndef rotaryplus_h
#define rotaryplus_h

#include "Arduino.h"

// Enable this to emit codes twice per step.
//#define HALF_STEP

// Enable weak pullups
#define ENABLE_PULLUPS

// Values returned by 'process'
// No complete step yet.
#define DIR_NONE 0x0
// Clockwise step.
#define DIR_CW 0x10
// Anti-clockwise step.
#define DIR_CCW 0x20

class Rotary
{
  public:
    Rotary(char, char);
    // Process pin(s)
    unsigned char process();
    bool change();
    long pos();
    void setPos(long);
    int changeDir();
    int lastChange;
    long limit;
  private:
    unsigned char state;
    unsigned char pin1;
    unsigned char pin2;
    volatile long position = 0;
    long oldPosition = -999;
};

#endif
 
