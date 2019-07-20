#ifndef SUB_BYTE_ARRAY_H_49836739__
#define SUB_BYTE_ARRAY_H_49836739__

#include <stdint.h>

template<uint8_t BitsPerElt, typename SizeType, SizeType Elts, uint8_t (*ReadFunc)(const void *) = nullptr>
class SubByteArray
{
    template<uint8_t _BitsPerElt, typename _SizeType, _SizeType _Elts, uint8_t (*_ReadFunc)(const void *)>
    friend class SubByteArray;

    /* Returns the first integer greater or equal to num/denom */
    static constexpr SizeType integer_ceil_division(SizeType num, SizeType denom)
    {
        return (num > 0) ? (num - 1) / denom + 1 : 0;
    }
};

template<typename SizeType, SizeType Elts, uint8_t (*ReadFunc)(const void *)>
class SubByteArray<1, SizeType, Elts, ReadFunc>
{
public: // Would be private, but seems to screw up initializing member(s) with = {...}
    struct ArrayMember
    {
        uint8_t a:1;
        uint8_t b:1;
        uint8_t c:1;
        uint8_t d:1;
        uint8_t e:1;
        uint8_t f:1;
        uint8_t g:1;
        uint8_t h:1;
    };

    template<uint8_t (*_ReadFunc)(const void *), uint8_t Garbage = 42>
    class MemberReader
    {
        typedef SubByteArray<1, SizeType, Elts, _ReadFunc> OuterClass;
    public:
        static inline uint8_t read_member(const OuterClass * outer, SizeType i)
        {
            const union
            {
                uint8_t x;
                ArrayMember m;
            } x = {_ReadFunc(&outer->m_array[i>>3])};
            switch (i & 7)
            {
            case 0:
                return x.m.a;
            case 1:
                return x.m.b;
            case 2:
                return x.m.c;
            case 3:
                return x.m.d;
            case 4:
                return x.m.e;
            case 5:
                return x.m.f;
            case 6:
                return x.m.g;
            case 7:
            default:
                return x.m.h;
            }
        }
    };

    template<uint8_t Garbage>
    class MemberReader<nullptr, Garbage>
    {
        typedef SubByteArray<1, SizeType, Elts, nullptr> OuterClass;
    public:
        static inline uint8_t read_member(const OuterClass * outer, SizeType i)
        {
            const ArrayMember & m = outer->m_array[i>>3];
            switch (i & 7)
            {
            case 0:
                return m.a;
            case 1:
                return m.b;
            case 2:
                return m.c;
            case 3:
                return m.d;
            case 4:
                return m.e;
            case 5:
                return m.f;
            case 6:
                return m.g;
            case 7:
            default:
                return m.h;
            }
        }
    };

    ArrayMember m_array[SubByteArray<99, SizeType, 1>::integer_ceil_division(Elts,8/1)];

public:
    class ElementRef
    {
        typedef SubByteArray<1, SizeType, Elts, ReadFunc> OuterClass;
        friend ElementRef OuterClass::operator[] (SizeType);
        OuterClass * outer;
        SizeType i;
    public:
        inline ElementRef(OuterClass * outer, SizeType i)
            : outer (outer)
            , i (i)
        { }

        inline ElementRef & operator= (const uint8_t & rhs)
        {
            ArrayMember & m = outer->m_array[i>>3];
            switch (i & 7)
            {
            case 0:
                m.a = rhs;
                break;
            case 1:
                m.b = rhs;
                break;
            case 2:
                m.c = rhs;
                break;
            case 3:
                m.d = rhs;
                break;
            case 4:
                m.e = rhs;
                break;
            case 5:
                m.f = rhs;
                break;
            case 6:
                m.g = rhs;
                break;
            case 7:
                m.h = rhs;
                break;
            }
            return *this;
        }

        inline operator uint8_t() const
        {
            return MemberReader<ReadFunc>::read_member(outer, i);
        }
    };

    inline ElementRef operator[] (SizeType i)
    {
        return ElementRef(this, i);
    }

    inline uint8_t operator[] (SizeType i) const
    {
        return MemberReader<ReadFunc>::read_member(this, i);
    }
};

template<typename SizeType, SizeType Elts, uint8_t (*ReadFunc)(const void *)>
class SubByteArray<2, SizeType, Elts, ReadFunc>
{
public: // Would be private, but seems to screw up initializing member(s) with = {...}
    struct ArrayMember
    {
        uint8_t a:2;
        uint8_t b:2;
        uint8_t c:2;
        uint8_t d:2;
    };

    template<uint8_t (*_ReadFunc)(const void *), uint8_t Garbage = 42>
    class MemberReader
    {
        typedef SubByteArray<2, SizeType, Elts, _ReadFunc> OuterClass;
    public:
        static inline uint8_t read_member(const OuterClass * outer, SizeType i)
        {
            const union
            {
                uint8_t x;
                ArrayMember m;
            } x = {_ReadFunc(&outer->m_array[i>>2])};
            switch (i & 3)
            {
            case 0:
                return x.m.a;
            case 1:
                return x.m.b;
            case 2:
                return x.m.c;
            case 3:
            default:
                return x.m.d;
            }
        }
    };

