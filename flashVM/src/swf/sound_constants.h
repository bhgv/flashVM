/* FlashVM: DefineSound wire fields and exact PCM representation boundaries. */
#ifndef FVM_SWF_SOUND_CONSTANTS_H
#define FVM_SWF_SOUND_CONSTANTS_H     /* Guard the private sound wire constants. */
#include "flashvm/swf_sound.h"        /* Reuse public PCM codec identities without duplicating their values. */

#define FVM_SWF_SOUND_HEADER_BYTES 7U /* UI16 ID, flags byte and UI32 frame count. */
#define FVM_SWF_SOUND_ID_MAX 65535U   /* Largest root sound dictionary identifier. */
#define FVM_SWF_SOUND_FORMAT_SHIFT 4U /* High nibble contains SoundFormat. */
#define FVM_SWF_SOUND_RATE_MASK 12U   /* Bits 2..3 select the nominal header rate. */
#define FVM_SWF_SOUND_RATE_SHIFT 2U   /* Shift rate bits to a table index. */
#define FVM_SWF_SOUND_SIZE_MASK 2U    /* Set means 16-bit rather than 8-bit header samples. */
#define FVM_SWF_SOUND_STEREO_MASK 1U  /* Set means two interleaved channels. */
#define FVM_SWF_SOUND_FORMAT_NATIVE FVM_SWF_SOUND_PCM_NATIVE /* Legacy PCM follows the playback host's byte order. */
#define FVM_SWF_SOUND_FORMAT_LE FVM_SWF_SOUND_PCM_LE         /* SWF4+ explicitly little-endian PCM. */
#define FVM_SWF_SOUND_LE_MIN_VERSION 4U                      /* Earliest supported SWF version for format 3. */
#define FVM_SWF_SOUND_RATE_5K 5512U         /* Integer nominal representation of the 5.5 kHz rate code. */
#define FVM_SWF_SOUND_RATE_11K 11025U       /* Nominal 11 kHz rate code. */
#define FVM_SWF_SOUND_RATE_22K 22050U       /* Nominal 22 kHz rate code. */
#define FVM_SWF_SOUND_RATE_44K 44100U       /* Nominal 44 kHz rate code. */
#define FVM_SWF_SOUND_BITS_8 8U             /* Header eight-bit sample width. */
#define FVM_SWF_SOUND_BITS_16 16U           /* Supported native signed-short sample width. */
#define FVM_SWF_SOUND_U8_BIAS 128L          /* Offset-binary eight-bit PCM silence. */
#define FVM_SWF_SOUND_U8_SCALE 256L         /* Expand U8 into the high eight bits of signed-16 output. */
#define FVM_SWF_SOUND_PCM_BYTES 2U          /* Complete signed-16 PCM channel value. */
#define FVM_SWF_SOUND_SIGN_BIT 32768UL      /* Unsigned wire values at or above this encode negative S16. */
#define FVM_SWF_SOUND_SIGN_RANGE 65536L     /* Exact signed conversion without implementation-defined narrowing. */
#define FVM_SWF_SOUND_SHORT_MIN (-32768L)   /* Required signed-16 output minimum. */
#define FVM_SWF_SOUND_SHORT_MAX 32767       /* Required host signed-short maximum. */
#define FVM_SWF_SOUND_UI16_BYTES 2U         /* Width of wire IDs and short tag headers. */
#define FVM_SWF_SOUND_UI32_BYTES 4U         /* Width of counts and extended tag lengths. */
#define FVM_SWF_SOUND_FLAGS_OFFSET 2U       /* Byte after the UI16 sound ID. */
#define FVM_SWF_SOUND_COUNT_OFFSET 3U       /* First byte of the UI32 sample count. */
#define FVM_SWF_SOUND_FILE_PREFIX_BYTES 8U  /* Signature/version/declared-length prefix before RECT. */
#define FVM_SWF_SOUND_MIN_FILE_BYTES 9U     /* Prefix plus the first RECT byte. */
#define FVM_SWF_SOUND_RECT_WIDTH_SHIFT 3U   /* Top five bits of the first RECT byte encode Nbits. */
#define FVM_SWF_SOUND_RECT_WIDTH_BITS 5U    /* Width of the RECT Nbits field. */
#define FVM_SWF_SOUND_RECT_COORDINATES 4U   /* xMin/xMax/yMin/yMax fields in RECT. */
#define FVM_SWF_SOUND_BYTE_BITS 8U          /* SWF wire bytes are exactly eight bits. */
#define FVM_SWF_SOUND_BYTE_ROUNDING 7U      /* Round a RECT bit count up to complete bytes. */
#define FVM_SWF_SOUND_MOVIE_HEADER_BYTES 4U /* FrameRate and FrameCount follow RECT. */
#define FVM_SWF_SOUND_MONO_CHANNELS 1U      /* Header mono PCM frame geometry. */
#define FVM_SWF_SOUND_STEREO_CHANNELS 2U    /* Header stereo PCM frame geometry. */
#define FVM_SWF_ADPCM_WIDTH_BITS 2U         /* Encoded width-minus-two field. */
#define FVM_SWF_ADPCM_MIN_WIDTH 2U          /* Smallest differential code width. */
#define FVM_SWF_ADPCM_INDEX_BITS 6U         /* Initial step index width per channel. */
#define FVM_SWF_ADPCM_BLOCK_FRAMES 4096UL   /* Initial sample plus 4095 differential samples. */
#define FVM_SWF_ADPCM_INDEX_MAX 88          /* Largest adaptive step-table index. */
#define FVM_SWF_ADPCM_SAMPLE_MIN (-32768L)  /* Saturated negative signed-16 limit. */
#endif
