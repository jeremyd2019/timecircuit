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

MultiplexMM5450 RED(9), GREEN(8), YELLOW(7);

HT16K33QuadAlphanum RED_MONTH = HT16K33QuadAlphanum(0x70),
                    GREEN_MONTH = HT16K33QuadAlphanum(0x71),
                    YELLOW_MONTH = HT16K33QuadAlphanum(0x72);

// TODO current time should be on GREEN, not RED, but I only built RED...
#define CURRENT_TIME_ON_RED
#ifdef CURRENT_TIME_ON_RED
#define CURRENT_TIME_OVERRIDE overrides.redoverride
#else
#define CURRENT_TIME_OVERRIDE overrides.greenoverride
#endif

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

static void setupDisplay(MultiplexMM5450 & color, HT16K33QuadAlphanum & monthdisplay)
{
	color.initialize();
	monthdisplay.systemSetup(true);
	monthdisplay.rowIntSet(true, true);
	monthdisplay.dimmingSet(5);
	// nice all-LEDs-on test pattern
	for (uint8_t i = 0; i < 4; ++i)
		monthdisplay.writeDigitRaw(i, 0xFFFF);
	for (uint8_t i = 0; i < 3; ++i)
		color.assignLedRange(i, 1, 30, 0x3FFFFFFF);
	monthdisplay.writeDisplay();
	monthdisplay.displaySetup(true);
}

static void clearDisplay(MultiplexMM5450 & color, HT16K33QuadAlphanum & monthdisplay, uint8_t & last_month)
{
	color.assignLedRange(0, 1, 30, 0);
	color.assignLedRange(1, 2, 28, 0);
	color.assignLedRange(2, 10, 14, 0);
	if (last_month != 254)
	{
		monthdisplay.clear();
		monthdisplay.writeDisplay();
		last_month = 254;
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
	setupDisplay(RED, RED_MONTH);
	setupDisplay(GREEN, GREEN_MONTH);
	setupDisplay(YELLOW, YELLOW_MONTH);
	Serial.begin(115200);
}

static const char MONTHS[12][3] PROGMEM = {
	{'J','A','N'}, {'F','E','B'}, {'M','A','R'},
	{'A','P','R'}, {'M','A','Y'}, {'J','U','N'},
	{'J','U','L'}, {'A','U','G'}, {'S','E','P'},
	{'O','C','T'}, {'N','O','V'}, {'D','E','C'}
};

typedef SubByteArray<4, uint8_t, 12> TimeDisplay_t;

static void writeTime(MultiplexMM5450 & color, HT16K33QuadAlphanum & monthdisplay, uint8_t & last_month, const TimeDisplay_t & value, uint8_t month = -1, uint8_t hour = -1)
{
	static const uint8_t unspecified = -1;
	if (hour == unspecified)
		hour = value[8] * 10 + value[9];
	uint8_t twelvehour = hour % 12;
	if (twelvehour == 0)
		twelvehour = 12;
	bool pm = hour >= 12;
	writeDigit(color, 0, 0, value[11]);
	writeDigit(color, 0, 1, value[10]);
	writeDigit(color, 0, 2, twelvehour % 10);
	writeDigit(color, 0, 3, (twelvehour / 10) % 10);
	color.assignLed(0, 1, !pm);
	color.assignLed(0, 30, pm);

	// TODO: BC flag?  Year 0? (movie shows DEC 25 0000 as birth-of-christ)
	writeDigit(color, 1, 0, value[7]);
	writeDigit(color, 1, 1, value[6]);
	writeDigit(color, 1, 2, value[5]);
	writeDigit(color, 1, 3, value[4]);

	writeDigit(color, 2, 0, value[3]);
	writeDigit(color, 2, 1, value[2]);

	if (month == unspecified)
		month = static_cast<uint8_t>(value[0] * 10 + value[1] - 1) % 12;
	if (last_month != month)
	{
		const char * monthabbr = MONTHS[month];
		monthdisplay.writeDigitRaw(0, 0x0);
		monthdisplay.writeDigitAscii(1, pgm_read_byte_func(&monthabbr[0]));
		monthdisplay.writeDigitAscii(2, pgm_read_byte_func(&monthabbr[1]));
		monthdisplay.writeDigitAscii(3, pgm_read_byte_func(&monthabbr[2]));
		monthdisplay.writeDisplay();
		last_month = month;
	}
}

static void writeTime(MultiplexMM5450 & color, HT16K33QuadAlphanum & monthdisplay, uint8_t & last_month, TimeDisplay_t & value, time_t timeval)
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

	writeTime(color, monthdisplay, last_month, value, curtm->tm_mon, curtm->tm_hour);
}

