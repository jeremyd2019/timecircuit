#ifndef ARDUINO_UTILS_H_94352098__
#define ARDUINO_UTILS_H_94352098__

inline uint8_t pgm_read_byte_func(const void * p)
{
	return pgm_read_byte(p);
}

inline uint16_t pgm_read_word_func(const void * p)
{
    return pgm_read_word(p);
}

inline uint32_t pgm_read_dword_func(const void * p)
{
	return pgm_read_dword(p);
}


#endif // ARDUINO_UTILS_H_94352098__
