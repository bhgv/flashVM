/* FlashVM: context-owned scheduled calls, separate from serializable Function graphs. */
#ifndef FVM_CORE_TIMER_STATE_HPP
#define FVM_CORE_TIMER_STATE_HPP /* Guard the private scheduled-call representation. */
#include "flashvm/value.h"
#include "../details/vector.hpp"
/* [GC:] One scheduled call strongly retains every value until cancellation or consumption. */
struct FVM_ScheduledCall
{
    fvm_detail::vector<FVM_Value> values; /* [GC:] Target, method/undefined, then copied callback arguments. */
    double deadline; /* Unwrapped context milliseconds at which delivery becomes eligible. */
    double delay; /* Positive integer milliseconds used to rearm an interval. */
    bool repeat; /* Rearm before invocation when true; otherwise consume the timeout. */
    bool method; /* Resolve values[1] on object values[0] at delivery, retaining its receiver. */
    /* Initialize unpublished timing state; publication requires a complete rooted value vector. */
    FVM_ScheduledCall() : deadline(0), delay(0), repeat(false), method(false) {}
};
#endif
