#include <EEPROM.h>
#include <SPI.h>
#include <mm5450.h>
#include <Wire.h>
#include <ht16k33.h>

// This page intentionally left blank, due to crazy arduino preprocessor
// See mm5450_test.cpp for what should have been here

#include <time.h>
#include "compile_time.h"

// Tomohiko Sakamoto's algorithm for day of week, with slight stylistic tweaks for use in macro
#define GET_DOW(y,m,d) ((((y)-((m)<3))+((y)-((m)<3))/4-((y)-((m)<3))/100+((y)-((m)<3))/400+("-bed=pen+mad.")[(m)]+(d))%7)

#define IS_USA_DST ( \
    (__TIME_MONTH__ > 3 || (__TIME_MONTH__ == 3 && __TIME_DAYS__ > 8+(7-GET_DOW(__TIME_YEARS__,3,8))%7) || \
        (__TIME_MONTH__ == 3 && __TIME_DAYS__ == 8+(7-GET_DOW(__TIME_YEARS__,3,8))%7 && __TIME_HOURS__ >= 2)) && \
    (__TIME_MONTH__ < 11 || (__TIME_MONTH__ == 11 && __TIME_DAYS__ < 1+(7-GET_DOW(__TIME_YEARS__,11,1))%7) || \
        (__TIME_MONTH__ == 11 && __TIME_DAYS__ == 1+(7-GET_DOW(__TIME_YEARS__,11,1))%7 && __TIME_HOURS__ < 2)) \
)

#define EST5EDT -5
#define CST6CDT -6
#define MST7MDT -7
#define PST8PDT -8

#define TIMEZONE_OFFSET_SECS ((PST8PDT + IS_USA_DST) * SEC_PER_HOUR)

extern const uint32_t INITIAL_TIME PROGMEM;
const uint32_t INITIAL_TIME PROGMEM = UNIX_TIMESTAMP - TIMEZONE_OFFSET_SECS - UNIX_OFFSET;
