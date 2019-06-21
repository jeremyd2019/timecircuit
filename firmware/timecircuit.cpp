#include <Arduino.h>
#include <assert.h>
#include <SPI.h>
#include <Wire.h>
#include <mm5450.h>

#include <time.h>
#include <util/usa_dst.h>

#include "seven_seg.h"
#include "fourteen_seg.h"

ISR(TIMER1_OVF_vect, ISR_NAKED)
{
	system_tick();
	reti();
}

MultiplexMM5450 RED(9), YELLOW(8), GREEN(7);

HT16K33QuadAlphanum RED_MONTH = HT16K33QuadAlphanum(0x70);

extern const uint32_t PROGMEM INITIAL_TIME;

void setup() {
	// put your setup code here, to run once:
	// enable timer 1 to fire an interrupt once per second
	TCCR1A = 0;
	ICR1 = 31250;
	TCCR1B = _BV(WGM13) | _BV(CS12);
	TIMSK1 |= _BV(TOIE1);

	set_zone(-8 * ONE_HOUR);
	set_dst(usa_dst);
	set_system_time(pgm_read_dword(&INITIAL_TIME));
	SPI.begin();
	RED.initialize();
	YELLOW.initialize();
	GREEN.initialize();
	Wire.begin();
	RED_MONTH.systemSetup(true);
	RED_MONTH.dimmingSet(5);
	// nice all-LEDs-on test pattern
	RED_MONTH.writeDigitRaw(0, 0xFFFF);
	RED_MONTH.writeDigitRaw(1, 0xFFFF);
	RED_MONTH.writeDigitRaw(2, 0xFFFF);
	RED_MONTH.writeDigitRaw(3, 0xFFFF);
	RED_MONTH.writeDisplay();
	RED_MONTH.displaySetup(true);
	RED.assignLedRange(0, 1, 30, 0x3FFFFFFF);
	RED.assignLedRange(1, 1, 30, 0x3FFFFFFF);
	RED.assignLedRange(2, 1, 30, 0x3FFFFFFF);
	Serial.begin(115200);
}

static const char MONTHS[12][3] PROGMEM = {
	{'J','A','N'}, {'F','E','B'}, {'M','A','R'},
	{'A','P','R'}, {'M','A','Y'}, {'J','U','N'},
	{'J','U','L'}, {'A','U','G'}, {'S','E','P'},
	{'O','C','T'}, {'N','O','V'}, {'D','E','C'}
};

// struct and function prototype stolen from AVR stdlib.h, and adapted for qi4 (aka int8_t, or char)
extern "C" {
typedef struct {
	int8_t quot;
	int8_t rem;
} qdiv_t;

#ifdef __AVR__
extern qdiv_t qdiv(int8_t __num, int8_t __denom) __asm__("__divmodqi4") __ATTR_CONST__;
}
#else
}
constexpr qdiv_t qdiv(const int8_t __num, const int8_t __denom) __ATTR_CONST__;
constexpr qdiv_t qdiv(const int8_t __num, const int8_t __denom)
{
	return {static_cast<int8_t>(__num / __denom), static_cast<int8_t>(__num % __denom)};
}
#endif

void loop() {
	// put your main code here, to run repeatedly:
	MultiplexMM5450::process({&RED, &YELLOW, &GREEN});
	static time_t serial_input = 0;
	while (Serial.available())
	{
		int ch = Serial.read();
		if (ch >= '0' && ch <= '9')
		{
			serial_input = serial_input * 10 + (ch - '0');
		}
		else
		{
			if ((ch == '\r' || ch == '\n') && serial_input != 0)
				set_system_time(serial_input - UNIX_OFFSET);
			serial_input = 0;
		}
	}
	static time_t last_now = (time_t)-1;
	time_t now;
	time(&now);
	if (now != last_now)
	{
		RED.assignLed(0, 31, now & 1);
		RED.assignLed(1, 31, now & 2);
		RED.assignLed(2, 31, now & 4);

		struct tm * curtm = localtime(&now);
		int8_t twelvehour = curtm->tm_hour % 12;
		if (twelvehour == 0)
			twelvehour = 12;
		bool pm = curtm->tm_hour >= 12;
		qdiv_t tmp = qdiv(curtm->tm_min, 10);
		writeDigit(RED, 0, 0, tmp.rem);
		tmp = qdiv(tmp.quot, 10);
		writeDigit(RED, 0, 1, tmp.rem);
		writeDigit(RED, 0, 2, twelvehour % 10);
		writeDigit(RED, 0, 3, (twelvehour / 10) % 10);
		RED.assignLed(0, 1, !pm);
		RED.assignLed(0, 30, pm);

		int16_t year = curtm->tm_year + 1900;
		div_t tmp2 = div(year, 10);
		writeDigit(RED, 1, 0, tmp2.rem);
		tmp2 = div(tmp2.quot, 10);
		writeDigit(RED, 1, 1, tmp2.rem);
		tmp2 = div(tmp2.quot, 10);
		writeDigit(RED, 1, 2, tmp2.rem);
		tmp2 = div(tmp2.quot, 10);
		writeDigit(RED, 1, 3, tmp2.rem);

		tmp = qdiv(curtm->tm_mday, 10);
		writeDigit(RED, 2, 0, tmp.rem);
		tmp = qdiv(tmp.quot, 10);
		writeDigit(RED, 2, 1, tmp.rem);
		uint8_t colon = curtm->tm_sec & 1;
		RED.assignLed(2,  1, colon);
		RED.assignLed(2, 30, colon);

		static int8_t last_month = -1;
		if (last_month != curtm->tm_mon)
		{
			const char * monthabbr = MONTHS[curtm->tm_mon];
			RED_MONTH.writeDigitRaw(0, 0x0);
			RED_MONTH.writeDigitAscii(1, pgm_read_byte(&monthabbr[0]));
			RED_MONTH.writeDigitAscii(2, pgm_read_byte(&monthabbr[1]));
			RED_MONTH.writeDigitAscii(3, pgm_read_byte(&monthabbr[2]));
			RED_MONTH.writeDisplay();
			last_month = curtm->tm_mon;
		}

		last_now = now;
	}
}
