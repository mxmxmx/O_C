RotaryPlus
==========

An Arduino library for interfacing with rotary encoders

Based (heavily) on Ben Buxton's rotary library — I've added a position variable and a change() function.


Availble functions
==================

bool change() — returns true if the encoder has changed value

long pos() — returns the position of the encoder, returns a number inside the .limit if it's set

long .limit — sets a limit for the returned number

int  changeDir() — returns the direction of the last change (-1 or 1)

long setPos() — sets the encoders position manually


Background
==========

I am in the process of writing a piece of sequencer code that utilises two rotary encoders. One encoder moves through the steps whilst the other adjusts the pitch of that step. I have extended the rotary library to keep track of the position as well as always return a position inside of a limit to help make this as simple as possible. One encoder is limited to 8 steps (a bar) whilst the other is limited to 12 (pitches of the chromatic scale).
