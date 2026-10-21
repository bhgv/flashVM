/* Binary S-record profile 1: canonical signed-magnitude integers and bounded checked records. */
#ifndef FLASHVM_SRECORD_H
#define FLASHVM_SRECORD_H /* Guard the standalone C89 wire-format interface. */
#include "flashvm.h"
#include <limits.h>
#define FVM_SRECORD_OCTET_BITS 8U /* Wire octets require eight-bit C bytes. */
#if CHAR_BIT != FVM_SRECORD_OCTET_BITS
#error Binary S-record profile 1 requires eight-bit bytes
#endif
#define FVM_SRECORD_PROFILE 1U       /* Out-of-band profile; no implicit wire negotiation. */
#define FVM_SRECORD_FINAL 0x80U      /* Set on the last integer octet, not a continuation octet. */
#define FVM_SRECORD_SIGN 0x40U       /* Sign in the first octet only. */
#define FVM_SRECORD_FIRST_MASK 0x3fU /* Six magnitude bits in the first integer octet. */
#define FVM_SRECORD_NEXT_MASK 0x7fU  /* Seven magnitude bits in subsequent octets. */
#define FVM_SRECORD_U32 0xffffffffUL /* Bound a portable limb and the CRC arithmetic. */
#define FVM_SRECORD_SHIFT_LIMIT                                                                    \
    0x01ffffffUL                          /* Largest high limb safe before a seven-bit shift.      \
                                           */
#define FVM_SRECORD_INTEGER_MAX 10U       /* Maximum canonical encoding of a 64-bit magnitude. */
#define FVM_SRECORD_BODY_MAX 16777216UL   /* Absolute profile body bound, including encoded tag. */
#define FVM_SRECORD_CRC_POLY 0xedb88320UL /* Reflected CRC-32/ISO-HDLC polynomial. */
#define FVM_SRECORD_CONTROL_TAG 0UL       /* Direction-local type binding records. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Protocol results are separate from VM execution results. */
    typedef enum FVM_SResult
    {
        FVM_S_OK = 0,       /* Complete valid record/value or successful write. */
        FVM_S_MORE = 1,     /* Valid prefix needs more bytes; no output published. */
        FVM_S_INVALID = 2,  /* Invalid argument, integer, geometry or body. */
        FVM_S_LIMIT = 3,    /* Capacity or configured record bound exceeded. */
        FVM_S_CHECKSUM = 4, /* Complete record checksum mismatch. */
        FVM_S_CONFLICT = 5, /* A direction-local tag has another immutable binding. */
        FVM_S_UNKNOWN = 6   /* Requested type binding does not exist. */
    } FVM_SResult;
    /* Host-endian limbs represent a 64-bit magnitude without requiring C99 long long. */
    typedef struct FVM_SInteger
    {
        unsigned long high; /* Most significant 32 magnitude bits, never a pointer. */
        unsigned long low;  /* Least significant 32 magnitude bits. */
        int negative;       /* Exactly 0 or 1; negative zero is rejected. */
    } FVM_SInteger;
    /* Verified view borrows bytes; preserve the complete input through use of this view. */
    typedef struct FVM_SRecord
    {
        unsigned long tag;            /* Unsigned 32-bit direction-local tag; zero is reserved. */
        const unsigned char *payload; /* Borrowed opaque body after the encoded tag. */
        size_t size;         /* Payload bytes, including NULs/nested records if supplied. */
        size_t encoded_size; /* Complete record extent including checksum and length. */
    } FVM_SRecord;
    /* Canonical big-endian signed magnitude. Nonoverlapping outputs required. No allocation/VM.
     * Write/read value and used only on OK; null input is valid only when size is zero (MORE).
     * Write needs valid value/output/used. Read needs value/used. Maximum magnitude is 2^64-1. */
    FVM_API FVM_SResult fvm_sinteger_write(const FVM_SInteger *, unsigned char *, size_t,
                                           size_t *used);
    FVM_API FVM_SResult fvm_sinteger_read(const unsigned char *, size_t, FVM_SInteger *,
                                          size_t *used);
    /* Record = unsigned checksum, unsigned length, unsigned tag, payload. Length counts
     * tag+payload. CRC covers exact length/tag/payload bytes; profile 1 retains
     * 6+7*(length-varint-size-1) bits, capped at 32. Integers are canonical, so checksum may use
     * fewer bytes if its value is small. Write validates all sizes before output. Payload/output
     * must not overlap; no VM/GC/SDL. Outputs and used remain unchanged on failure. Tag <= U32,
     * body <= BODY_MAX. */
    FVM_API FVM_SResult fvm_srecord_write(unsigned long tag, const void *payload, size_t size,
                                          unsigned char *output, size_t capacity, size_t *used);
    /* Scatter-input equivalent of write: concatenate two disjoint borrowed byte spans without
     * an object-sized temporary buffer. Each span and all outputs must not overlap. All size,
     * checksum and capacity checks precede output; no allocation or native structure serialization.
     */
    FVM_API FVM_SResult fvm_srecord_write_pair(unsigned long tag, const void *first,
                                               size_t first_size, const void *second,
                                               size_t second_size, unsigned char *, size_t capacity,
                                               size_t *used);
    /* limit is 1..BODY_MAX and counts tag+payload; pass BODY_MAX explicitly for the full profile.
     * MORE means retain/retry the prefix. Checksum is verified before tag interpretation.
     * OK publishes one record and consumed extent; trailing records remain unconsumed.
     * Caller must initialize neither view nor used; both remain unchanged on non-OK. */
    FVM_API FVM_SResult fvm_srecord_read(const void *input, size_t size, size_t limit,
                                         FVM_SRecord *view, size_t *used);
#ifdef __cplusplus
}
#endif
#endif
