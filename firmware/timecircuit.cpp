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

#include "subbytearray.h"

ISR(TIMER1_OVF_vect, ISR_NAKED)
{
	system_tick();
	reti();
}

static inline uint8_t pgm_read_byte_func(const void * p)
{
	return pgm_read_byte(p);
}

static const SubByteArray2D<4, uint8_t, 5, 3, pgm_read_byte_func> KEYMAPPING PROGMEM = {{
	0x1, 0x2, 0x3,
	0x4, 0x5, 0x6,
	0x7, 0x8, 0x9,
	0xD, 0x0, 0xE,
	0xA, 0xB, 0xC,
	0xF // to fill out the even number of elements
}};

MultiplexMM5450 RED(9), YELLOW(8), GREEN(7);

HT16K33QuadAlphanum RED_MONTH = HT16K33QuadAlphanum(0x70);
HT16K33QuadAlphanum YELLOW_MONTH = HT16K33QuadAlphanum(0x71);
HT16K33QuadAlphanum GREEN_MONTH = HT16K33QuadAlphanum(0x72);

// TODO current time should be on GREEN, not RED, but I only built RED...
#define CURRENT_TIME_ON_RED

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
	Wire.begin();
	pinMode(3, INPUT);
	attachInterrupt(1, onPin3Rising, RISING);
	setupDisplay(RED, RED_MONTH);
	setupDisplay(YELLOW, YELLOW_MONTH);
	setupDisplay(GREEN, GREEN_MONTH);
	Serial.begin(115200);
}

static const char MONTHS[12][3] PROGMEM = {
	{'J','A','N'}, {'F','E','B'}, {'M','A','R'},
	{'A','P','R'}, {'M','A','Y'}, {'J','U','N'},
	{'J','U','L'}, {'A','U','G'}, {'S','E','P'},
	{'O','C','T'}, {'N','O','V'}, {'D','E','C'}
};

struct TimeOverride
{
	int16_t year;
	uint16_t override:1;
	uint16_t month:4;
	uint16_t day:5;
	uint16_t hour:5;
	uint8_t minute:6;
};

