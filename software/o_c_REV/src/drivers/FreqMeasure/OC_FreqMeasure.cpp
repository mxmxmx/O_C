/* FreqMeasure Library, for measuring relatively low frequencies
 * http://www.pjrc.com/teensy/td_libs_FreqMeasure.html
 * Copyright (c) 2011 PJRC.COM, LLC - Paul Stoffregen <paul@pjrc.com>
 *
 * Version 1.1
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "OC_FreqMeasure.h"
#include "OC_FreqMeasureCapture.h"

#define FREQMEASURE_BUFFER_LEN 12
static volatile uint32_t buffer_value[FREQMEASURE_BUFFER_LEN];
static volatile uint8_t buffer_head;
static volatile uint8_t buffer_tail;
static uint16_t capture_msw;
static uint32_t capture_previous;

void FreqMeasureClass::begin(void)
{
	capture_init();
	capture_msw = 0;
	capture_previous = 0;
	buffer_head = 0;
	buffer_tail = 0;
	capture_start();
}

uint8_t FreqMeasureClass::available(void)
{
	uint8_t head, tail;

	head = buffer_head;
	tail = buffer_tail;

	if (head >= tail) 
		return (head - tail);

	return FREQMEASURE_BUFFER_LEN + head - tail;
}

uint32_t FreqMeasureClass::read(void)
{
	uint8_t head, tail;
	uint32_t value;

	head = buffer_head;
	tail = buffer_tail;
	if (head == tail) return 0xFFFFFFFF;
	tail = tail + 1;
	if (tail >= FREQMEASURE_BUFFER_LEN) tail = 0;
	value = buffer_value[tail];
	buffer_tail = tail;
	return value;
}

float FreqMeasureClass::countToFrequency(uint32_t count)
{
#if defined(__AVR__)
	return (float)F_CPU / (float)count;
#elif defined(__arm__) && defined(TEENSYDUINO) && defined(KINETISK)
	return (float)F_BUS / (float)count;
#elif defined(__arm__) && defined(TEENSYDUINO) && defined(KINETISL)
	return (float)(F_PLL/2) / (float)count;
#endif
}

void FreqMeasureClass::end(void)
{
	capture_shutdown();
}

void FTM_ISR_NAME (void)
{
	uint32_t capture, period, i;
	bool inc = false;

	if (capture_overflow()) {
		capture_overflow_reset();
		capture_msw++;
		inc = true;
	}

	if (capture_event()) {

		capture = capture_read();
		if (capture <= 0xE000 || !inc) {
			capture |= (capture_msw << 16);
		} else {
			capture |= ((capture_msw - 1) << 16);
		}
		// compute the waveform period
		period = capture - capture_previous;
		capture_previous = capture;
		// store it into the buffer
		i = buffer_head + 1;
		if (i >= FREQMEASURE_BUFFER_LEN) i = 0;
		if (i != buffer_tail) {
			buffer_value[i] = period;
			buffer_head = i;
		}
	}
}

FreqMeasureClass FreqMeasure;

