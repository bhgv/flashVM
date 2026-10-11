#ifndef FVM_STANDARD_ERROR_STATE_HPP
#define FVM_STANDARD_ERROR_STATE_HPP
#include "../core/runtime.hpp"

/* Error has no opaque native state: ordinary properties and prototype edges
 * remain editable, GC-traced and transferable as a graph. */
FVM_Result fvm_error_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
                           fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors);
FVM_Result fvm_error_validate(FVM_Context *);
void fvm_error_attach(FVM_Context *, FVM_Object *);
unsigned long fvm_error_callback_id(FVM_NativeFunction);
FVM_NativeFunction fvm_error_callback_from_id(unsigned long);
#endif
