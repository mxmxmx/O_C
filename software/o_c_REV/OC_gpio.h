#ifndef OC_GPIO_H_
#define OC_GPIO_H_

#define CV1 19
#define CV2 18
#define CV3 20
#define CV4 17

#define TR1 0
#define TR2 1
#define TR3 2
#define TR4 3

#define but_top 5
#define but_bot 4

#define OLED_DC 6
#define OLED_RST 7
#define OLED_CS 8

// These are inverted (?), see u8g_teensy.cpp
#define OLED_CS_HIGH LOW
#define OLED_CS_LOW  HIGH

#define DAC_RST 9
#define DAC_CS 10

#define encR1 15
#define encR2 16
#define butR  14

#define encL1 22
#define encL2 21
#define butL  23

// NOTE: back side :(
#define DEBUG_PIN_1 24
#define DEBUG_PIN_2 25

#endif // OC_GPIO_H_
