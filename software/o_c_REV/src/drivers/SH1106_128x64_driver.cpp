/*
*   (C) Copyright 2011, Oli Kraus
*   (C) Copyright 2013, Andrew Kroll (xxxajk)
*   (C) Copyright 2016, Patrick Dowling
*
*   Low-level driver code for SH1106 OLED with spicy DMA transfer.
*   Author: Patrick Dowling (pld@gurkenkiste.com)
*
*   Command sequences adapted from https://github.com/olikraus/u8glib/blob/master/csrc/u8g_dev_ssd1306_128x64.c
*   SPI transfer command adapted from https://github.com/xxxajk/spi4teensy3
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of version 3 of the GNU General Public License as
*   published by the Free Software Foundation at http://www.gnu.org/licenses,
*   with Additional Permissions under term 7(b) that the original copyright
*   notice and author attibution must be preserved and under term 7(c) that
*   modified versions be marked as different from the original.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*/


#include <Arduino.h>
#include "SH1106_128x64_driver.h"
#include "../../OC_gpio.h"
#include "../../OC_options.h"

#define DMA_PAGE_TRANSFER
#ifdef DMA_PAGE_TRANSFER
#include <DMAChannel.h>
static DMAChannel page_dma;
#endif
#ifndef SPI_SR_RXCTR
#define SPI_SR_RXCTR 0XF0
#endif

static uint8_t SH1106_data_start_seq[] = {
// u8g_dev_ssd1306_128x64_data_start
  0x10, /* set upper 4 bit of the col adr to 0 */
  0x02, /* set lower 4 bit of the col adr to 0 */
  0x00  /* 0xb0 | page */  
};

static uint8_t SH1106_init_seq[] = {
// u8g_dev_ssd1306_128x64_adafruit3_init_seq
  0x0ae,          /* display off, sleep mode */
  0x0d5, 0x080,   /* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
  0x0a8, 0x03f,   /* multiplex ratio, duty = 1/32 */

  0x0d3, 0x000,   /* set display offset */
  0x040,          /* start line */

  0x08d, 0x014,   /* [2] charge pump setting (p62): 0x014 enable, 0x010 disable */

  0x020, 0x000,   /* 2012-05-27: page addressing mode */ // PLD: Seems to work in conjuction with lower 4 bits of column data?
  #ifdef FLIP_180
  0x0a0,          /* segment remap a0/a1*/
  0x0c0,          /* c0: scan dir normal, c8: reverse */
  #else
  0x0a1,          /* segment remap a0/a1*/
  0x0c8,          /* c0: scan dir normal, c8: reverse */
  #endif
  0x0da, 0x012,   /* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */
  0x081, 0x0cf,   /* [2] set contrast control */
  0x0d9, 0x0f1,   /* [2] pre-charge period 0x022/f1*/
  0x0db, 0x040,   /* vcomh deselect level */
  
  0x02e,        /* 2012-05-27: Deactivate scroll */ 
  0x0a4,        /* output ram to display */
#ifdef INVERT_DISPLAY
  0x0a7,        /* inverted display mode */
#else
  0x0a6,        /* none inverted normal display mode */
#endif
  //0x0af,      /* display on */
};

static uint8_t SH1106_display_on_seq[] = {
  0xaf
};

/*static*/
void SH1106_128x64_Driver::Init() {
  OC::pinMode(OLED_CS, OUTPUT);
  OC::pinMode(OLED_RST, OUTPUT);
  OC::pinMode(OLED_DC, OUTPUT);
  //SPI_init(); 

  // u8g_teensy::U8G_COM_MSG_INIT
  digitalWriteFast(OLED_RST, HIGH);
  delay(1);
  digitalWriteFast(OLED_RST, LOW);
  delay(10);
  digitalWriteFast(OLED_RST, HIGH);

  // u8g_dev_ssd1306_128x64_adafruit3_init_seq
  digitalWriteFast(OLED_CS, OLED_CS_INACTIVE); // U8G_ESC_CS(0),             /* disable chip */
  digitalWriteFast(OLED_DC, LOW); // U8G_ESC_ADR(0),           /* instruction mode */

  digitalWriteFast(OLED_RST, LOW); // U8G_ESC_RST(1),           /* do reset low pulse with (1*16)+2 milliseconds */
  delay(20);
  digitalWriteFast(OLED_RST, HIGH);
  delay(20);

  digitalWriteFast(OLED_CS, OLED_CS_ACTIVE); // U8G_ESC_CS(1),             /* enable chip */

  SPI_send(SH1106_init_seq, sizeof(SH1106_init_seq));

  digitalWriteFast(OLED_CS, OLED_CS_INACTIVE); // U8G_ESC_CS(0),             /* disable chip */

#ifdef DMA_PAGE_TRANSFER
  page_dma.destination((volatile uint8_t&)SPI0_PUSHR);
  page_dma.transferSize(1);
  page_dma.transferCount(kPageSize);
  page_dma.disableOnCompletion();
  page_dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_TX);
  page_dma.disable();
