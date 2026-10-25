/* FlashVM: bounded diagnostics and ZIP32 stored-record identities. */
#ifndef FVM_CRASH_CONSTANTS_H
#define FVM_CRASH_CONSTANTS_H                        /* Diagnostic constants guard. */
#define FVM_CRASH_MAX_FRAMES 256U                    /* Maximum retained activation frames. */
#define FVM_CRASH_MAX_BYTES (16UL * 1024UL * 1024UL) /* Total report payload budget. */
#define FVM_CRASH_ZIP_LOCAL 0x04034b50UL             /* ZIP local-file record. */
#define FVM_CRASH_ZIP_CENTRAL 0x02014b50UL           /* ZIP central directory record. */
#define FVM_CRASH_ZIP_END 0x06054b50UL               /* ZIP end-of-directory record. */
#define FVM_CRASH_ZIP_VERSION 20U                    /* ZIP 2.0 compatible stored entries. */
#define FVM_CRASH_BYTE_MASK 255U                     /* Low byte during little-endian record encoding. */
#define FVM_CRASH_BYTE_BITS 8U                       /* ZIP field byte width. */
#endif
