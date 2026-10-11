#ifndef FVM_STANDARD_FUNCTION_STATE_HPP
#define FVM_STANDARD_FUNCTION_STATE_HPP
#include "../core/runtime.hpp"
FVM_Result fvm_function_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                              fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);
FVM_Result fvm_function_validate(FVM_Context *);
FVM_Result fvm_function_wrap_entry(FVM_Context *, FVM_Object *, const char *,
                                   const FVM_NativeEntry &, FVM_Value *);
FVM_Result fvm_function_make_script(FVM_Context *, const FVM_ScriptFunction &, FVM_Object *,
                                    FVM_Object **);
FVM_Result fvm_avm1_invoke_script(FVM_Context *, const FVM_ScriptFunction &, FVM_Object *,
                                  FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *,
                                  FVM_Object *);
unsigned long fvm_function_callback_id(FVM_NativeFunction);
FVM_NativeFunction fvm_function_callback_from_id(unsigned long);
bool fvm_function_collector_callback(FVM_NativeFunction);
#endif
