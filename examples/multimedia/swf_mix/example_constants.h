/* Tutorial-local bounds for two independently prepared voices and small mixed output. */
#ifndef TUTORIAL_SWF_MIX_CONSTANTS_H /* Guard example-local native buffer geometry. */
#define TUTORIAL_SWF_MIX_CONSTANTS_H /* Keep this companion header valid in C89. */
#define TUTORIAL_MIX_MODE_ARGUMENT_COUNT 3 /* Program, explicit output path and optional execution-mode switch. */
#define TUTORIAL_MIX_ARGUMENT_COUNT 2 /* Program name followed by one explicit mixed-output path. */
#define TUTORIAL_MIX_INPUT_COUNT 2U /* Prepare the fixture's two root StartSound controls. */
#define TUTORIAL_MIX_CHUNK_FRAMES 2U /* Make unequal voice endings visible across tiny caller buffers. */
#define TUTORIAL_MIX_MAX_CHANNELS 2U /* Bound both mono and stereo caller-owned storage. */
#define TUTORIAL_MIX_STOP_NATIVE_ENTRIES 1UL /* One actual StopSounds function entry is required when JIT is available. */
#endif /* Finish tutorial-local bounds. */
