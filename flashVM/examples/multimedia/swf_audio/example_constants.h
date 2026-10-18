/* Tutorial-local input limits and bounded native PCM geometry. */
#ifndef TUTORIAL_SWF_AUDIO_CONSTANTS_H /* Guard example-local constants. */
#define TUTORIAL_SWF_AUDIO_CONSTANTS_H /* Keep limits valid for C89 callers. */
#define TUTORIAL_SWF_INPUT_LIMIT (64UL * 1024UL * 1024UL) /* Bound the explicit input-file allocation. */
#define TUTORIAL_SWF_SOUND_ID_MAX 65535UL /* Sound IDs are nonzero unsigned sixteen-bit dictionary keys. */
#define TUTORIAL_SWF_PCM_CHUNK 256U /* Submit small explicit sample-frame ranges instead of allocating an entire song. */
#define TUTORIAL_SWF_MAX_CHANNELS 2U /* Snapshot output is mono or interleaved stereo. */
#endif /* Finish tutorial-local constants. */
