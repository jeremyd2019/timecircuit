#include <SPI.h>

// This page intentionally left blank, due to crazy arduino preprocessor
// See mm5450_test.cpp for what should have been here

#include <time.h>
#include "compile_time.h"

// rough estimate of DST
#define TIMEZONE_OFFSET_SECS (((__TIME_MONTH__ < 3 || __TIME_MONTH__ >= 11) ? -8 : -7) * SEC_PER_HOUR)
extern const uint32_t INITIAL_TIME PROGMEM;
const uint32_t INITIAL_TIME PROGMEM = UNIX_TIMESTAMP - TIMEZONE_OFFSET_SECS - UNIX_OFFSET;
