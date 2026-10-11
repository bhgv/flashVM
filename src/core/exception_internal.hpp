#ifndef FVM_EXCEPTION_INTERNAL_HPP
#define FVM_EXCEPTION_INTERNAL_HPP
#include "runtime.hpp"
void fvm_resource_fault(FVM_Context *, unsigned long, unsigned long, const char *, const char *);
void fvm_resource_fault_resource(FVM_Context *, unsigned long kind, void *, unsigned long origin);
void fvm_resource_fault_begin(FVM_Context *);
FVM_Result fvm_exception_complete(FVM_Context *, FVM_Object *, FVM_Result);
#endif
