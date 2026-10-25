/* FlashVM: SWD6--8 record identities and bounded symbol inspection. */
#ifndef FVM_SOURCE_DEBUG_CONSTANTS_H
#define FVM_SOURCE_DEBUG_CONSTANTS_H /* SWD limits guard. */
/* Field widths in the little-endian companion symbol stream. */
#define FVM_SWD_U32_BYTES 4U                       /* Module, record, line and uncompressed-offset width. */
#define FVM_SWD_U8_BYTES 1U                        /* Register index/count width. */
#define FVM_SWD_HEADER_BYTES 4U                    /* Three-byte FWD signature followed by its version. */
#define FVM_SWD_VERSION_INDEX 3U                   /* Version byte in the FWD header. */
#define FVM_SWD_BYTE_BITS 8U                       /* Little-endian byte-to-integer shift distance. */
#define FVM_SWD_SCRIPT 0UL                         /* Module ID, bitmap, terminated filename and source text. */
#define FVM_SWD_OFFSET 1UL                         /* Module ID, one-based line and absolute uncompressed SWF offset. */
#define FVM_SWD_BREAKPOINT 2UL                     /* Authoring breakpoint offset; not automatically enabled. */
#define FVM_SWD_ID 3UL                             /* Opaque 16-byte SWF DebugID association. */
#define FVM_SWD_REGISTERS 5UL                      /* Offset, byte count, byte register IDs and terminated names. */
#define FVM_SWD_END 0xffffffffUL                   /* Explicit end marker. */
#define FVM_SWD_MIN_VERSION 6U                     /* First supported FWD generation. */
#define FVM_SWD_MAX_VERSION 8U                     /* AVM1-focused supported generation. */
#define FVM_SWD_MAX_RECORDS 65536UL                /* Bounded source/line/register metadata records. */
#define FVM_SWD_MAX_BYTES (16UL * 1024UL * 1024UL) /* Maximum SWD input. */
#define FVM_SWD_MAX_MATCHES 4096UL                 /* Bound ambiguous identical-body candidate searches. */
#define FVM_SWD_LOOKUP_WORK (128UL * 1024UL * 1024UL) /* Maximum compared body bytes. */
#endif
