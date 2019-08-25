#include <Arduino.h>
#include <mm5450.h>
#include "arduino_utils.h"
#include "seven_seg.h"


/*
  A
F   B
  G
E   C
  D
*/
static const uint8_t sevenseg_font[10] PROGMEM = {
	0x3F, //0
	0x06, //1
	0x5B, //2
	0x4F, //3
	0x66, //4
	0x6D, //5
	0x7C, //6 (movie clips didn't have top on 6)
	0x07, //7
	0x7F, //8
	0x67, //9 (movie clips didn't have bottom on 9)
};

static const uint8_t SWIZZLE_N_BITS[28] PROGMEM = {
	16, 17, 18, 19, 20, 15, 14,
	12, 13, 21, 22, 23, 11, 10,
	8, 9, 24, 25, 26, 7, 6,
	4, 5, 27, 28, 29, 3, 2
};

// MMM DD YYYY(am/pm)HH(colon)MM
// I'm thinking YYYY on bank 1, HHMM plus am/pm leds on bank 0, and DD plus colon on bank 2
void MultiplexMM5450SevenSeg::writeDigit(uint8_t bankno, uint8_t digitno, uint8_t digit)
{
	assert(bankno < 3 && digitno < 4 && digit < 10);
#ifdef SANE_SOFTWARE
	assignLedRange(bankno, digitno*7 + 1, 7, pgm_read_byte_func(&sevenseg_font[digit]);
#else
	uint8_t glyph = pgm_read_byte_func(&sevenseg_font[digit]);
	assignLed(bankno, pgm_read_byte_func(&SWIZZLE_N_BITS[digitno * 7 + 0]), glyph & 0x01);
	assignLed(bankno, pgm_read_byte_func(&SWIZZLE_N_BITS[digitno * 7 + 1]), glyph & 0x02);
	assignLed(bankno, pgm_read_byte_func(&SWIZZLE_N_BITS[digitno * 7 + 2]), glyph & 0x04);
	assignLed(bankno, pgm_read_byte_func(&SWIZZLE_N_BITS[digitno * 7 + 3]), glyph & 0x08);
	assignLed(bankno, pgm_read_byte_func(&SWIZZLE_N_BITS[digitno * 7 + 4]), glyph & 0x10);
	assignLed(bankno, pgm_read_byte_func(&SWIZZLE_N_BITS[digitno * 7 + 5]), glyph & 0x20);
	assignLed(bankno, pgm_read_byte_func(&SWIZZLE_N_BITS[digitno * 7 + 6]), glyph & 0x40);
#endif
}

