/* FlashVM: Interpreter state, object lifetimes and shared runtime helpers: exception internal.
 * File: src/core/exception_internal.hpp.
 */

#ifndef FVM_EXCEPTION_INTERNAL_HPP
/* Define FVM EXCEPTION INTERNAL HPP constant or compilation switch. */
#define FVM_EXCEPTION_INTERNAL_HPP
#include "runtime.hpp"


/* Implement resource fault for this subsystem. */
void fvm_resource_fault(FVM_Context *, unsigned long, unsigned long, const char *, const char *);


/* Implement resource fault resource for this subsystem. */
void fvm_resource_fault_resource(FVM_Context *, unsigned long kind, void *, unsigned long origin);


/* Implement resource fault begin for this subsystem. */
void fvm_resource_fault_begin(FVM_Context *);


/* Implement exception complete for this subsystem. */
FVM_Result fvm_exception_complete(FVM_Context *, FVM_Object *, FVM_Result);
#endif
