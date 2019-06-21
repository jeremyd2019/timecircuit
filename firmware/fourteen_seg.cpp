#include <Arduino.h>
#include <ht16k33.h>
#include "fourteen_seg.h"

// Font compiled from both
// https://github.com/adafruit/Adafruit_LED_Backpack/blob/master/Adafruit_LEDBackpack.cpp
// AND
// https://github.com/dmadison/LED-Segment-ASCII/blob/master/14-Segment/14-Segment-ASCII_HEX.txt
// with some slight tweaking (For instance, both G1 and G2 on in F)
static const uint16_t UPPERCASE_LETTER_FONT[] PROGMEM = {
    0x00F7, // A
    0x128F, // B
    0x0039, // C
    0x120F, // D
    0x00F9, // E
    0x00F1, // F
    0x00BD, // G
    0x00F6, // H
    0x1209, // I
    0x001E, // J
    0x2470, // K
    0x0038, // L
    0x0536, // M
    0x2136, // N
    0x003F, // O
    0x00F3, // P
    0x203F, // Q
    0x20F3, // R
    0x00ED, // S
    0x1201, // T
    0x003E, // U
    0x0C30, // V
    0x2836, // W
    0x2D00, // X
#if defined(FUNKY_Y_1)
    0x00EE, // Y
#elif defined(FUNKY_Y_2)
    0x10E2, // Y
#else
    0x1500, // Y
#endif
    0x0C09  // Z
};

void HT16K33QuadAlphanum::writeDigitAscii(uint8_t n, uint8_t character)
{
    assert(character >= 'A' && character <= 'Z');
    writeDigitRaw(n, pgm_read_word(&UPPERCASE_LETTER_FONT[character-'A']));
}
