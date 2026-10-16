/* FlashVM: temporary root bounds for nested invocation identity. */
#ifndef FVM_CORE_CALL_SCOPE_CONSTANTS_H
#define FVM_CORE_CALL_SCOPE_CONSTANTS_H /* Invocation snapshot bounds guard. */
#include "numeric_constants.h"
/* [GC:] Constructor, function and owner identities for both saved and active calls. */
#define FVM_CALL_SCOPE_SNAPSHOT_COUNT 6U
/* Sentinel requesting ordinary owner-based superclass depth inference. */
#define FVM_CALL_SCOPE_INFER_SUPER_DEPTH (FVM_PROTOTYPE_MAX_DEPTH + 1U)
#endif
