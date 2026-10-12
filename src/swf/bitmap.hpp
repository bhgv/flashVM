/* FlashVM: bounded decoded SWF lossless bitmap resources before managed graph publication. */
#ifndef FVM_SWF_BITMAP_HPP
#define FVM_SWF_BITMAP_HPP /* Include guard for lossless bitmap resources. */
#include "../details/vector.hpp"
#include "flashvm/value.h"
/* Independently owned straight RGBA pixels decoded from one lossless bitmap tag. */
struct FVM_SWFBitmap
{
    unsigned id;      /* Nonzero SWF character identifier. */
    unsigned width;   /* Number of pixels in each decoded row, bounded to 2880. */
    unsigned height;  /* Number of decoded rows, bounded to 2880. */
    bool transparent; /* Whether the lossless tag carries an alpha channel. */
    fvm_detail::vector<unsigned char> pixels; /* Owned tightly packed straight RGBA bytes. */
    /* Initialize an empty resource before bounded decoding. */
    FVM_SWFBitmap() : id(0), width(0), height(0), transparent(false)
    {
    }
};
/* Decode tag 20/36 into a bounded bitmap; malformed streams leave output unchanged. */
FVM_Result fvm_swf_read_bitmap(const unsigned char *, size_t, unsigned, size_t, FVM_SWFBitmap &);
/* Decode JPEG2/3 resources and optional exact zlib alpha into a bounded bitmap. */
FVM_Result fvm_swf_read_jpeg(const unsigned char *, size_t, unsigned, unsigned, size_t,
                             FVM_SWFBitmap &);
/* Normalize one complete JPEG Tables/Misc stream without accepting image markers. */
FVM_Result fvm_swf_read_jpeg_tables(const unsigned char *, size_t, unsigned,
                                    fvm_detail::vector<unsigned char> &);
/* Join abbreviated DefineBits data to validated shared tables before bounded JPEG decoding. */
FVM_Result fvm_swf_read_legacy_jpeg(const unsigned char *, size_t,
                                    const fvm_detail::vector<unsigned char> &, unsigned, size_t,
                                    FVM_SWFBitmap &);
#endif
