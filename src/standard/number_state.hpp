#ifndef FVM_STANDARD_NUMBER_STATE_HPP
#define FVM_STANDARD_NUMBER_STATE_HPP
#include "../core/runtime.hpp"
FVM_Result fvm_number_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                            fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);
FVM_Result fvm_number_validate(FVM_Context *);
unsigned long fvm_number_callback_id(FVM_NativeFunction);
FVM_NativeFunction fvm_number_callback_from_id(unsigned long);
#endif
