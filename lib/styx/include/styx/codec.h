/* Copyright (c) 2026 bhgv
 * EN: OS-free scalar facade for the exact original Inferno 9P2000 codecs; no allocation or VM headers.
 * RU: Независимый от OS скалярный facade исходных codec Inferno 9P2000; без allocation и VM headers.
 */
#ifndef STANDALONE_STYX_CODEC_H
#define STANDALONE_STYX_CODEC_H
#include <stdint.h>
#include <limits.h>
typedef unsigned char uchar;   /* Wire octet. */
typedef unsigned short ushort; /* Original Inferno API short scalar. */
typedef unsigned int uint;     /* Original API sizes; require at least32 bits. */
typedef unsigned long ulong;   /* Original API display metadata; wire codec always emits explicit32-bit fields. */
typedef uint32_t u32int;       /* Exact original fid/generation width. */
typedef uint64_t uvlong;       /* Exact unsigned64 qid/offset storage. */
typedef int64_t vlong;         /* Exact signed64 native offset/length storage. */
typedef char styx_octet_width[(CHAR_BIT == 8 && sizeof(ushort) == 2 && sizeof(uint) >= 4)
                                  ? 1
                                  : -1]; /* Reject incompatible C targets. */

/* EN/RU: Native metadata only; original codec marshals fields explicitly, never memcpy of this structure on wire.
 * Только native metadata; исходный codec маршалит поля явно, не memcpy структуры в wire.
 */
typedef struct Qid
{
    uvlong path; /* Stable object identity. */
    ulong vers;  /* Wire uint32 content version. */
    uchar type;  /* Standard qid kind bits. */
} Qid;

/* EN/RU: Original scalar Dir layout for codec-only consumers; string ownership stays with the caller.
 * Исходная скалярная Dir для codec-only потребителей; строками владеет вызывающий.
 */
typedef struct Dir
{
    ushort type;  /* Server kind. */
    uint dev;     /* Server device identifier. */
    Qid qid;      /* Stable backend object metadata. */
    ulong mode;   /* Standard mode/rwx bits. */
    ulong atime;  /* Wire uint32 access time. */
    ulong mtime;  /* Wire uint32 modification time. */
    vlong length; /* File length. */
    char *name;   /* Borrowed component. */
    char *uid;    /* Borrowed owner. */
    char *gid;    /* Borrowed group. */
    char *muid;   /* Borrowed modifier. */
} Dir;
#ifdef __cplusplus
extern "C"
{
#endif
#include "fcall.h"
#ifdef __cplusplus
}
#endif
#endif
