/* FlashVM: version-aware primitive/string/numeric coercion for AVM1 actions, separate from host C conversion. */
#ifndef FVM_AVM1_COERCION_HPP
#define FVM_AVM1_COERCION_HPP /* Guard for the internal AVM1 conversion contract. */
#include "../core/runtime.hpp"
/* Resolve one object method with AVM1 numeric/default-hint rules, preserving object results. */
FVM_Result fvm_avm1_primitive(FVM_Context *, const FVM_Value *, bool default_hint, FVM_Value *);
/* Convert ordinary loaded-SWF text, including primitive spelling and one object toString call.
 * [GC:] The caller retains the input owner across method lookup/invocation. Trace has a separate
 * undefined spelling; unloaded contexts retain host conversion rules. */
FVM_Result fvm_avm1_string(FVM_Context *, const FVM_Value &, FVM_Value *);
/* Convert an action operand using loaded SWF rules; unloaded contexts retain host semantics. */
FVM_Result fvm_avm1_number(FVM_Context *, const FVM_Value *, double *);
/* Convert truth without invoking object methods; old loaded SWFs parse string numbers. */
FVM_Result fvm_avm1_boolean(FVM_Context *, const FVM_Value &, bool *);
#endif
