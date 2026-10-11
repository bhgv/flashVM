#ifndef FVM_STANDARD_DATE_STATE_HPP
#define FVM_STANDARD_DATE_STATE_HPP
#include "../core/runtime.hpp"
FVM_Result fvm_date_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                          fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);
FVM_Result fvm_date_validate(FVM_Context *);
unsigned long fvm_date_callback_id(FVM_NativeFunction);
FVM_NativeFunction fvm_date_callback_from_id(unsigned long);
#endif
