/* Tutorial-local streaming PCM input, output and bounded buffer geometry. */
#ifndef TUTORIAL_SWF_STREAM_CONSTANTS_H /* Guard standalone C89 tutorial constants. */
#define TUTORIAL_SWF_STREAM_CONSTANTS_H /* Keep source and bridge geometry consistent. */
#define TUTORIAL_STREAM_ARGUMENT_COUNT 2 /* Program and one explicitly replaceable output path. */
#define TUTORIAL_STREAM_CHANNELS 2U /* The literal source contains interleaved stereo frames. */
#define TUTORIAL_STREAM_CHUNK_FRAMES 1U /* Split the first two-frame block into separate host writes. */
#define TUTORIAL_STREAM_OUTPUT_RATE 48000UL /* Explicit conversion target; source remains 44100 Hz. */
#define TUTORIAL_STREAM_QUEUE_FRAMES 2U /* Small explicit FIFO forces producer backpressure and drains. */
#endif /* Finish the example-local bounds. */
