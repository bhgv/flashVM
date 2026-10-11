#ifndef FVM_GLOBAL_PARSING_HPP
#define FVM_GLOBAL_PARSING_HPP
#include "flashvm/value.h"
FVM_Result fvm_builtin_parse_int(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                                 FVM_Value *, void *);
FVM_Result fvm_builtin_parse_float(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                                   FVM_Value *, void *);
FVM_Result fvm_builtin_string(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                              FVM_Value *, void *);
#endif
