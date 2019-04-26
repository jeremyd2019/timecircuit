#include <Arduino.h>
#include "mm5450.h"
#include "seven_seg.h"


/*
  A
F   B
  G
E   C
  D
*/
static const uint8_t sevenseg_font[10] = {
	0x2F, //0
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

// MMM DD YYYY HH(colon)MM(am/pm)
// I'm thinking YYYY on bank 0, HHMM plus am/pm leds on bank 1, and DD plus colon on bank 2
void writeDigit(MultiplexMM5450 & color, uint8_t bankno, uint8_t digitno, uint8_t digit)
{
	assert(bankno < 3 && digitno < 4 && digit < 10);
	color.assignLedRange(bankno, digitno*7 + 1, 7, sevenseg_font[digit]);
}

