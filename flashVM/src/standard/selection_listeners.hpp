/* FlashVM: context-owned Selection observer methods and inspection snapshots. */
#ifndef FVM_STANDARD_SELECTION_LISTENERS_HPP
#define FVM_STANDARD_SELECTION_LISTENERS_HPP /* Guard the private native namespace entries. */
#include "../core/runtime.hpp"

/* Bound registered identities independently of native memory exhaustion. */
#define FVM_SELECTION_MAX_LISTENERS 4096UL
/* Register a same-context object once; non-object script arguments are ignored. */
FVM_Result
fvm_selection_add_listener(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);
/* Remove one identity without allocation and report whether it was present. */
FVM_Result
fvm_selection_remove_listener(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);
/* [GC:] Return a fresh, detached Array containing the current strong registrations. */
FVM_Result
fvm_selection_listener_array(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);
#endif
