/* FlashVM: legacy script quality numeric policy, independent of public enum ordinals. */
#ifndef FVM_MOVIECLIP_QUALITY_CONSTANTS_H
#define FVM_MOVIECLIP_QUALITY_CONSTANTS_H /* Numeric policy guard. */
#define FVM_QUALITY_LEGACY_LOW 0.0        /* LOW and MEDIUM read as zero. */
#define FVM_QUALITY_LEGACY_HIGH 1.0       /* HIGH reads as one. */
#define FVM_QUALITY_LEGACY_BEST 2.0       /* BEST reads as two. */
#define FVM_QUALITY_BEST_THRESHOLD 1.5    /* Strictly greater values select BEST. */
#endif
