/* Tutorial constants: exact native work accounting is described in README.md. */
#ifndef FVM_TUTORIAL_MOVIECLIP_LIMITS_H /* Keep standalone example configuration local. */
#define FVM_TUTORIAL_MOVIECLIP_LIMITS_H /* Prevent duplicate inclusion. */
#define FVM_TUTORIAL_TICK_WORK_LIMIT 5UL /* A node entry and optional handler cost two units per tick. */
#define FVM_TUTORIAL_TICK_BATCH 3U /* The third handler exceeds five work units. */
#define FVM_TUTORIAL_DEPTH_WORK_LIMIT 64UL /* Leave room for three children, sorting and eligibility checks. */
#define FVM_TUTORIAL_DEPTH_CHILDREN 3U /* The depth-order demonstration records exactly three child callbacks. */
#endif /* End the example's private constants. */
