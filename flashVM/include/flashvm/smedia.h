/* Bounded media/file fragments carried by checked S-record recovery slots; no socket ownership. */
#ifndef FLASHVM_SMEDIA_H
#define FLASHVM_SMEDIA_H /* Guard the standalone C89 fragment/reassembly API. */
#include "sslot.h"
#define FVM_SMEDIA_VERSION 1UL /* Payload schema within the direction-bound fragment tag. */
#define FVM_SMEDIA_TYPE "flashvm.media.fragment" /* Negotiate the tag separately per direction. */
#define FVM_SMEDIA_FILE 1UL /* Enumerate missing parts for reliable control-channel requests. */
#define FVM_SMEDIA_LIVE 2UL /* Never request missing parts; host owns playback deadlines. */
#define FVM_SMEDIA_PAYLOAD 768UL /* All nonfinal parts carry this many raw bytes. */
#define FVM_SMEDIA_PARTS 4096UL  /* Per-object part/bitmap bound; larger files use more objects. */
#define FVM_SMEDIA_MAX_BYTES (FVM_SMEDIA_PAYLOAD * FVM_SMEDIA_PARTS) /* Three MiB per object. */
#define FVM_SMEDIA_BITMAP (FVM_SMEDIA_PARTS / 8UL) /* One bit per accepted part. */
#define FVM_SMEDIA_FIELDS 8U /* Version, mode, stream, epoch, object, time_ms, total, part. */
#define FVM_SMEDIA_BITS 8U   /* Wire profile requires eight-bit octets. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Agreed immutable identity for one object: host must authenticate it before receiver init. */
    typedef struct FVM_SMediaInfo
    {
        unsigned long mode;   /* FILE or LIVE; receiver rejects a peer's mode change. */
        unsigned long stream; /* Nonzero Uint32 stream identifier assigned by the host. */
        unsigned long epoch; /* Nonzero Uint32 session incarnation; never reused for old traffic. */
        unsigned long object; /* Nonzero Uint32 object/segment identity within that incarnation. */
        unsigned long
            time_ms;         /* Host-defined relative presentation time; zero is valid for files. */
        unsigned long total; /* Exact object byte count, 1..MAX_BYTES. */
    } FVM_SMediaInfo;
    /* Verified borrowed fragment. Raw data is never interpreted as a codec or native structure. */
    typedef struct FVM_SMediaPiece
    {
        FVM_SMediaInfo info;       /* Copied identity and total extent. */
        unsigned long part;        /* Zero-based part index; offset is part*PAYLOAD. */
        const unsigned char *data; /* Borrowed input until the containing slot is changed. */
        size_t size;               /* PAYLOAD bytes, except the exact final remainder. */
    } FVM_SMediaPiece;
    /* Caller-owned, single-owner reassembly state; fields are private after initialization. */
    typedef struct FVM_SMediaReceiver
    {
        size_t struct_size;  /* Exact ABI size initialized by receiver_init. */
        FVM_SMediaInfo info; /* Expected identity; no automatic adoption of network metadata. */
        unsigned long
            tag; /* Direction-local fragment tag agreed on authenticated control channel. */
        unsigned char
            *buffer; /* Borrowed writable object buffer; no VM roots or hidden allocations. */
        size_t
            capacity; /* Accessible extent, at least info.total; stable until reinitialization. */
        unsigned long received; /* Number of distinct accepted parts, not number of packets. */
        unsigned char bitmap[FVM_SMEDIA_BITMAP]; /* Accepted parts; unknown holes are never read. */
    } FVM_SMediaReceiver;
    /* Encode one exact part to one padded slot. tag must be nonzero Uint32. Atomic output/used on
     * failure; payload must have precisely the extent implied by total/part. No allocation or I/O.
     */
    FVM_API FVM_SResult fvm_smedia_write(unsigned long tag, const FVM_SMediaInfo *,
                                         unsigned long part, const void *data, size_t size,
                                         void *output, size_t capacity, size_t *used);
    /* Verify slot, tag, canonical fields and exact part extent before publishing borrowed piece.
     * UNKNOWN means another direction-bound tag. Output unchanged on every error. */
    FVM_API FVM_SResult fvm_smedia_read(const void *, size_t, unsigned long tag, FVM_SMediaPiece *);
    /* Validate expected identity and caller capacity before replacing native state; buffer bytes
     * are untouched. Reinitialization explicitly abandons the previous object at a host-chosen
     * deadline. All descriptors, buffers and slot input/output must be disjoint. No cleanup
     * allocation exists. */
    FVM_API FVM_SResult fvm_smedia_receiver_init(FVM_SMediaReceiver *, unsigned long tag,
                                                 const FVM_SMediaInfo *, void *buffer,
                                                 size_t capacity);
    /* Verify before copying; reject mismatched identity CONFLICT without changing
     * state/buffer/complete. Exact duplicate OK without increasing received; same part with
     * different valid bytes CONFLICT. Copy only verified new part. complete is true once every part
     * arrived, not a playback decision. Caller authenticates packet/session before entry; CRC alone
     * cannot establish trust. */
    FVM_API FVM_SResult fvm_smedia_accept(FVM_SMediaReceiver *, const void *, size_t,
                                          int *complete);
    /* Enumerate absent part indices >=start, at most capacity, in ascending order. Continue with
     * the last returned index+1. FILE reports holes; LIVE always returns count zero. No timer,
     * network request, retry storm or transfer acknowledgement is generated. Invalid args leave
     * output/count unchanged. A host bounds/rate-limits requests and retains sender data through
     * file completion. */
    FVM_API FVM_SResult fvm_smedia_missing(const FVM_SMediaReceiver *, unsigned long start,
                                           unsigned long *parts, size_t capacity, size_t *count);
#ifdef __cplusplus
}
#endif
#endif