#endif

  Clear();
}

/*static*/
void SH1106_128x64_Driver::Flush() {
#ifdef DMA_PAGE_TRANSFER
  // Assume DMA transfer has completed, else we're doomed
  digitalWriteFast(OLED_CS, OLED_CS_INACTIVE); // U8G_ESC_CS(0)
  page_dma.clearComplete();
  page_dma.disable();
  // DmaSpi.h::post_finishCurrentTransfer_impl
  SPI0_RSER = 0;
  SPI0_SR = 0xFF0F0000;
#endif
}

static uint8_t empty_page[SH1106_128x64_Driver::kPageSize];

/*static*/
void SH1106_128x64_Driver::Clear() {
  memset(empty_page, 0, sizeof(kPageSize));

  SH1106_data_start_seq[2] = 0xb0 | 0;
  digitalWriteFast(OLED_DC, LOW);
  digitalWriteFast(OLED_CS, OLED_CS_ACTIVE);
  SPI_send(SH1106_data_start_seq, sizeof(SH1106_data_start_seq));
  digitalWriteFast(OLED_DC, HIGH);
  for (size_t p = 0; p < kNumPages; ++p)
    SPI_send(empty_page, kPageSize);
  digitalWriteFast(OLED_CS, OLED_CS_INACTIVE); // U8G_ESC_CS(0)

  digitalWriteFast(OLED_DC, LOW);
  digitalWriteFast(OLED_CS, OLED_CS_ACTIVE);
  SPI_send(SH1106_display_on_seq, sizeof(SH1106_display_on_seq));
  digitalWriteFast(OLED_DC, HIGH);
}

/*static*/
void SH1106_128x64_Driver::SendPage(uint_fast8_t index, const uint8_t *data) {
  SH1106_data_start_seq[2] = 0xb0 | index;

  digitalWriteFast(OLED_DC, LOW); // U8G_ESC_ADR(0),           /* instruction mode */
  digitalWriteFast(OLED_CS, OLED_CS_ACTIVE); // U8G_ESC_CS(1),             /* enable chip */
  SPI_send(SH1106_data_start_seq, sizeof(SH1106_data_start_seq)); // u8g_WriteEscSeqP(u8g, dev, u8g_dev_ssd1306_128x64_data_start);
  digitalWriteFast(OLED_DC, HIGH); // /* data mode */

#ifdef DMA_PAGE_TRANSFER
  // DmaSpi.h::pre_cs_impl()
  SPI0_SR = 0xFF0F0000;
  SPI0_RSER = SPI_RSER_RFDF_RE | SPI_RSER_RFDF_DIRS | SPI_RSER_TFFF_RE | SPI_RSER_TFFF_DIRS;

  page_dma.sourceBuffer(data, kPageSize);
  page_dma.enable(); // go
#else
  SPI_send(data, kPageSize);
  digitalWriteFast(OLED_CS, OLED_CS_INACTIVE); // U8G_ESC_CS(0)
#endif
}

void SH1106_128x64_Driver::SPI_send(void *bufr, size_t n) {

  // adapted from https://github.com/xxxajk/spi4teensy3
  int i;
  int nf;
  uint8_t *buf = (uint8_t *)bufr;

  if (n & 1) {
    uint8_t b = *buf++;
    // clear any data in RX/TX FIFOs, and be certain we are in master mode.
    SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_CLR_RXF | SPI_MCR_CLR_TXF | SPI_MCR_PCSIS(0x1F);
    SPI0_SR = SPI_SR_TCF;
    SPI0_PUSHR = SPI_PUSHR_CONT | b;
    while (!(SPI0_SR & SPI_SR_TCF));
    n--;
  }
  // clear any data in RX/TX FIFOs, and be certain we are in master mode.
  SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_CLR_RXF | SPI_MCR_CLR_TXF | SPI_MCR_PCSIS(0x1F);
  // initial number of words to push into TX FIFO
  nf = n / 2 < 3 ? n / 2 : 3;
  // limit for pushing data into TX FIFO
  uint8_t* limit = buf + n;
  for (i = 0; i < nf; i++) {
    uint16_t w = (*buf++) << 8;
    w |= *buf++;
    SPI0_PUSHR = SPI_PUSHR_CONT | SPI_PUSHR_CTAS(1) | w;
  }
  // write data to TX FIFO
  while (buf < limit) {
          uint16_t w = *buf++ << 8;
          w |= *buf++;
          while (!(SPI0_SR & SPI_SR_RXCTR));
          SPI0_PUSHR = SPI_PUSHR_CONT | SPI_PUSHR_CTAS(1) | w;
          SPI0_POPR;
  }
  // wait for data to be sent
  while (nf) {
          while (!(SPI0_SR & SPI_SR_RXCTR));
          SPI0_POPR;
          nf--;
  }
}

/*static*/
void SH1106_128x64_Driver::AdjustOffset(uint8_t offset) {
  SH1106_data_start_seq[1] = offset; // lower 4 bits of col adr
}
