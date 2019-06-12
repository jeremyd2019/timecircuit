#ifndef MM5450_H_354624
#define MM5450_H_354624
#include <assert.h>

class MultiplexMM5450
{
public:
	MultiplexMM5450(uint8_t ss_pin);

	void initialize();
	void refreshBank(uint8_t bankno) const;

	template<uint8_t N, uint8_t DELAYTIME = 3>
	static void process(const MultiplexMM5450 * const (&chips)[N])
	{
		static uint16_t lastUpdate = 0 - DELAYTIME - 1;
		static uint8_t stage = 0;
		uint16_t curmillis = static_cast<uint16_t> (millis());
		if ((curmillis - lastUpdate) >= DELAYTIME)
		{
			for (uint8_t i = 0; i < N; ++i)
				chips[i]->refreshBank(stage);
			lastUpdate = curmillis;
			if (++stage >= 3)
				stage = 0;
		}
	}

	inline void setLed(uint8_t bankno, uint8_t ledno)
	{
		assert(bankno < 3 && ledno > 0 && ledno < 32);
		m_banks[bankno][ledno >> 3] |= (1 << (ledno&7));
	}

	inline void clearLed(uint8_t bankno, uint8_t ledno)
	{
		assert(bankno < 3 && ledno > 0 && ledno < 32);
		m_banks[bankno][ledno >> 3] &= ~(1 << (ledno&7));
	}

	inline void toggleLed(uint8_t bankno, uint8_t ledno)
	{
		assert(bankno < 3 && ledno > 0 && ledno < 32);
		m_banks[bankno][ledno >> 3] ^= (1 << (ledno&7));
	}

	inline void assignLed(uint8_t bankno, uint8_t ledno, uint8_t value)
	{
		assert(bankno < 3 && ledno > 0 && ledno < 32);
		m_banks[bankno][ledno >> 3] = (m_banks[bankno][ledno >> 3] & ~(1 << (ledno&7))) | ((!!value) << (ledno&7));
	}

	inline void assignLedRange(uint8_t bankno, uint8_t start, uint8_t len, uint32_t value)
	{
		assert(bankno < 3 && start > 0 && (start+len) <= 32);
		const uint32_t mask = (1 << len) - 1;
		m_banks[bankno].b = (m_banks[bankno].b & ~(mask << start)) | (value & mask) << start;
	}

private:
	union LEDBank
	{
		uint8_t a[5];
		uint32_t b;
		inline uint8_t & operator[] (const size_t i)
		{
			return a[i];
		}
		inline const uint8_t & operator[] (const size_t i) const
		{
			return a[i];
		}
	};
	LEDBank m_banks[3];
	const uint8_t m_ss_pin;
};

#endif // MM5450_H_354624
