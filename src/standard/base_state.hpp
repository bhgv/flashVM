#ifndef FVM_STANDARD_BASE_STATE_HPP
#define FVM_STANDARD_BASE_STATE_HPP
#include "../core/runtime.hpp"
FVM_Result fvm_base_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                          fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &,
                          fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);
FVM_Result fvm_base_validate(FVM_Context *);
unsigned long fvm_base_callback_id(FVM_NativeFunction);
FVM_NativeFunction fvm_base_callback_from_id(unsigned long);
bool fvm_as_index(const char *, unsigned long *);
void fvm_array_note_index(FVM_Object *, unsigned long);
FVM_Result fvm_array_create_like(FVM_Context *, FVM_Object *, unsigned long, FVM_Object **);
#endif
