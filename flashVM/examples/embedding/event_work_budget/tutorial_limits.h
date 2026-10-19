/* FlashVM tutorial: named bounds for exact, deterministic native event accounting. */
#ifndef FVM_EVENT_WORK_BUDGET_TUTORIAL_LIMITS_H /* Include these tutorial bounds once. */
#define FVM_EVENT_WORK_BUDGET_TUTORIAL_LIMITS_H /* Guard tutorial-only constants. */
#define EXAMPLE_WORK_LIMIT 5UL /* Three captures, publication and the first callback fit exactly. */
#define EXAMPLE_LISTENER_COUNT 3U /* The second callback exceeds the configured event budget. */
#endif /* End the example's private bounds. */
