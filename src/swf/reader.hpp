/* FlashVM: bounded byte/bit decoding for SWF records, matrices and color transforms. */
#ifndef FLASHVM_SWF_READER_HPP
#define FLASHVM_SWF_READER_HPP /* Include guard for the SWF record decoder. */
#include "flashvm/path.h"
#include "../details/string.hpp"

/* Non-owning reader; an error is sticky and never advances beyond the record boundary. */
class FVM_SWFReader
{
    const unsigned char *bytes_; /* Borrowed canonical SWF bytes. */
    size_t position_;            /* Current byte offset relative to this record. */
    size_t size_;                /* Exclusive record boundary. */
    unsigned int bit_;           /* Next most-significant bit within the current byte. */
    bool valid_;                 /* Whether all previous reads were in bounds. */
  public:
    /* Start at the first byte of one already bounded SWF record. */
    FVM_SWFReader(const unsigned char *bytes, size_t size)
        : bytes_(bytes), position_(0), size_(size), bit_(0), valid_(bytes != 0 || size == 0)
    {
    }
    /* Read up to 32 bits in SWF's most-significant-bit-first order. */
    unsigned long bits(unsigned int count)
    {
        if (!valid_ || count > 32)
        {
            valid_ = false;
            return 0;
        }
        unsigned long value = 0;
        for (unsigned int i = 0; i < count; ++i)
        {
            if (position_ >= size_)
            {
                valid_ = false;
                return 0;
            }
            value = (value << 1) | ((bytes_[position_] >> (7 - bit_)) & 1);
            if (++bit_ == 8)
            {
                bit_ = 0;
                ++position_;
            }
        }
        return value;
    }
    /* Sign-extend a bit field without signed shifts or out-of-range positive casts. */
    long signed_bits(unsigned int count)
    {
        unsigned long value = bits(count);
        if (!valid_ || count == 0)
            return 0;
        if (count == 32)
            return value < 0x80000000UL ? (long)value : -1L - (long)(0xffffffffUL - value);
        return value & (1UL << (count - 1)) ? -(long)((1UL << count) - value) : (long)value;
    }
    /* Skip padding bits before a byte-aligned field. */
    void align()
    {
        if (bit_)
        {
            bit_ = 0;
            ++position_;
        }
    }
    /* Read a byte-aligned little-endian integer of up to four bytes. */
    unsigned long integer(unsigned int count)
    {
        align();
        if (!valid_ || count > 4 || count > size_ - position_)
        {
            valid_ = false;
            return 0;
        }
        unsigned long value = 0;
        for (unsigned int i = 0; i < count; ++i)
            value |= (unsigned long)bytes_[position_++] << (8 * i);
        return value;
    }
    /* Copy a terminated SWF string; a missing terminator invalidates the record. */
    bool string(fvm_detail::string &out)
    {
        align();
        if (!valid_)
            return false;
        size_t first = position_;
        while (position_ < size_ && bytes_[position_] != 0)
            ++position_;
        if (position_ == size_)
        {
            valid_ = false;
            return false;
        }
        out = fvm_detail::string((const char *)bytes_ + first, position_ - first);
        ++position_;
        return true;
    }
    /* Test whether the record remains valid after any sequence of reads. */
    bool good() const
    {
        return valid_;
    }
    /* Require an exact byte-aligned record ending, excluding unused trailing fields. */
    bool finished()
    {
        align();
        return valid_ && position_ == size_;
    }
};

/* Per-channel affine color transform; multipliers operate on 0..255 components. */
struct FVM_SWFColorTransform
{
    double multiply[4]; /* Red, green, blue and alpha multipliers. */
    double add[4];      /* Red, green, blue and alpha offsets. */
    /* Initialize the identity transform for omitted color fields. */
    FVM_SWFColorTransform()
    {
        for (unsigned int i = 0; i < 4; ++i)
        {
            multiply[i] = 1;
            add[i] = 0;
        }
    }
};

/* Decode an affine SWF MATRIX, converting twip translations to pixel coordinates. */
inline bool fvm_swf_read_matrix(FVM_SWFReader &reader, FVM_Affine &matrix)
{
    matrix.xx = matrix.yy = 1;
    matrix.xy = matrix.yx = matrix.x0 = matrix.y0 = 0;
    if (reader.bits(1))
    {
        unsigned int count = (unsigned int)reader.bits(5);
        matrix.xx = reader.signed_bits(count) / 65536.0;
        matrix.yy = reader.signed_bits(count) / 65536.0;
    }
    if (reader.bits(1))
    {
        unsigned int count = (unsigned int)reader.bits(5);
        matrix.yx = reader.signed_bits(count) / 65536.0;
        matrix.xy = reader.signed_bits(count) / 65536.0;
    }
    unsigned int count = (unsigned int)reader.bits(5);
    matrix.x0 = reader.signed_bits(count) / 20.0;
    matrix.y0 = reader.signed_bits(count) / 20.0;
    reader.align();
    return reader.good();
}

/* Decode CXFORM/CXFORMWITHALPHA, defaulting absent terms and preserving signed offsets. */
inline bool fvm_swf_read_color(FVM_SWFReader &reader, bool alpha, FVM_SWFColorTransform &color)
{
    color = FVM_SWFColorTransform();
    bool has_add = reader.bits(1) != 0, has_multiply = reader.bits(1) != 0;
    unsigned int count = (unsigned int)reader.bits(4);
    unsigned int channels = alpha ? 4 : 3;
    if (has_multiply)
        for (unsigned int i = 0; i < channels; ++i)
            color.multiply[i] = reader.signed_bits(count) / 256.0;
    if (has_add)
        for (unsigned int i = 0; i < channels; ++i)
            color.add[i] = (double)reader.signed_bits(count);
    reader.align();
    return reader.good();
}
#endif
