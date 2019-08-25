// seven_seg.h

#ifndef _SEVEN_SEG_h
#define _SEVEN_SEG_h
#include <mm5450.h>

class MultiplexMM5450SevenSeg : public MultiplexMM5450
{
public:
    inline MultiplexMM5450SevenSeg(uint8_t ss_pin)
        : MultiplexMM5450(ss_pin)
    { }

    void writeDigit(uint8_t bankno, uint8_t digitno, uint8_t digit);
};

#endif
