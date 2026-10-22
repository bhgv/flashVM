/* Explicit geometry and finite output policy for one stream plus two native event voices. */
#ifndef FVM_EXAMPLE_STREAM_MIX_H /* Keep example-only constants independent of public ABI. */
#define FVM_EXAMPLE_STREAM_MIX_H /* Begin the tutorial header. */
#define EXAMPLE_MIX_RATE 48000U /* Convert both source kinds to the same explicit output clock. */
#define EXAMPLE_MIX_CHANNELS 2U /* Write interleaved stereo S16. */
#define EXAMPLE_MIX_VOICES 2U /* Prepare both StartSound events from the embedded movie. */
#define EXAMPLE_MIX_BLOCK 4U /* Bound caller PCM/retry storage to four output frames. */
#define EXAMPLE_MIX_TOTAL 16U /* Explicit finite export includes the longer event tail and silence. */
#define EXAMPLE_MIX_BUDGET 65536UL /* Bound native stream snapshot preparation to 64 KiB. */
#define EXAMPLE_MIX_VOICE_BYTES 65536UL /* Bound each temporary prepared voice separately. */
#define EXAMPLE_MIX_EVENT_BYTES 65536UL /* Bound aggregate mixer object/slots/adopted voices. */
#define EXAMPLE_MIX_QUEUE_FRAMES 32U /* Hold both legacy 16-frame exports and the gated 17-frame sequence. */
#define EXAMPLE_MIX_GATE_FRAMES 8U /* Hold base for two explicit four-frame event-only blocks. */
#define EXAMPLE_MIX_GATED_TOTAL 17U /* Eight held frames plus nine converted base frames. */
#define EXAMPLE_MIX_OPEN_FRAME 1U /* Authorize the embedded source's only movie frame. */
#define EXAMPLE_MIX_LONG_ID 8U /* The embedded event with the longer converted output interval. */
#endif /* End example-only geometry. */
