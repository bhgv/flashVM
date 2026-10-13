/* FlashVM: version-aware numeric coercion for AVM1 actions, separate from host C conversion. */
#ifndef FVM_AVM1_COERCION_HPP
#define FVM_AVM1_COERCION_HPP /* Guard for the internal AVM1 conversion contract. */
#include "../core/runtime.hpp"
/* Resolve one object method with AVM1 numeric/default-hint rules, preserving object results. */
FVM_Result fvm_avm1_primitive(FVM_Context *, const FVM_Value *, bool default_hint, FVM_Value *);
/* Convert an action operand using loaded SWF rules; unloaded contexts retain host semantics. */
FVM_Result fvm_avm1_number(FVM_Context *, const FVM_Value *, double *);
#endif
