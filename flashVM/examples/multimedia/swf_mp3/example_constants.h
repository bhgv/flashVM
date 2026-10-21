/* Explicit tutorial ownership and export ceilings; these are host policy, not SWF limits. */
#ifndef EXAMPLE_MP3_CONSTANTS_H /* Guard standalone example bounds. */
#define EXAMPLE_MP3_CONSTANTS_H /* Keep sample and memory limits distinct. */
#define EXAMPLE_MP3_INPUT_BYTES (16UL * 1024UL * 1024UL) /* Maximum source file copied by this host. */
#define EXAMPLE_MP3_NATIVE_BYTES (32UL * 1024UL * 1024UL) /* Native snapshot/runtime payload ceiling. */
#define EXAMPLE_MP3_EXPORT_FRAMES (32UL * 1024UL * 1024UL) /* Stop unbounded loop-driven export. */
#define EXAMPLE_MP3_BLOCK 1024U /* Bounded native frame copy, independent of file buffering. */
#define EXAMPLE_MP3_VOICES 1U /* This exporter authorizes one adopted event voice at a time. */
#define EXAMPLE_MP3_CHANNELS 2U /* Largest supported MPEG interleaving. */
#endif /* End example-only configuration. */
