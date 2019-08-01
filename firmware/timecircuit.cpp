#include <Arduino.h>
#include <assert.h>
#include <SPI.h>
#include <Wire.h>
#include <mm5450.h>

#include <time.h>
#include <util/usa_dst.h>

#include "arduino_utils.h"
#include "divmod.h"
#include "subbytearray.h"

#include "seven_seg.h"
#include "fourteen_seg.h"


// TODO current time should be on GREEN, not RED, but I only built RED...
#define CURRENT_TIME_ON_RED

ISR(TIMER1_OVF_vect, ISR_NAKED)
{
	system_tick();
	reti();
}

static const SubByteArray2D<4, uint8_t, 5, 3, pgm_read_byte_func> KEYMAPPING PROGMEM = {{
	0x1, 0x2, 0x3,
	0x4, 0x5, 0x6,
	0x7, 0x8, 0x9,
	0xD, 0x0, 0xE,
	0xA, 0xB, 0xC,
	0xF // to fill out the even number of elements
}};

static const char MONTHS[12][3] PROGMEM = {
	{'J','A','N'}, {'F','E','B'}, {'M','A','R'},
	{'A','P','R'}, {'M','A','Y'}, {'J','U','N'},
	{'J','U','L'}, {'A','U','G'}, {'S','E','P'},
	{'O','C','T'}, {'N','O','V'}, {'D','E','C'}
};

typedef SubByteArray<4, uint8_t, 12> TimeDisplay_t;

struct DisplayRow
{
	MultiplexMM5450 mm5450;
	HT16K33QuadAlphanum ht16k33;
	TimeDisplay_t value;
	uint8_t last_month;

	inline DisplayRow(uint8_t ss_pin, uint8_t i2c_addr)
		: mm5450 (ss_pin)
		, ht16k33 (i2c_addr)
		, value {0}
		, last_month (-1)
	{ }

	void setup()
	{
		mm5450.initialize();
		ht16k33.systemSetup(true);
		ht16k33.rowIntSet(true, true);
		ht16k33.dimmingSet(5);
		// nice all-LEDs-on test pattern
		for (uint8_t i = 0; i < 4; ++i)
			ht16k33.writeDigitRaw(i, 0xFFFF);
		for (uint8_t i = 0; i < 3; ++i)
			mm5450.assignLedRange(i, 1, 30, 0x3FFFFFFF);
		ht16k33.writeDisplay();
		ht16k33.displaySetup(true);
	}

	void blank()
	{
		mm5450.assignLedRange(0, 1, 30, 0);
		mm5450.assignLedRange(1, 2, 28, 0);
		mm5450.assignLedRange(2, 10, 14, 0);
		if (last_month != 254)
		{
			ht16k33.clear();
			ht16k33.writeDisplay();
			last_month = 254;
		}
	}

	inline void updateColon(uint8_t colon)
	{
		mm5450.assignLed(2,  1, colon);
		mm5450.assignLed(2, 30, colon);
	}

	inline void writeTime()
	{
		writeTime(static_cast<uint8_t>(value[0] * 10 + value[1] - 1) % 12, value[8] * 10 + value[9]);
	}

	void writeTime(time_t timeval);

private:
	void writeTime(uint8_t month, uint8_t hour);
};

static DisplayRow RED(9, 0x70), GREEN(8, 0x71), YELLOW(7, 0x72);

struct Overrides
{
	uint8_t red:1;
	uint8_t green:1;
	uint8_t yellow:1;

	inline bool current() const
	{
#ifdef CURRENT_TIME_ON_RED
		return red;
#else
		return green;
#endif
	}

	inline void set(const TimeDisplay_t * current, uint8_t value)
	{
		if (current == &RED.value)
			red = value;
		else if (current == &GREEN.value)
			green = value;
		else if (current == &YELLOW.value)
			yellow = value;
	}
};

static Overrides overrides = {0};

extern const uint32_t PROGMEM INITIAL_TIME;

static volatile bool polling = false;
static volatile unsigned long lastPollMillis = 0;
static void onPin3Rising()
{
	if (!polling)
	{
		lastPollMillis = millis();
		polling = true;
	}
}

void setup() {
	// put your setup code here, to run once:
	// enable timer 1 to fire an interrupt once per second
	TCCR1A = 0;
	ICR1 = 31250;
	TCCR1B = _BV(WGM13) | _BV(CS12);
	TIMSK1 |= _BV(TOIE1);

	set_zone(-8 * ONE_HOUR);
	set_dst(usa_dst);
	set_system_time(pgm_read_dword_func(&INITIAL_TIME));
	SPI.begin();
	Wire.begin();
	pinMode(3, INPUT);
	attachInterrupt(1, onPin3Rising, RISING);
	RED.setup();
	GREEN.setup();
	YELLOW.setup();
	Serial.begin(115200);
}

