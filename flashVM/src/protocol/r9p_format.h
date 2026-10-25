/* Private standard 9P2000 wire IDs and extents; S-record payloads use their separate own format. */
#ifndef FLASHVM_R9P_FORMAT_H
#define FLASHVM_R9P_FORMAT_H  /* Keep adapter constants local to the protocol component. */
#define R9_VERSION 100U       /* Tversion; paired replies have request type plus one. */
#define R9_AUTH 102U          /* Authentication is supplied by the host transport, not this adapter. */
#define R9_ATTACH 104U        /* Attach the already authenticated resource root. */
#define R9_ERROR 107U         /* Rerror carries a counted advisory string. */
#define R9_FLUSH 108U         /* Cancel a pending read's old tag. */
#define R9_WALK 110U          /* Traverse at most sixteen counted components. */
#define R9_OPEN 112U          /* Open an existing resource node. */
#define R9_READ 116U          /* Read at a split 64-bit offset. */
#define R9_WRITE 118U         /* Write one atomic operation payload. */
#define R9_CLUNK 120U         /* Release one fid and its native file. */
#define R9_STAT 124U          /* Return a counted machine-independent stat entry. */
#define R9_HEADER 7U          /* Size Uint32, type octet, tag Uint16. */
#define R9_READ_HEADER 11U    /* Reply header plus Uint32 byte count. */
#define R9_IOHDRSZ 24U        /* Standard conservative maximum I/O framing overhead. */
#define R9_QID 13U            /* Type octet, version Uint32, path Uint64. */
#define R9_QTDIR 128U         /* Directory qid bit. */
#define R9_DMDIR 2147483648UL /* Directory mode bit. */
#define R9_DIR_MODE 0500UL    /* Informational owner read/search permissions. */
#define R9_FILE_MODE 0400UL   /* Informational owner read permissions. */
#define R9_CALL_MODE 0600UL   /* Informational owner read/write permissions. */
#define R9_OREAD 0U           /* Standard read-only open mode. */
#define R9_ORDWR 2U           /* Standard read/write open mode required by call files. */
#define R9_STAT_MAX 192U      /* Bounded encoded stat with the largest resource component. */
#define R9_U16 65535UL        /* Maximum unsigned two-byte wire value. */
#define R9_OCTET 255UL        /* Mask for little-endian wire octets. */
#define R9_SHIFT 8U           /* Bits per wire octet. */
#define R9_U32_BYTES 4U       /* Portable Uint32 width. */
#define R9_U16_BYTES 2U       /* Portable Uint16 width. */
#endif
