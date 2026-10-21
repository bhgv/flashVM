/* Compile the unmodified CC0 scalar Layer-III decoder once, with private symbol prefixes. */
#define MINIMP3_IMPLEMENTATION /* Emit the upstream decoder implementation in this unit only. */
#define MINIMP3_ONLY_MP3 /* SWF format 2 is MPEG Layer III, never Layer I/II. */
#define MINIMP3_NO_SIMD /* Keep one portable scalar implementation independent of target ISA. */
#define mp3dec_init fvm_minimp3_init /* Avoid collisions with an embedding host's decoder. */
#define mp3dec_decode_frame fvm_minimp3_decode_frame /* Isolate the second upstream external symbol. */
#include "upstream/minimp3.h" /* Preserve the complete upstream code and licensing notice. */
