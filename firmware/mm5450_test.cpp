#include <Arduino.h>
#include <assert.h>
#include <SPI.h>

#include "mm5450.h"

MultiplexMM5450 RED(9), YELLOW(8), GREEN(7);

static const unsigned long DELAYTIME = 3;
void myDoStuff()
{
	static unsigned long lastUpdate = 0 - DELAYTIME - 1;
	static uint8_t stage = 0;
	if ((millis() - lastUpdate) >= DELAYTIME)
	{
		RED.refreshBank(stage);
		YELLOW.refreshBank(stage);
		GREEN.refreshBank(stage);
		lastUpdate = millis();
		if (++stage >= 3)
			stage = 0;
	}
}

void setup() {
	// put your setup code here, to run once:
	SPI.begin();
	RED.initialize();
	YELLOW.initialize();
	GREEN.initialize();
#ifdef CURRENT_TEST
	RED.setLed(0, 1);
	RED.setLed(1, 1);
	RED.setLed(2, 1);
#endif
}

void loop() {
	// put your main code here, to run repeatedly:
	myDoStuff();
	uint16_t frob = (millis() / 1000) & 0x1fff;
	RED.assignLed(0, 31, frob & 1);
	RED.assignLed(1, 31, frob & 2);
	RED.assignLed(2, 31, frob & 4);
	RED.assignLedRange(0, 18, 13, frob);
}
