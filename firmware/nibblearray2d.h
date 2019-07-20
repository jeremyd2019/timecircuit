#ifndef NIBBLE_ARRAY_2D_H_325345__
#define NIBBLE_ARRAY_2D_H_325345__

#include <stdint.h>

template<typename SizeType, SizeType Rows, SizeType Cols, bool ProgMem = false>
class NibbleArray2D
{
public: // Would be private, but seems to screw up initializing member(s) with = {...}
	struct ArrayMember
	{
		uint8_t a:4;
		uint8_t b:4;
	};

	/* Returns the first integer greater or equal to num/denom */
	static constexpr SizeType integer_ceil_division(SizeType num, SizeType denom)
	{
		return (num > 0) ? (num - 1) / denom + 1 : 0;
	}

	ArrayMember m_array[integer_ceil_division(Rows*Cols,2)];

public:
	template<bool RowProgMem, uint8_t Garbage = 42>
	class Row
	{
		typedef NibbleArray2D<SizeType, Rows, Cols, RowProgMem> OuterClass;
		friend Row<RowProgMem> OuterClass::operator[] (SizeType) const;

		const OuterClass * outer;
		const SizeType idx;

		Row(const OuterClass * that, SizeType idx)
			: outer (that)
			, idx (idx)
		{ }

	public:
		inline uint8_t operator[] (SizeType col) const
		{
			const SizeType i = idx + col;
			const ArrayMember & m = outer->m_array[i>>1];
			return (i&1) ? m.b : m.a;
		}
	};

#ifdef pgm_read_byte
	template<uint8_t Garbage>
	class Row<true, Garbage>
	{
		typedef NibbleArray2D<SizeType, Rows, Cols, true> OuterClass;
		friend Row<true> OuterClass::operator[] (SizeType) const;

		const OuterClass * outer;
		const SizeType idx;

		Row(const OuterClass * that, SizeType idx)
			: outer (that)
			, idx (idx)
		{ }

	public:
		inline uint8_t operator[] (SizeType col) const
		{
			const SizeType i = idx + col;
			const union
			{
				uint8_t x;
				ArrayMember m;
			} x = {pgm_read_byte(&outer->m_array[i>>1])};
			return (i&1) ? x.m.b : x.m.a;
		}
	};
#endif

	inline Row<ProgMem> operator[] (SizeType row) const
	{
		return Row<ProgMem>(this, row * Cols);
	}
};


#endif // NIBBLE_ARRAY_2D_H_325345__