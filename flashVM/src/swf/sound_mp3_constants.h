/* MPEG Layer-III framing bounds, kept separate from SWF sound tag identities. */
#ifndef FVM_SWF_SOUND_MP3_CONSTANTS_H
#define FVM_SWF_SOUND_MP3_CONSTANTS_H /* Guard private MPEG wire constants. */
#define FVM_MP3_HEADER_BYTES 4U /* Sync and all MPEG frame fields. */
#define FVM_MP3_SYNC_FIRST 255U /* First eight synchronization bits. */
#define FVM_MP3_SYNC_MASK 224U /* Remaining three sync bits in the second byte. */
#define FVM_MP3_VERSION_SHIFT 3U /* MPEG version ID position in byte one. */
#define FVM_MP3_VERSION_MASK 3U /* Two-bit version ID. */
#define FVM_MP3_VERSION_1 3U /* MPEG-1, 1152 frames per Layer-III packet. */
#define FVM_MP3_VERSION_2 2U /* MPEG-2, 576 frames per Layer-III packet. */
#define FVM_MP3_VERSION_25 0U /* MPEG-2.5, quarter-rate MPEG-1 sampling. */
#define FVM_MP3_VERSION_RESERVED 1U /* Invalid reserved version ID. */
#define FVM_MP3_LAYER_SHIFT 1U /* Layer ID position in byte one. */
#define FVM_MP3_LAYER_MASK 3U /* Two-bit layer ID. */
#define FVM_MP3_DECODED_LAYER_3 3U /* Upstream decoded layer identity, distinct from wire bits. */
#define FVM_MP3_LAYER_3 1U /* Only Layer III belongs in SWF MP3 data. */
#define FVM_MP3_BITRATE_SHIFT 4U /* High nibble in byte two. */
#define FVM_MP3_BITRATE_BAD 15U /* Reserved invalid bitrate code. */
#define FVM_MP3_BITRATE_FREE 0U /* Free-format framing needs a separate inference contract. */
#define FVM_MP3_RATE_SHIFT 2U /* Two-bit sampling-rate table index in byte two. */
#define FVM_MP3_RATE_MASK 3U /* Sampling-rate index field. */
#define FVM_MP3_RATE_RESERVED 3U /* Invalid sampling-rate table index. */
#define FVM_MP3_PADDING_MASK 2U /* Adds one complete byte to a Layer-III packet. */
#define FVM_MP3_CHANNEL_SHIFT 6U /* Stereo-mode field in byte three. */
#define FVM_MP3_MONO_MODE 3U /* Single-channel MPEG mode. */
#define FVM_MP3_EMPHASIS_MASK 3U /* Final two header bits. */
#define FVM_MP3_EMPHASIS_RESERVED 2U /* Reserved emphasis code. */
#define FVM_MP3_MPEG1_COEFFICIENT 144000UL /* Packet bytes per bitrate-kbps divided by Hz. */
#define FVM_MP3_MPEG2_COEFFICIENT 72000UL /* Half-sample MPEG-2/2.5 packet coefficient. */
#define FVM_MP3_MPEG1_SAMPLES 1152U /* Interleaved sample frames, not channel values. */
#define FVM_MP3_MPEG2_SAMPLES 576U /* MPEG-2 and MPEG-2.5 Layer-III frame length. */
#define FVM_MP3_CHANNELS 2U /* Maximum MPEG Layer-III decoded channel count. */
#define FVM_MP3_MAX_VALUES (FVM_MP3_MPEG1_SAMPLES * FVM_MP3_CHANNELS) /* Fixed decoded block. */
#define FVM_MP3_SEEK_BYTES 2U /* Signed SWF event SeekSamples prefix before MPEG frames. */
#define FVM_MP3_MIN_SWF_VERSION 4U /* First SWF version supporting MP3. */
#endif
