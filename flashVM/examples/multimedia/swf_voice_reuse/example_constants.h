/* Finite caller policy for independent native replay; these are not SWF format limits. */
#ifndef EXAMPLE_VOICE_REUSE_CONSTANTS_H                      /* Guard the detachable example configuration. */
#define EXAMPLE_VOICE_REUSE_CONSTANTS_H                      /* Name every distinct host bound. */
#define EXAMPLE_REUSE_INPUT_BYTES (16UL * 1024UL * 1024UL)   /* Bound the copied input file. */
#define EXAMPLE_REUSE_NATIVE_BYTES (32UL * 1024UL * 1024UL)  /* Template plus mixer and incoming clone. */
#define EXAMPLE_REUSE_EXPORT_FRAMES (32UL * 1024UL * 1024UL) /* Bound the complete two-pass export. */
#define EXAMPLE_REUSE_PASSES 2U                              /* Replay the same decoded interval twice without another VM. */
#define EXAMPLE_REUSE_BLOCK 1024U                            /* Bound each native PCM copy independently of file buffering. */
#define EXAMPLE_REUSE_CHANNELS 2U                            /* Maximum supported source interleaving. */
#define EXAMPLE_REUSE_VOICES 1U                              /* At most one adopted replay owner exists at a time. */
#endif                                                       /* End the example-only policy. */
