/* FlashVM: Interpreter state, object lifetimes and shared runtime helpers: transfer resource.
 * File: src/core/transfer_resource.hpp.
 */

#ifndef FVM_TRANSFER_RESOURCE_HPP
/* Define FVM TRANSFER RESOURCE HPP constant or compilation switch. */
#define FVM_TRANSFER_RESOURCE_HPP
#include "runtime.hpp"

/* These tokens are scoped to one retained in-process packet, never durable IDs. */


/* Capture transfer resource. */
FVM_Result fvm_transfer_capture_resource(FVM_Context *, FVM_Object *, unsigned long *);


/* Restore transfer resource. */
FVM_Result fvm_transfer_restore_resource(FVM_Context *, unsigned long, unsigned long, FVM_Object **);
#endif