void DisplayRow::writeTime(uint8_t month, uint8_t hour)
{
	uint8_t twelvehour = hour % 12;
	if (twelvehour == 0)
		twelvehour = 12;
	bool pm = hour >= 12;
	writeDigit(mm5450, 0, 0, value[11]);
	writeDigit(mm5450, 0, 1, value[10]);
	writeDigit(mm5450, 0, 2, twelvehour % 10);
	writeDigit(mm5450, 0, 3, (twelvehour / 10) % 10);
	mm5450.assignLed(0, 1, !pm);
	mm5450.assignLed(0, 30, pm);

	// TODO: BC flag?  Year 0? (movie shows DEC 25 0000 as birth-of-christ)
	writeDigit(mm5450, 1, 0, value[7]);
	writeDigit(mm5450, 1, 1, value[6]);
	writeDigit(mm5450, 1, 2, value[5]);
	writeDigit(mm5450, 1, 3, value[4]);

	writeDigit(mm5450, 2, 0, value[3]);
	writeDigit(mm5450, 2, 1, value[2]);

	if (last_month != month)
	{
		const char * monthabbr = MONTHS[month];
		ht16k33.writeDigitRaw(0, 0x0);
		ht16k33.writeDigitAscii(1, pgm_read_byte_func(&monthabbr[0]));
		ht16k33.writeDigitAscii(2, pgm_read_byte_func(&monthabbr[1]));
		ht16k33.writeDigitAscii(3, pgm_read_byte_func(&monthabbr[2]));
		ht16k33.writeDisplay();
		last_month = month;
	}
}

void DisplayRow::writeTime(time_t timeval)
{
	struct tm * curtm = localtime(&timeval);
#ifdef FULL_YEAR_RANGE
	uint16_t year = (curtm->tm_year >= -1900) ? curtm->tm_year + 1900 : -(curtm->tm_year + 1900);
#else
	// doesn't even handle year 10k with only 4 digits, so ignore overflow
	uint16_t year = curtm->tm_year + 1900;
#endif
	// TODO: BC flag?  Year 0? (movie shows DEC 25 0000 as birth-of-christ)
	if (static_cast<int16_t>(year) < 0)
		year = -year;

	// don't need month filled in, it needs as an integer anyway, so passed in as parameter

	divmod_t<uint8_t> tmp = divmod<uint8_t>(curtm->tm_mday, 10);
	value[3] = tmp.rem;
	tmp = divmod<uint8_t>(tmp.quot, 10);
	value[2] = tmp.rem;

	divmod_t<uint16_t> tmp2 = divmod<uint16_t>(year, 10);
	value[7] = tmp2.rem;
	tmp2 = divmod<uint16_t>(tmp2.quot, 10);
	value[6] = tmp2.rem;
	tmp2 = divmod<uint16_t>(tmp2.quot, 10);
	value[5] = tmp2.rem;
	tmp2 = divmod<uint16_t>(tmp2.quot, 10);
	value[4] = tmp2.rem;

	// don't need hour filled in, it needs as an integer anyway, so passed in as parameter

	tmp = divmod<uint8_t>(curtm->tm_min, 10);
	value[11] = tmp.rem;
	tmp = divmod<uint8_t>(tmp.quot, 10);
	value[10] = tmp.rem;

	writeTime(curtm->tm_mon, curtm->tm_hour);
}

static time_t parsetime(const TimeDisplay_t & input)
{
	struct tm parsed = {0};
	// tm_isdst must be negative for mktime to determine if DST should be in effect
	parsed.tm_isdst = -1;
	parsed.tm_mon = input[0] * 10 + input[1] - 1;
	parsed.tm_mday = input[2] * 10 + input[3];
	parsed.tm_year = input[4] * 1000 + input[5] * 100 + input[6] * 10 + input[7] - 1900;
	parsed.tm_hour = input[8] * 10 + input[9];
	parsed.tm_min = input[10] * 10 + input[11];
	return mktime(&parsed);
}

