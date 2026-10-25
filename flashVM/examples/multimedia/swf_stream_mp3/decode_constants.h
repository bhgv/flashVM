/* Bounds for the real SWF-to-PCM export tutorial, independent of encoded format. */
#ifndef FVM_TUTORIAL_MP3_DECODE_CONSTANTS_H         /* Begin the scoped tutorial bounds guard. */
#define FVM_TUTORIAL_MP3_DECODE_CONSTANTS_H         /* Prevent repeated bound declarations. */
#define TUTORIAL_MP3_BYTES (64UL * 1024UL * 1024UL) /* Bound input and native preparation independently. */
#define TUTORIAL_MP3_VALUES 512U                    /* Bound each native interleaved output chunk. */
#endif                                              /* End the scoped tutorial bounds guard. */
