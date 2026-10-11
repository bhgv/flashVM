#ifndef FVM_MOVIECLIP_STATE_HPP
#define FVM_MOVIECLIP_STATE_HPP
#include "../core/runtime.hpp"
FVM_Result fvm_movieclip_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                               fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);
FVM_Result fvm_movieclip_validate(FVM_Context *);
unsigned long fvm_movieclip_callback_id(FVM_NativeFunction);
FVM_NativeFunction fvm_movieclip_callback_from_id(unsigned long);
#endif
