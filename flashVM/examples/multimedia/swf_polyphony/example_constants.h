/* Tutorial constants: explicit event ordinals and independent host output geometry. */
#ifndef TUTORIAL_SWF_POLYPHONY_CONSTANTS_H /* Guard these example-only identifiers. */
#define TUTORIAL_SWF_POLYPHONY_CONSTANTS_H /* Keep the tutorial header valid in C89. */
#define TUTORIAL_SWF_VOICE_COUNT 3U /* Two resource-7 voices and one independently selected output. */
#define TUTORIAL_SWF_ARGUMENT_COUNT 4 /* Program name followed by three explicit output paths. */
#define TUTORIAL_SWF_EVENT_START 0U /* Root event starting resource 7. */
#define TUTORIAL_SWF_EVENT_NO_MULTIPLE 1U /* Root event suppressing duplicate resource-7 playback. */
#define TUTORIAL_SWF_EVENT_STOP 2U /* Root event stopping every bound resource-7 voice. */
#define TUTORIAL_SWF_EVENT_OTHER 3U /* Root event starting separate resource 8. */
#endif /* Finish example-local constants. */
