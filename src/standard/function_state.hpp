/* FlashVM: Built-in ActionScript objects: function state.
 * File: src/standard/function_state.hpp.
 */

#ifndef FVM_STANDARD_FUNCTION_STATE_HPP
/* Define FVM STANDARD FUNCTION STATE HPP constant or compilation switch. */
#define FVM_STANDARD_FUNCTION_STATE_HPP
#include "../core/runtime.hpp"
/* Build constructor and prototype for function. */
FVM_Result fvm_function_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                              fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);

/* Validate function. */
FVM_Result fvm_function_validate(FVM_Context *);

/* Implement function wrap entry for this subsystem. */
FVM_Result fvm_function_wrap_entry(FVM_Context *, FVM_Object *, const char *,
                                   const FVM_NativeEntry &, FVM_Value *);

/* Implement function make script for this subsystem. */
FVM_Result fvm_function_make_script(FVM_Context *, const FVM_ScriptFunction &, FVM_Object *,
                                    FVM_Object **);

/* Invoke avm1 script. */
FVM_Result fvm_avm1_invoke_script(FVM_Context *, const FVM_ScriptFunction &, FVM_Object *,
                                  FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *,
                                  FVM_Object *);

/* Map a native function callback to its stable serialization identifier. */
unsigned long fvm_function_callback_id(FVM_NativeFunction);

/* Restore a native function callback from its stable serialization identifier. */
FVM_NativeFunction fvm_function_callback_from_id(unsigned long);

/* Implement function collector callback for this subsystem. */
bool fvm_function_collector_callback(FVM_NativeFunction);
#endif