    template<uint8_t Garbage>
    class MemberReader<nullptr, Garbage>
    {
        typedef SubByteArray<2, SizeType, Elts, nullptr> OuterClass;
    public:
        static inline uint8_t read_member(const OuterClass * outer, SizeType i)
        {
            const ArrayMember & m = outer->m_array[i>>2];
            switch (i & 3)
            {
            case 0:
                return m.a;
            case 1:
                return m.b;
            case 2:
                return m.c;
            case 3:
            default:
                return m.d;
            }
        }
    };

    ArrayMember m_array[SubByteArray<99, SizeType, 1>::integer_ceil_division(Elts,8/2)];

public:
    class ElementRef
    {
        typedef SubByteArray<2, SizeType, Elts, ReadFunc> OuterClass;
        friend ElementRef OuterClass::operator[] (SizeType);
        OuterClass * outer;
        SizeType i;
    public:
        inline ElementRef(OuterClass * outer, SizeType i)
            : outer (outer)
            , i (i)
        { }

        inline ElementRef & operator= (const uint8_t & rhs)
        {
            ArrayMember & m = outer->m_array[i>>2];
            switch (i & 3)
            {
            case 0:
                m.a = rhs;
                break;
            case 1:
                m.b = rhs;
                break;
            case 2:
                m.c = rhs;
                break;
            case 3:
                m.d = rhs;
                break;
            }
            return *this;
        }

        inline operator uint8_t() const
        {
            return MemberReader<ReadFunc>::read_member(outer, i);
        }
    };

    inline ElementRef operator[] (SizeType i)
    {
        return ElementRef(this, i);
    }

    inline uint8_t operator[] (SizeType i) const
    {
        return MemberReader<ReadFunc>::read_member(this, i);
    }
};

template<typename SizeType, SizeType Elts, uint8_t (*ReadFunc)(const void *)>
class SubByteArray<4, SizeType, Elts, ReadFunc>
{
public: // Would be private, but seems to screw up initializing member(s) with = {...}
    struct ArrayMember
    {
        uint8_t a:4;
        uint8_t b:4;
    };

    template<uint8_t (*_ReadFunc)(const void *), uint8_t Garbage = 42>
    class MemberReader
    {
        typedef SubByteArray<4, SizeType, Elts, _ReadFunc> OuterClass;
    public:
        static inline uint8_t read_member(const OuterClass * outer, SizeType i)
        {
            const union
            {
                uint8_t x;
                ArrayMember m;
            } x = {_ReadFunc(&outer->m_array[i>>1])};
            return (i&1) ? x.m.b : x.m.a;
        }
    };

    template<uint8_t Garbage>
    class MemberReader<nullptr, Garbage>
    {
        typedef SubByteArray<4, SizeType, Elts, nullptr> OuterClass;
    public:
        static inline uint8_t read_member(const OuterClass * outer, SizeType i)
        {
            const ArrayMember & m = outer->m_array[i>>1];
            return (i&1) ? m.b : m.a;
        }
    };

    ArrayMember m_array[SubByteArray<99, SizeType, 1>::integer_ceil_division(Elts,8/4)];

public:
    class ElementRef
    {
        typedef SubByteArray<4, SizeType, Elts, ReadFunc> OuterClass;
        friend ElementRef OuterClass::operator[] (SizeType);
        OuterClass * outer;
        SizeType i;
    public:
        inline ElementRef(OuterClass * outer, SizeType i)
            : outer (outer)
            , i (i)
        { }

        inline ElementRef & operator= (const uint8_t & rhs)
        {
            ArrayMember & m = outer->m_array[i>>1];
            if (i&1)
                m.b = rhs;
            else
                m.a = rhs;
            return *this;
        }

        inline operator uint8_t() const
        {
            return MemberReader<ReadFunc>::read_member(outer, i);
        }
    };

    inline ElementRef operator[] (SizeType i)
    {
        return ElementRef(this, i);
    }

    inline uint8_t operator[] (SizeType i) const
    {
        return MemberReader<ReadFunc>::read_member(this, i);
    }
};


template<uint8_t BitsPerElt, typename SizeType, SizeType Rows, SizeType Cols, uint8_t (*ReadFunc)(const void *) = nullptr>
class SubByteArray2D
{
public:
    typedef SubByteArray<BitsPerElt, SizeType, Rows * Cols, ReadFunc> ArrayType;
    ArrayType m_array;

    class Row
    {
        typedef SubByteArray2D<BitsPerElt, SizeType, Rows, Cols, ReadFunc> OuterClass;
        friend Row OuterClass::operator[] (SizeType);

        OuterClass * outer;
        const SizeType idx;

        Row(OuterClass * that, SizeType idx)
            : outer (that)
            , idx (idx)
        { }

    public:
        inline decltype(outer->m_array[0]) operator[] (SizeType col)
        {
            return outer->m_array[idx + col];
        }
    };

    class ConstRow
    {
        typedef SubByteArray2D<BitsPerElt, SizeType, Rows, Cols, ReadFunc> OuterClass;
        friend ConstRow OuterClass::operator[] (SizeType) const;

        const OuterClass * outer;
        const SizeType idx;

        ConstRow(const OuterClass * that, SizeType idx)
            : outer (that)
            , idx (idx)
        { }

    public:
        inline uint8_t operator[] (SizeType col) const
        {
            return outer->m_array[idx + col];
        }
    };

    inline Row operator[] (SizeType row)
    {
        return Row(this, row * Cols);
    }

    inline ConstRow operator[] (SizeType row) const
    {
        return ConstRow(this, row * Cols);
    }
};

#endif // SUB_BYTE_ARRAY_H_49836739__
