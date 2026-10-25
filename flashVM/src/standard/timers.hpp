/* FlashVM: private built-in scheduling callbacks and context teardown. */
#ifndef FVM_STANDARD_TIMERS_HPP
#define FVM_STANDARD_TIMERS_HPP /* Guard private timer entry declarations. */
#include "flashvm/value.h"


/* Register a repeating Function or a named object method for explicit host polling. */
FVM_Result fvm_builtin_set_interval(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);
/* Cancel a context-local ID; absent or unknown IDs have no effect. */
FVM_Result
fvm_builtin_clear_interval(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);


/* Register a one-shot Function callback; the object/method overload is interval-only. */
FVM_Result fvm_builtin_set_timeout(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);


/* Cancel a context-local scheduled ID under SWF8 availability. */
FVM_Result fvm_builtin_clear_timeout(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);


/* [GC:] Remove all retained scheduler values before object finalization at context destruction. */
void fvm_timers_cleanup(FVM_Context *);


/* [GC:] These runtime callbacks retain coercible arguments through collecting script calls. */
inline bool fvm_timers_collector_callback(FVM_NativeFunction callback)
{
    return callback == fvm_builtin_set_interval || callback == fvm_builtin_clear_interval ||
           callback == fvm_builtin_set_timeout || callback == fvm_builtin_clear_timeout;
}
#endif
