/* FlashVM: worker cancellation polling during native execution work. */
#ifndef FVM_EXECUTION_WORK_CONSTANTS_H
#define FVM_EXECUTION_WORK_CONSTANTS_H /* Native work polling constants guard. */
#define FVM_EXECUTION_WORK_CANCEL_MASK 63UL /* Poll once per 64 charged units. */
#define FVM_EXECUTION_WORK_CANCEL_PHASE 1UL /* Poll the first unit in each period. */
#endif
