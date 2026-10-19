/* Tutorial constants: exact native work accounting is described in README.md. */
#ifndef FVM_TUTORIAL_MOVIECLIP_LIMITS_H /* Keep standalone example configuration local. */
#define FVM_TUTORIAL_MOVIECLIP_LIMITS_H /* Prevent duplicate inclusion. */
#define FVM_TUTORIAL_TICK_WORK_LIMIT 5UL /* A node entry and optional handler cost two units per tick. */
#define FVM_TUTORIAL_TICK_BATCH 3U /* The third handler exceeds five work units. */
#endif /* End the example's private constants. */