static void writeTime(MultiplexMM5450 & color, HT16K33QuadAlphanum & monthdisplay, uint8_t & last_month, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute)
{
	uint8_t twelvehour = hour % 12;
	if (twelvehour == 0)
		twelvehour = 12;
	bool pm = hour >= 12;
	divmod_t<uint8_t> tmp = divmod<uint8_t>(minute, 10);
	writeDigit(color, 0, 0, tmp.rem);
	tmp = divmod<uint8_t>(tmp.quot, 10);
	writeDigit(color, 0, 1, tmp.rem);
	writeDigit(color, 0, 2, twelvehour % 10);
	writeDigit(color, 0, 3, (twelvehour / 10) % 10);
	color.assignLed(0, 1, !pm);
	color.assignLed(0, 30, pm);

	// TODO: BC flag?  Year 0? (movie shows DEC 25 0000 as birth-of-christ)
	if (static_cast<int16_t>(year) < 0)
		year = -year;
	divmod_t<uint16_t> tmp2 = divmod<uint16_t>(year, 10);
	writeDigit(color, 1, 0, tmp2.rem);
	tmp2 = divmod<uint16_t>(tmp2.quot, 10);
	writeDigit(color, 1, 1, tmp2.rem);
	tmp2 = divmod<uint16_t>(tmp2.quot, 10);
	writeDigit(color, 1, 2, tmp2.rem);
	tmp2 = divmod<uint16_t>(tmp2.quot, 10);
	writeDigit(color, 1, 3, tmp2.rem);

	tmp = divmod<uint8_t>(day, 10);
	writeDigit(color, 2, 0, tmp.rem);
	tmp = divmod<uint8_t>(tmp.quot, 10);
	writeDigit(color, 2, 1, tmp.rem);

	if (last_month != month)
	{
		const char * monthabbr = MONTHS[month];
		monthdisplay.writeDigitRaw(0, 0x0);
		monthdisplay.writeDigitAscii(1, pgm_read_byte(&monthabbr[0]));
		monthdisplay.writeDigitAscii(2, pgm_read_byte(&monthabbr[1]));
		monthdisplay.writeDigitAscii(3, pgm_read_byte(&monthabbr[2]));
		monthdisplay.writeDisplay();
		last_month = month;
	}
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
	static TimeOverride redoverride = {0}, yellowoverride = {0}, greenoverride = {0};
	static TimeOverride * inputoverride = NULL;
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
					if (inputoverride != NULL)
					{
						switch (inputdigit)
						{
						case 0:
							inputoverride->month = mappedKey * 10;
							break;
						case 1:
							inputoverride->month += mappedKey;
							break;
						case 2:
							inputoverride->day = mappedKey * 10;
							break;
						case 3:
							inputoverride->day += mappedKey;
							break;
						case 4:
							inputoverride->year = mappedKey * 1000;
							break;
						case 5:
							inputoverride->year += mappedKey * 100;
							break;
						case 6:
							inputoverride->year += mappedKey * 10;
							break;
						case 7:
							inputoverride->year += mappedKey;
							break;
						case 8:
							inputoverride->hour = mappedKey * 10;
							break;
						case 9:
							inputoverride->hour += mappedKey;
							break;
						case 10:
							inputoverride->minute = mappedKey * 10;
							break;
						case 11:
							inputoverride->minute += mappedKey;
							break;
						}
						if (inputdigit < 12)
							++inputdigit;
					}
					break;
				case 0xA:
					if (inputoverride != NULL)
						memset(inputoverride, 0, sizeof(*inputoverride));
					inputdigit = 0;
					inputoverride = &redoverride;
					memset(inputoverride, 0, sizeof(*inputoverride));
					inputoverride->override = 1;
					RED.assignLedRange(2, 25, 5, 0x06);
					forceupdate = true;
					break;
				case 0xB:
					if (inputoverride != NULL)
						memset(inputoverride, 0, sizeof(*inputoverride));
					inputdigit = 0;
					inputoverride = &yellowoverride;
					memset(inputoverride, 0, sizeof(*inputoverride));
					inputoverride->override = 1;
					RED.assignLedRange(2, 25, 5, 0x05);
					forceupdate = true;
					break;
				case 0xC:
					if (inputoverride != NULL)
						memset(inputoverride, 0, sizeof(*inputoverride));
					inputdigit = 0;
					inputoverride = &greenoverride;
					memset(inputoverride, 0, sizeof(*inputoverride));
					inputoverride->override = 1;
					RED.assignLedRange(2, 25, 5, 0x3);
					forceupdate = true;
					break;
				case 0xD:
					switch (inputdigit)
					{
					case 0:
						if (inputoverride != NULL)
							memset(inputoverride, 0, sizeof(*inputoverride));
						inputoverride = NULL;
						RED.assignLedRange(2, 25, 5, redoverride.override ? 0x0F : 0x1F);
						forceupdate = true;
						break;
					case 1:
						inputoverride->month = (inputoverride->month / 10) * 10;
						break;
					case 2:
						inputoverride->day = 0;
						break;
					case 3:
						inputoverride->day = (inputoverride->day / 10) * 10;
						break;
					case 4:
						inputoverride->year = 0;
						break;
					case 5:
						inputoverride->year = (inputoverride->year / 1000) * 1000;
						break;
					case 6:
						inputoverride->year = (inputoverride->year / 100) * 100;
						break;
					case 7:
						inputoverride->year = (inputoverride->year / 10) * 10;
						break;
					case 8:
						inputoverride->hour = 0;
						break;
					case 9:
						inputoverride->hour = (inputoverride->hour / 10) * 10;
						break;
					case 10:
						inputoverride->minute = 0;
						break;
					case 11:
						inputoverride->minute = (inputoverride->minute / 10) * 10;
						break;
					}
					if (inputdigit > 0)
						--inputdigit;
					break;
				case 0xE:
					// NOTE this is the only 'input validation', and then only because out-of-range months crash
					if (inputoverride != NULL)
						inputoverride->month = static_cast<uint8_t>(inputoverride->month - 1) % 12 + 1;
					inputoverride = NULL;
					inputdigit = 0;
					RED.assignLedRange(2, 25, 5, redoverride.override ? 0x0F : 0x1F);
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
	static uint8_t red_last_month = -1, yellow_last_month = -1, green_last_month = -1;
	static time_t last_now = (time_t)-1;
	time_t now;
	time(&now);
	if (forceupdate || now != last_now)
	{
		if (inputoverride == &redoverride)
		{
			clearDisplay(RED, RED_MONTH, red_last_month);
		}
		else if (redoverride.override)
		{
			writeTime(RED, RED_MONTH, red_last_month, redoverride.year, redoverride.month-1, redoverride.day, redoverride.hour, redoverride.minute);
		}
#ifdef CURRENT_TIME_ON_RED
		else
		{
			struct tm * curtm = localtime(&now);
#ifdef FULL_YEAR_RANGE
			uint16_t year = (curtm->tm_year >= -1900) ? curtm->tm_year + 1900 : -(curtm->tm_year + 1900);
#else
			// doesn't even handle year 10k with only 4 digits, so ignore overflow
			uint16_t year = curtm->tm_year + 1900;
#endif
			writeTime(RED, RED_MONTH, red_last_month, year, curtm->tm_mon, curtm->tm_mday, curtm->tm_hour, curtm->tm_min);
		}
#endif

		if (inputoverride == &yellowoverride)
		{
			clearDisplay(YELLOW, YELLOW_MONTH, yellow_last_month);
		}
		else if (yellowoverride.override)
		{
			writeTime(YELLOW, YELLOW_MONTH, yellow_last_month, yellowoverride.year, yellowoverride.month-1, yellowoverride.day, yellowoverride.hour, yellowoverride.minute);
		}

		if (inputoverride == &greenoverride)
		{
			clearDisplay(GREEN, GREEN_MONTH, green_last_month);
		}
		else if (greenoverride.override)
		{
			writeTime(GREEN, GREEN_MONTH, green_last_month, greenoverride.year, greenoverride.month-1, greenoverride.day, greenoverride.hour, greenoverride.minute);
		}
#ifndef CURRENT_TIME_ON_RED
		else
		{
			struct tm * curtm = localtime(&now);
#ifdef FULL_YEAR_RANGE
			uint16_t year = (curtm->tm_year >= -1900) ? curtm->tm_year + 1900 : -(curtm->tm_year + 1900);
#else
			// doesn't even handle year 10k with only 4 digits, so ignore overflow
			uint16_t year = curtm->tm_year + 1900;
#endif
			writeTime(GREEN, GREEN_MONTH, green_last_month, year, curtm->tm_mon, curtm->tm_mday, curtm->tm_hour, curtm->tm_min);
		}
#endif

		uint8_t colon = now & 1;
		RED.assignLed(2,  1, colon);
		RED.assignLed(2, 30, colon);
		YELLOW.assignLed(2,  1, colon);
		YELLOW.assignLed(2, 30, colon);
		GREEN.assignLed(2,  1, colon);
		GREEN.assignLed(2, 30, colon);
		last_now = now;
	}
}
