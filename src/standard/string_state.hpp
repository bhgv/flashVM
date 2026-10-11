#ifndef FVM_STANDARD_STRING_STATE_HPP
#define FVM_STANDARD_STRING_STATE_HPP
#include "../core/runtime.hpp"
FVM_Result fvm_string_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                            fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);
FVM_Result fvm_string_validate(FVM_Context *);
unsigned long fvm_string_callback_id(FVM_NativeFunction);
FVM_NativeFunction fvm_string_callback_from_id(unsigned long);
#endif
