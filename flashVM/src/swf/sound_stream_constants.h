/* FlashVM: streaming sound tag identities and header constraints. */
#ifndef FVM_SWF_SOUND_STREAM_CONSTANTS_H
#define FVM_SWF_SOUND_STREAM_CONSTANTS_H /* Guard stream-specific protocol constants. */
#define FVM_SWF_SOUND_STREAM_HEAD 18U /* Original ADPCM/MP3 stream header tag. */
#define FVM_SWF_SOUND_STREAM_HEAD2 45U /* SWF3+ header supporting uncompressed sample widths. */
#define FVM_SWF_SOUND_STREAM_BLOCK 19U /* Root frame sound payload tag. */
#define FVM_SWF_SOUND_STREAM_HEAD_BYTES 4U /* Playback flags, stream flags and UI16 average count. */
#define FVM_SWF_SOUND_STREAM_MP3_BYTES 6U /* Base header plus signed MP3 latency. */
#define FVM_SWF_SOUND_STREAM_MP3_PREFIX_BYTES 4U /* UI16 declared count followed by SI16 block seek. */
#define FVM_SWF_SOUND_STREAM_RESERVED_MASK 240U /* Playback header high nibble must be zero. */
#define FVM_SWF_SOUND_STREAM_HEAD2_VERSION 3U /* First SWF version permitting tag 45. */
#define FVM_SWF_SOUND_STREAM_MP3 2U /* MP3 compression identity; decoding remains unsupported here. */
#define FVM_SWF_SOUND_STREAM_MP3_VERSION 4U /* First version permitting MP3 stream headers. */
#define FVM_SWF_STREAM_ADPCM_INITIAL_BITS 22U /* Signed initial sample and six-bit index. */
#define FVM_SWF_STREAM_ADPCM_WIDTH_SHIFT 6U /* Two MSB code-width bits in a block. */
#define FVM_SWF_STREAM_ADPCM_PADDING_BITS 7U /* At most seven final zero padding bits. */
#endif
