#include <Arduino.h>
#include <assert.h>
#include <SPI.h>

#include "mm5450.h"

MultiplexMM5450::MultiplexMM5450(uint8_t ss_pin)
	: m_ss_pin (ss_pin)
	            //0x400000001
	, m_banks { {1, 0, 0, 0, 4},
	            //0x200000001
	            {1, 0, 0, 0, 2},
	            //0x100000001
	            {1, 0, 0, 0, 1} }
{ }

void MultiplexMM5450::initialize()
{
	digitalWrite(m_ss_pin, HIGH);
	pinMode(m_ss_pin, OUTPUT);
}

void MultiplexMM5450::refreshBank(uint8_t bankno) const
{
	assert(bankno < 3);
	SPI.beginTransaction(SPISettings(500000, LSBFIRST, SPI_MODE0));
	digitalWrite(m_ss_pin, LOW);
	for (uint8_t i = 0; i < 5; ++i)
		SPI.transfer(m_banks[bankno][i]);
	digitalWrite(m_ss_pin, HIGH);
	SPI.endTransaction();
}
