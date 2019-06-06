#include <SPI.h>

// This page intentionally left blank, due to crazy arduino preprocessor
// See mm5450_test.cpp for what should have been here

#define __TIMEZONE_OFFSET_SECS__ (-7*3600)
#include "compile_time.h"
extern const uint32_t INITIAL_TIME PROGMEM;
const uint32_t INITIAL_TIME PROGMEM = UNIX_TIMESTAMP;
