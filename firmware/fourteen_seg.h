// fourteen_seg.h

#ifndef _FOURTEEN_SEG_h
#define _FOURTEEN_SEG_h
#include <ht16k33.h>

class HT16K33QuadAlphanum : public HT16K33Display
{
public:
    inline HT16K33QuadAlphanum(uint8_t addr)
        : HT16K33Display (addr)
    { }

    void writeDigitAscii(uint8_t n, uint8_t character);
};

#endif