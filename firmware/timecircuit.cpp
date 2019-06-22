#include <Arduino.h>
#include <assert.h>
#include <SPI.h>
#include <Wire.h>
#include <mm5450.h>

#include <time.h>
#include <util/usa_dst.h>

#include "divmod.h"

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
		uint8_t twelvehour = static_cast<uint8_t>(curtm->tm_hour) % 12;
		if (twelvehour == 0)
			twelvehour = 12;
		bool pm = curtm->tm_hour >= 12;
		divmod_t<uint8_t> tmp = divmod<uint8_t>(curtm->tm_min, 10);
		writeDigit(RED, 0, 0, tmp.rem);
		tmp = divmod<uint8_t>(tmp.quot, 10);
		writeDigit(RED, 0, 1, tmp.rem);
		writeDigit(RED, 0, 2, twelvehour % 10);
		writeDigit(RED, 0, 3, (twelvehour / 10) % 10);
		RED.assignLed(0, 1, !pm);
		RED.assignLed(0, 30, pm);

		// TODO: BC flag?  Year 0? (movie shows DEC 25 0000 as birth-of-christ)
#ifdef FULL_YEAR_RANGE
		uint16_t year = (curtm->tm_year >= -1900) ? curtm->tm_year + 1900 : -(curtm->tm_year + 1900);
#else
		// doesn't even handle year 10k with only 4 digits, so ignore overflow
		uint16_t year = curtm->tm_year + 1900;
		if (static_cast<int16_t>(year) < 0)
			year = -year;
#endif
		divmod_t<uint16_t> tmp2 = divmod<uint16_t>(year, 10);
		writeDigit(RED, 1, 0, tmp2.rem);
		tmp2 = divmod<uint16_t>(tmp2.quot, 10);
		writeDigit(RED, 1, 1, tmp2.rem);
		tmp2 = divmod<uint16_t>(tmp2.quot, 10);
		writeDigit(RED, 1, 2, tmp2.rem);
		tmp2 = divmod<uint16_t>(tmp2.quot, 10);
		writeDigit(RED, 1, 3, tmp2.rem);

		tmp = divmod<uint8_t>(curtm->tm_mday, 10);
		writeDigit(RED, 2, 0, tmp.rem);
		tmp = divmod<uint8_t>(tmp.quot, 10);
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
