/* FlashVM: explicit scheduler limits, version gates and append-only native graph IDs. */
#ifndef FVM_STANDARD_TIMER_CONSTANTS_H
#define FVM_STANDARD_TIMER_CONSTANTS_H /* Guard the scheduler constants. */
#define FVM_TIMERS_MAX_PENDING 1024U /* Bound context-local pending callbacks and poll snapshots. */
#define FVM_TIMERS_MAX_ARGUMENTS 256U /* Bound retained callback arguments independently of stack limits. */
#define FVM_TIMERS_MIN_DELAY 1.0 /* Shortest supported interval in monotonic milliseconds. */
#define FVM_TIMERS_MAX_DELAY 2147483647.0 /* Largest accepted integral delay in milliseconds. */
#define FVM_TIMERS_MAX_ID 4294967295UL /* Exact Number identifier; never reused within a context. */
#define FVM_TIMERS_INTERVAL_VERSION 6U /* setInterval and clearInterval availability. */
#define FVM_TIMERS_TIMEOUT_VERSION 8U /* setTimeout and clearTimeout availability. */
#define FVM_TIMERS_SET_INTERVAL_CALLBACK 164UL /* Append after getTimer's graph ID 163. */
#define FVM_TIMERS_CLEAR_INTERVAL_CALLBACK 165UL /* Stable cancellation callback identity. */
#define FVM_TIMERS_SET_TIMEOUT_CALLBACK 166UL /* Stable one-shot registration identity. */
#define FVM_TIMERS_CLEAR_TIMEOUT_CALLBACK 167UL /* Stable one-shot cancellation identity. */
#define FVM_TIMERS_ARGUMENT_OFFSET 2U /* Target and method precede retained user arguments. */
#endif
