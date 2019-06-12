#include <Arduino.h>
#include <assert.h>
#include <SPI.h>

#include "mm5450.h"

#include <time.h>
#include <util/usa_dst.h>

#include "seven_seg.h"

ISR(TIMER1_OVF_vect, ISR_NAKED)
{
	system_tick();
	reti();
}

MultiplexMM5450 RED(9), YELLOW(8), GREEN(7);

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
	// nice all-LEDs-on test pattern
	RED.assignLedRange(0, 1, 30, 0x3FFFFFFF);
	RED.assignLedRange(1, 1, 30, 0x3FFFFFFF);
	RED.assignLedRange(2, 1, 30, 0x3FFFFFFF);
	Serial.begin(115200);
}

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
		writeDigit(RED, 0, 0, curtm->tm_min % 10);
		writeDigit(RED, 0, 1, (curtm->tm_min / 10) % 10);
		writeDigit(RED, 0, 2, twelvehour % 10);
		writeDigit(RED, 0, 3, (twelvehour / 10) % 10);
		RED.assignLed(0, 1, !pm);
		RED.assignLed(0, 30, pm);

		int16_t year = curtm->tm_year + 1900;
		writeDigit(RED, 1, 0, year % 10);
		writeDigit(RED, 1, 1, (year / 10) % 10);
		writeDigit(RED, 1, 2, (year / 100) % 10);
		writeDigit(RED, 1, 3, (year / 1000) % 10);

		writeDigit(RED, 2, 0, curtm->tm_mday % 10);
		writeDigit(RED, 2, 1, (curtm->tm_mday / 10) % 10);
		uint8_t colon = curtm->tm_sec & 1;
		RED.assignLed(2,  1, colon);
		RED.assignLed(2, 30, colon);

		last_now = now;
	}
}
