#ifndef DIVMOD_H__345248__
#define DIVMOD_H__345248__

template<typename T>
struct divmod_t
{
	T quot;
	T rem;
};

template<typename T>
inline divmod_t<T> divmod(T __num, T __denom) __ATTR_CONST__;
template<typename T>
inline divmod_t<T> divmod(T __num, T __denom)
{
	return {static_cast<T>(__num / __denom), static_cast<T>(__num % __denom)};
}

#if defined (__AVR__)
#define DIVMOD_SPECIALIZATION(type, asmname) template<> \
	divmod_t<type> divmod(type __num, type __denom) __asm__(#asmname) __ATTR_CONST__;

DIVMOD_SPECIALIZATION(signed char, __divmodqi4);
DIVMOD_SPECIALIZATION(unsigned char, __udivmodqi4)
DIVMOD_SPECIALIZATION(short, __divmodhi4)
DIVMOD_SPECIALIZATION(unsigned short, __udivmodhi4)
DIVMOD_SPECIALIZATION(int, __divmodhi4)
DIVMOD_SPECIALIZATION(unsigned int, __udivmodhi4)
DIVMOD_SPECIALIZATION(long, __divmodsi4)
DIVMOD_SPECIALIZATION(unsigned long, __udivmodsi4)
DIVMOD_SPECIALIZATION(long long, __divmoddi4)
DIVMOD_SPECIALIZATION(unsigned long long, __udivmoddi4)

#undef DIVMOD_SPECIALIZATION
#endif

#endif // DIVMOD_H__345248__