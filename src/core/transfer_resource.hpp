#ifndef FVM_TRANSFER_RESOURCE_HPP
#define FVM_TRANSFER_RESOURCE_HPP
#include "runtime.hpp"
/* These tokens are scoped to one retained in-process packet, never durable IDs. */
FVM_Result fvm_transfer_capture_resource(FVM_Context *, FVM_Object *, unsigned long *);
FVM_Result fvm_transfer_restore_resource(FVM_Context *, unsigned long, unsigned long,
                                         FVM_Object **);
#endif
