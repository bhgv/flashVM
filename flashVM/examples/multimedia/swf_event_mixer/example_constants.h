/* Tutorial constants describe fixture events separately from mixer/output geometry. */
#ifndef FVM_EXAMPLE_EVENT_MIXER_H  /* Avoid duplicate tutorial declarations. */
#define FVM_EXAMPLE_EVENT_MIXER_H  /* Begin the private example header. */
#define EXAMPLE_RATE 44100U        /* Both U8 fixture resources use this nominal clock. */
#define EXAMPLE_CHANNELS 1U        /* The demonstration writes native mono S16. */
#define EXAMPLE_VOICES 3U          /* Two ID-7 voices plus one independent ID-8 voice. */
#define EXAMPLE_START 0U           /* Start an ordinary copy of resource 7. */
#define EXAMPLE_NO_MULTIPLE 1U     /* Suppress another copy while ID 7 is active. */
#define EXAMPLE_STOP 2U            /* Stop every resource-7 voice. */
#define EXAMPLE_OTHER 3U           /* Start the separate resource 8. */
#define EXAMPLE_TAIL_FRAMES 3U     /* Read one remaining sample followed by two silence frames. */
#define EXAMPLE_OUTPUT_RATE 48000U /* Explicit converted output clock for the third demo mode. */
#define EXAMPLE_OUTPUT_CHANNELS 2U /* Bound caller storage for mono and stereo output modes. */
#endif                             /* Finish the tutorial constants. */
