/* FlashVM: Built-in ActionScript objects: string state.
 * File: src/standard/string_state.hpp.
 */

#ifndef FVM_STANDARD_STRING_STATE_HPP
/* Define FVM STANDARD STRING STATE HPP constant or compilation switch. */
#define FVM_STANDARD_STRING_STATE_HPP
#include "../core/runtime.hpp"
/* Build constructor and prototype for string. */
FVM_Result fvm_string_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                            fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);

/* Implement string case for this subsystem. */
FVM_Result fvm_string_case(FVM_Context *, const FVM_Value &, FVM_Value *, bool);

/* Validate string. */
FVM_Result fvm_string_validate(FVM_Context *);

/* Prepared static attributes are shared with the transaction coordinator. */
struct FVM_MethodAttributeActivation;
/* Prepare fromCharCode attributes independently of public prototype state; may throw. */
void fvm_string_prepare_class_loaded(FVM_Context *, FVM_MethodAttributeActivation &);

/* Convert an untouched host String prototype before successful SWF publication; no callbacks. */
FVM_Result fvm_string_activate_loaded(FVM_Context *);

/* Map a native string callback to its stable serialization identifier. */
unsigned long fvm_string_callback_id(FVM_NativeFunction);

/* Restore a native string callback from its stable serialization identifier. */
FVM_NativeFunction fvm_string_callback_from_id(unsigned long);
#endif
