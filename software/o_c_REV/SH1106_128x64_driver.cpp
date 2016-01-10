#include "SH1106_128x64_driver.h"

#include <Arduino.h>
#include <spi4teensy3.h>

#include "O_C_gpio.h"

/*static*/ uint8_t SH1106_128x64_Driver::data_start_seq[] = {
// u8g_dev_ssd1306_128x64_data_start
  0x10, /* set upper 4 bit of the col adr to 0 */
  0x00, /* set lower 4 bit of the col adr to 0  */
  0x00  /* 0xb0 | page */  
};

/*static*/ uint8_t SH1106_128x64_Driver::init_seq[] = {
// u8g_dev_ssd1306_128x64_adafruit3_init_seq
  0x0ae,        /* display off, sleep mode */
  0x0d5, 0x080,   /* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
  0x0a8, 0x03f,   /* */

  0x0d3, 0x000,   /*  */

  0x040,        /* start line */
  
  0x08d, 0x014,   /* [2] charge pump setting (p62): 0x014 enable, 0x010 disable */

  0x020, 0x002,   /* 2012-05-27: page addressing mode */
  0x0a1,        /* segment remap a0/a1*/
  0x0c8,        /* c0: scan dir normal, c8: reverse */
  0x0da, 0x012,   /* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */
  0x081, 0x0cf,   /* [2] set contrast control */
  0x0d9, 0x0f1,   /* [2] pre-charge period 0x022/f1*/
  0x0db, 0x040,   /* vcomh deselect level */
  
  0x02e,        /* 2012-05-27: Deactivate scroll */ 
  0x0a4,        /* output ram to display */
  0x0a6,        /* none inverted normal display mode */
  0x0af,        /* display on */
};

// These are inverted (?), see u8g_teensy.cpp
#define OLED_CS_HIGH LOW
#define OLED_CS_LOW  HIGH

/*static*/
void SH1106_128x64_Driver::Init() {
  pinMode(OLED_CS, OUTPUT);
  pinMode(OLED_RST, OUTPUT);
  pinMode(OLED_DC, OUTPUT);
  spi4teensy3::init();

  // u8g_teensy::U8G_COM_MSG_INIT
  digitalWriteFast(OLED_RST, HIGH);
  delay(1);
  digitalWriteFast(OLED_RST, LOW);
  delay(10);
  digitalWriteFast(OLED_RST, HIGH);

  // u8g_dev_ssd1306_128x64_adafruit3_init_seq
  digitalWriteFast(OLED_CS, OLED_CS_LOW); // U8G_ESC_CS(0),             /* disable chip */
  digitalWriteFast(OLED_DC, LOW); // U8G_ESC_ADR(0),           /* instruction mode */

  digitalWriteFast(OLED_RST, LOW); // U8G_ESC_RST(1),           /* do reset low pulse with (1*16)+2 milliseconds */
  delay(20);
  digitalWriteFast(OLED_RST, HIGH);
  delay(20);

  digitalWriteFast(OLED_CS, OLED_CS_HIGH); // U8G_ESC_CS(1),             /* enable chip */

  spi4teensy3::send(init_seq, sizeof(init_seq));

  digitalWriteFast(OLED_CS, OLED_CS_LOW); // U8G_ESC_CS(0),             /* disable chip */
}

/*static*/
void SH1106_128x64_Driver::SendPage(uint_fast8_t index, const uint8_t *data) {
  data_start_seq[2] = 0xb0 | index;

  digitalWriteFast(OLED_DC, LOW); // U8G_ESC_ADR(0),           /* instruction mode */
  digitalWriteFast(OLED_CS, OLED_CS_HIGH); // U8G_ESC_CS(1),             /* enable chip */
  spi4teensy3::send(data_start_seq, sizeof(data_start_seq)); // u8g_WriteEscSeqP(u8g, dev, u8g_dev_ssd1306_128x64_data_start);
  digitalWriteFast(OLED_DC, HIGH); // /* data mode */

  spi4teensy3::send(data, kPageSize);
  digitalWriteFast(OLED_CS, OLED_CS_LOW); // U8G_ESC_CS(0)
}
