#ifndef BITFIELD_H
#define BITFIELD_H

/************************************************************************
 * This is for creating Endianness-free bit-fields.
 * Implementation taken from: https://codereview.stackexchange.com/questions/54342/
 * The values are always packed as big-endian.
 * When used for arithmetics/comparison, the values are read in the
 * proper endianness for the current CPU mode.
 *
 * Note: The fields can overlap if you want
 */

template<int firstBit, int bitSize>
struct BitFieldMember {
    typedef BitFieldMember<firstBit, bitSize> self_t;
    typedef unsigned char uchar;
    enum {
        lastBit = firstBit + bitSize - 1, mask = (1ULL << bitSize) - 1
    };
    uchar *selfArray() {
        return reinterpret_cast<uchar *>(this);
    }
    const uchar *selfArray() const {
        return reinterpret_cast<const uchar *>(this);
    }

    /* used to read data from the field */
    /* will also work with all the operators that work with integral types */
    inline operator unsigned() const {
        const uchar *arr = selfArray();
        const uchar *p = arr + firstBit / 8;
        int i = 8 - (firstBit & 7);
        unsigned ret = 0;
        ret |= *p;
        while (i < bitSize) {
            ret <<= 8;
            ret |= *(++p);
            i += 8;
        }
        return ((ret >> (7 - (lastBit & 7))) & mask);
    }

    /* used to assign a value into the field */
    inline self_t& operator=(unsigned m) {
        uchar *arr = selfArray();
        m &= mask;
        unsigned wmask = ~(mask << (7 - (lastBit & 7)));
        m <<= (7 - (lastBit & 7));
        uchar *p = arr + lastBit / 8;
        int i = (lastBit & 7) + 1;
        (*p &= wmask) |= m;
        while (i < bitSize) {
            m >>= 8;
            wmask >>= 8;
            (*(--p) &= wmask) |= m;
            i += 8;
        }
        return *this;
    }

    inline self_t& operator+=(unsigned m) {
        *this = *this + m;
        return *this;
    }

    inline self_t& operator-=(unsigned m) {
        *this = *this - m;
        return *this;
    }

    inline self_t& operator*=(unsigned m) {
        *this = *this * m;
        return *this;
    }

    inline self_t& operator/=(unsigned m) {
        *this = *this / m;
        return *this;
    }

    inline self_t& operator%=(unsigned m) {
        *this = *this % m;
        return *this;
    }

    inline self_t& operator<<=(unsigned m) {
        *this = *this << m;
        return *this;
    }

    inline self_t& operator>>=(unsigned m) {
        *this = *this >> m;
        return *this;
    }

    inline self_t& operator|=(unsigned m) {
        *this = *this | m;
        return *this;
    }

    inline self_t& operator&=(unsigned m) {
        *this = *this & m;
        return *this;
    }

    inline self_t& operator^=(unsigned m) {
        *this = *this ^ m;
        return *this;
    }

};

#endif /* BITFIELD_H_ */
