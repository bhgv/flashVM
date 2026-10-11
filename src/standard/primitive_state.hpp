#ifndef FVM_STANDARD_PRIMITIVE_STATE_HPP
#define FVM_STANDARD_PRIMITIVE_STATE_HPP
#include "flashvm/value.h"
FVM_Result fvm_to_primitive(FVM_Context *, const FVM_Value *, bool string_hint, FVM_Value *);
#endif
