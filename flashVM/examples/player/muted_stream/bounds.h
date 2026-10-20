/* Fixed tutorial input budget; no dependency on private VM headers. */
#ifndef FVM_EXAMPLE_MUTED_STREAM_BOUNDS_H /* Keep the example-owned limit local. */
#define FVM_EXAMPLE_MUTED_STREAM_BOUNDS_H /* Guard repeated inclusion. */
#define FVM_EXAMPLE_MUTED_STREAM_INPUT_LIMIT (64L * 1024L * 1024L) /* Maximum regular-file bytes. */
#endif /* End the standalone example bounds. */
