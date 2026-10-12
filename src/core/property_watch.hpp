#ifndef FVM_CORE_PROPERTY_WATCH_HPP
#define FVM_CORE_PROPERTY_WATCH_HPP
#include "runtime.hpp"
bool fvm_property_watchable(FVM_Object *, const char *);
FVM_Result fvm_property_watch_prepare(FVM_Context *, FVM_Object *, const char *, const FVM_Value *,
                                      FVM_Value *);
#endif
