/* the spi4teensy3 library can be found at 
  https://github.com/xxxajk/spi4teensy3
  
  To use it, you must have Teensyduino >1.6 installed (versions up to 1.6 have bugs that prevent 
  the library to work).
*/

/*
  Using the fast SPI library for the teensy3 with shift registers
  hase, 28-Nov-2013
  
  This example uses 74xx165 chips
  This is a parallel-in serial-out shift registers with latch inputs.
  While its ~PL input is LOW, the values read from the parallel inputs will be in the register.
  This also means that the input value from D7 will appear at the Q7 output.
  When ~PL is HIGH, the values on the parallel inputs are ignored.
  The ~PL pin of all 74xx165 are wired together and to an output of the Teensy3
  
  The ~CE (clock enable) input and the CP (clock pulse) actually are both clock inputs: they are internally
  combined by an OR gate (after ~CE has been inverted); so it is possible to use CP as an active-HIGH enable and
  ~CE as a low-active clock - or use CP as the clock and ~CE as the active-LOW enable.
  Usually they are used with the functions suggested by their names.
  This is what we do here.
  All ~CE pins are wired together and to an output of the Teensy3.
  
  The CP (clock) is again wired together for all registers and to pin 13/LED/SCK of the Teensy3: this is the SCK pin 
  operated by the hardware-SPI interface in the Freescale processor.
  
  Multiple registers can be cascaded, I use 2 registers here.
  When cascading, connect one serial input (DS pin) to the Q7 output of another chip.
  Connect the remaining Q7 output to the MISO/DIN pin of the Teensy3: this is the serial output of the chain of registers.
  
  A note about shift registers and SPI: the registers are *not* true SPI.
  A true SPI device will disconnect its output from the serial BUS when not selected.
  The shift registers do not do this: the output (Q7) is always driven.
  Always as in 100% of the time: when shifting (~CE LOW, pulses on CP), when loading (~PL LOW)
  and when idle (~PE and ~CE both HIGH).
  This would interfere with the signal that another device might want to transmit on the BUS.
  
 Using an SPI BUS for shifters and real SPI devices is a mistake commonly seen in forum posts.
 It is possible to make the shift registers play nice with other SPI devices - at extra cost: when not shifting
 a driver must disconnect the Q7 from the BUS line; 1/8th of a 74xx244 could do this or one mux out of a 74xx157 or
 any gate that has a Tri-State output.
 Use the ~CE signal to switch on such a driver if you need to use one SPI for shifters and other devices.
 */
 
#define DEBUG_PROGRESS 1 // compile in some debug output on the serial

#include <spi4teensy3.h>
#include <Arduino.h>

/* the 4 functions _write(), _read(), _isatty() and _fstat() are used by the printf routines for output on the 
   serial pipe established between the Teensy3 and the USB host.
   If you do not use printf or its sibling printf_P, you can remove them
*/
extern "C" {

        int _write(int fd, const char *ptr, int len) {
                int j;
                for (j = 0; j < len; j++) {
                        if (fd == 1)
                                Serial.write(*ptr++);
                        else if (fd == 2)
                                Serial3.write(*ptr++);
                }
                return len;
        }

        int _read(int fd, char *ptr, int len) {
                if (len > 0 && fd == 0) {
                        while (!Serial.available());
                        *ptr = Serial.read();
                        return 1;
                }
                return 0;
        }

#include <sys/stat.h>

        int _fstat(int fd, struct stat *st) {
                memset(st, 0, sizeof (*st));
                st->st_mode = S_IFCHR;
                st->st_blksize = 1024;
                return 0;
        }

        int _isatty(int fd) {
                return (fd < 3) ? 1 : 0;
        }
}

const byte chipselect = 9; // digital pin to use for chip select/clock enable on the latch/shifter
const byte pload = 10; // digital pin to trigger parallel loading of the shift registers
const byte num_register = 2; // number of shifters in the daisy-chain

void setup() {
  pinMode(chipselect, OUTPUT);
  pinMode(pload, OUTPUT);
  digitalWrite(chipselect, HIGH); // ~CE is active LOW, so this disables shifting
  digitalWrite(pload, HIGH); // ~PL ls active LOW, so this disables loading
/* // init is an overloaded method of the C++ object. Depending on the number of arguments, it acts differently.
   no arguments: max speed, clock polarity 0 and clock phase 0
   one argument: the argument defines the speed, cpol and cpha are 0 as above
   two arguments: clock polarity and clock phase
   three arguments: speed, clock polarity and clock phase
*/
  spi4teensy3::init(0,0,0); 
}

void loop() {
  
  uint8_t inval[num_register]; // buffer variable to store the read data

/* Note: the printf and printf_P are not standard in the Arduino environment
   printf works much like the standard libc version
   printf_P takes a PSTR argument rather than a string: this string is stored in Flash and occupies no RAM
*/
#ifdef DEBUG_PROGRESS
  printf_P(PSTR("start SPI read\n"));
#endif
  digitalWrite(pload, LOW); // load the input data to the shift registers
  delayMicroseconds(1); // give the chips some time to do their job
  digitalWrite(pload, HIGH); // not loading any more
  digitalWrite(chipselect, LOW); // enable shifting
  delayMicroseconds(1); // give the chips some time to do their job
  spi4teensy3::receive(inval, num_register); // read in the values rom the register chain
  digitalWrite(chipselect, HIGH); // disable clock again
#ifdef DEBUG_PROGRESS
  printf_P(PSTR("read from shift regs by SPI:"));
  byte i; 
  for (i=0;i<num_register;i++) {
    printf("%x", inval[i]);
  }
#endif

/* and this is the place where _you_ do with the read values whatever you need to do */

}