void loop() {
	// put your main code here, to run repeatedly:
	MultiplexMM5450::process({&RED.mm5450, &GREEN.mm5450, &YELLOW.mm5450});
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
	static TimeDisplay_t * inputbuffer = NULL;
	static uint8_t inputdigit = 0;
	bool forceupdate = false;
	if (polling && lastPollMillis + 25 < millis())
	{
		RED.ht16k33.readKeys();
		static HT16K33Display::KeyPosition lastKeyPressed = {0xF, 0xF};
		HT16K33Display::KeyPosition keyPressed = RED.ht16k33.firstKeySet();
		bool change = (lastKeyPressed != keyPressed);
		lastKeyPressed = keyPressed;
		if (keyPressed.col != 0xF /*|| keyPressed.row != 0xF*/)
		{
			if (change)
			{
				uint8_t mappedKey = KEYMAPPING[keyPressed.row][keyPressed.col];
				Serial.print("Key pressed ");
				Serial.print(keyPressed.row);
				Serial.print(", ");
				Serial.print(keyPressed.col);
				Serial.print(" = ");
				Serial.println(mappedKey, 16);
				switch (mappedKey)
				{
				// GCC extension
				case 0 ... 9:
					if (inputbuffer != NULL && inputdigit < 12)
						(*inputbuffer)[inputdigit++] = mappedKey;
					break;
				case 0xA:
					if (inputbuffer != NULL)
					{
						memset(inputbuffer, 0, sizeof(*inputbuffer));
						overrides.set(inputbuffer, 0);
					}
					inputdigit = 0;
					inputbuffer = &RED.value;
					memset(inputbuffer, 0, sizeof(*inputbuffer));
					RED.mm5450.assignLedRange(2, 25, 5, 0x06);
					forceupdate = true;
					break;
				case 0xB:
					if (inputbuffer != NULL)
					{
						memset(inputbuffer, 0, sizeof(*inputbuffer));
						overrides.set(inputbuffer, 0);
					}
					inputdigit = 0;
					inputbuffer = &YELLOW.value;
					memset(inputbuffer, 0, sizeof(*inputbuffer));
					RED.mm5450.assignLedRange(2, 25, 5, 0x05);
					forceupdate = true;
					break;
				case 0xC:
					if (inputbuffer != NULL)
					{
						memset(inputbuffer, 0, sizeof(*inputbuffer));
						overrides.set(inputbuffer, 0);
					}
					inputdigit = 0;
					inputbuffer = &GREEN.value;
					memset(inputbuffer, 0, sizeof(*inputbuffer));
					RED.mm5450.assignLedRange(2, 25, 5, 0x3);
					forceupdate = true;
					break;
				case 0xD:
					if (inputdigit == 0)
					{
						if (inputbuffer != NULL)
						{
							memset(inputbuffer, 0, sizeof(*inputbuffer));
							overrides.set(inputbuffer, 0);
						}
						else if (overrides.current())
						{
#ifdef CURRENT_TIME_ON_RED
							set_system_time(parsetime(RED.value));
							overrides.red = 0;
#else
							set_system_time(parsetime(GREEN.value));
							overrides.green = 0;
#endif
						}
						inputbuffer = NULL;
						RED.mm5450.assignLedRange(2, 25, 5, overrides.current() ? 0x0F : 0x1F);
						forceupdate = true;
					}
					else
					{
						(*inputbuffer)[--inputdigit] = 0;
					}
					break;
				case 0xE:
					overrides.set(inputbuffer, 1);
					inputbuffer = NULL;
					inputdigit = 0;
					RED.mm5450.assignLedRange(2, 25, 5, overrides.current() ? 0x0F : 0x1F);
					forceupdate = true;
					break;
				}
			}
			lastPollMillis = millis();
		}
		else
		{
			polling = false;
		}
	}
	static time_t last_now = (time_t)-1;
	time_t now;
	time(&now);
	if (forceupdate || now != last_now)
	{
		if (inputbuffer == &RED.value)
		{
			RED.blank();
		}
		else if (overrides.red)
		{
			RED.writeTime();
		}
#ifdef CURRENT_TIME_ON_RED
		else
		{
			RED.writeTime(now);
		}
#endif

		if (inputbuffer == &GREEN.value)
		{
			GREEN.blank();
		}
		else if (overrides.green)
		{
			GREEN.writeTime();
		}
#ifndef CURRENT_TIME_ON_RED
		else
		{
			GREEN.writeTime(now);
		}
#endif

		if (inputbuffer == &YELLOW.value)
		{
			YELLOW.blank();
		}
		else if (overrides.yellow)
		{
			YELLOW.writeTime();
		}

		uint8_t colon = now & 1;
		RED.updateColon(colon);
		GREEN.updateColon(colon);
		YELLOW.updateColon(colon);
		last_now = now;
	}
}
