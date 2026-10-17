/* Tutorial-local streaming PCM input, output and bounded buffer geometry. */
#ifndef TUTORIAL_SWF_STREAM_CONSTANTS_H /* Guard standalone C89 tutorial constants. */
#define TUTORIAL_SWF_STREAM_CONSTANTS_H /* Keep source and bridge geometry consistent. */
#define TUTORIAL_STREAM_ARGUMENT_COUNT 2 /* Program and one explicitly replaceable output path. */
#define TUTORIAL_STREAM_CHANNELS 2U /* The literal source contains interleaved stereo frames. */
#define TUTORIAL_STREAM_CHUNK_FRAMES 1U /* Split the first two-frame block into separate host writes. */
#endif /* Finish the example-local bounds. */
