/* Explicit geometry and finite output policy for one stream plus two native event voices. */
#ifndef FVM_EXAMPLE_STREAM_MIX_H /* Keep example-only constants independent of public ABI. */
#define FVM_EXAMPLE_STREAM_MIX_H /* Begin the tutorial header. */
#define EXAMPLE_MIX_RATE 48000U /* Convert both source kinds to the same explicit output clock. */
#define EXAMPLE_MIX_CHANNELS 2U /* Write interleaved stereo S16. */
#define EXAMPLE_MIX_VOICES 2U /* Prepare both StartSound events from the embedded movie. */
#define EXAMPLE_MIX_BLOCK 4U /* Bound caller PCM/retry storage to four output frames. */
#define EXAMPLE_MIX_TOTAL 16U /* Explicit finite export includes the longer event tail and silence. */
#define EXAMPLE_MIX_BUDGET 65536UL /* Bound native stream snapshot preparation to 64 KiB. */
#endif /* End example-only geometry. */
