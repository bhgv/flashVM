/* Finite host policy for native identity and a caller-owned test FIFO; not SWF format bounds. */
#ifndef EXAMPLE_QUEUED_IDENTITY_H /* Guard detachable tutorial constants. */
#define EXAMPLE_QUEUED_IDENTITY_H /* Name ownership and output limits separately. */
#define EXAMPLE_ID_SOUND 7U /* Dictionary identity in the independent embedded source. */
#define EXAMPLE_ID_RATE 48000U /* Explicit queue input clock. */
#define EXAMPLE_ID_CHANNELS 2U /* Interleaved native S16 output channels. */
#define EXAMPLE_ID_VOICES 1U /* Only one software voice may occupy the native mixer. */
#define EXAMPLE_ID_BLOCK 16U /* One complete native preparation/retry block. */
#define EXAMPLE_ID_TARGET 64U /* Maximum exclusive FIFO backlog. */
#define EXAMPLE_ID_VALUES 128U /* Fixed caller-owned accepted-PCM evidence capacity. */
#define EXAMPLE_ID_MIXER_BYTES 32768UL /* Mixer object/slots and retained/incoming voice ceiling. */
#define EXAMPLE_ID_RUNTIME_BYTES (1024UL * 1024UL) /* Separate source runtime payload policy. */
#define EXAMPLE_ID_END 8U /* ceil(7*48000/44100), independent rational source-duration oracle. */
#define EXAMPLE_ID_EXPECTED_VALUES 64U /* Two accepted blocks times stereo interleaving. */
#endif /* End example-only native policy. */