TimeDisplay_t redvalue = {0}, greenvalue = {0}, yellowvalue = {0};

struct
{
	uint8_t redoverride:1;
	uint8_t greenoverride:1;
	uint8_t yellowoverride:1;
} overrides = {0};

static inline void setOverride(const TimeDisplay_t * color, uint8_t value)
{
	if (color == &redvalue)
		overrides.redoverride = value;
	else if (color == &greenvalue)
		overrides.greenoverride = value;
	else if (color == &yellowvalue)
		overrides.yellowoverride = value;
}


void loop() {
	// put your main code here, to run repeatedly:
	MultiplexMM5450::process({&RED, &GREEN, &YELLOW});
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
		RED_MONTH.readKeys();
		static HT16K33Display::KeyPosition lastKeyPressed = {0xF, 0xF};
		HT16K33Display::KeyPosition keyPressed = RED_MONTH.firstKeySet();
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
						setOverride(inputbuffer, 0);
					}
					inputdigit = 0;
					inputbuffer = &redvalue;
					memset(inputbuffer, 0, sizeof(*inputbuffer));
					RED.assignLedRange(2, 25, 5, 0x06);
					forceupdate = true;
					break;
				case 0xB:
					if (inputbuffer != NULL)
					{
						memset(inputbuffer, 0, sizeof(*inputbuffer));
						setOverride(inputbuffer, 0);
					}
					inputdigit = 0;
					inputbuffer = &yellowvalue;
					memset(inputbuffer, 0, sizeof(*inputbuffer));
					RED.assignLedRange(2, 25, 5, 0x05);
					forceupdate = true;
					break;
				case 0xC:
					if (inputbuffer != NULL)
					{
						memset(inputbuffer, 0, sizeof(*inputbuffer));
						setOverride(inputbuffer, 0);
					}
					inputdigit = 0;
					inputbuffer = &greenvalue;
					memset(inputbuffer, 0, sizeof(*inputbuffer));
					RED.assignLedRange(2, 25, 5, 0x3);
					forceupdate = true;
					break;
				case 0xD:
					if (inputdigit == 0)
					{
						if (inputbuffer != NULL)
						{
							memset(inputbuffer, 0, sizeof(*inputbuffer));
							setOverride(inputbuffer, 0);
						}
						inputbuffer = NULL;
						RED.assignLedRange(2, 25, 5, CURRENT_TIME_OVERRIDE ? 0x0F : 0x1F);
						forceupdate = true;
					}
					else
					{
						(*inputbuffer)[--inputdigit] = 0;
					}
					break;
				case 0xE:
					setOverride(inputbuffer, 1);
					inputbuffer = NULL;
					inputdigit = 0;
					RED.assignLedRange(2, 25, 5, CURRENT_TIME_OVERRIDE ? 0x0F : 0x1F);
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
	static uint8_t red_last_month = -1, green_last_month = -1, yellow_last_month = -1;
	static time_t last_now = (time_t)-1;
	time_t now;
	time(&now);
	if (forceupdate || now != last_now)
	{
		if (inputbuffer == &redvalue)
		{
			clearDisplay(RED, RED_MONTH, red_last_month);
		}
		else if (overrides.redoverride)
		{
			writeTime(RED, RED_MONTH, red_last_month, redvalue);
		}
#ifdef CURRENT_TIME_ON_RED
		else
		{
			writeTime(RED, RED_MONTH, red_last_month, redvalue, now);
		}
#endif

		if (inputbuffer == &greenvalue)
		{
			clearDisplay(GREEN, GREEN_MONTH, green_last_month);
		}
		else if (overrides.greenoverride)
		{
			writeTime(GREEN, GREEN_MONTH, green_last_month, greenvalue);
		}
#ifndef CURRENT_TIME_ON_RED
		else
		{
			writeTime(GREEN, GREEN_MONTH, green_last_month, greenvalue, now);
		}
#endif

		if (inputbuffer == &yellowvalue)
		{
			clearDisplay(YELLOW, YELLOW_MONTH, yellow_last_month);
		}
		else if (overrides.yellowoverride)
		{
			writeTime(YELLOW, YELLOW_MONTH, yellow_last_month, yellowvalue);
		}

		uint8_t colon = now & 1;
		RED.assignLed(2,  1, colon);
		RED.assignLed(2, 30, colon);
		GREEN.assignLed(2,  1, colon);
		GREEN.assignLed(2, 30, colon);
		YELLOW.assignLed(2,  1, colon);
		YELLOW.assignLed(2, 30, colon);
		last_now = now;
	}
}
