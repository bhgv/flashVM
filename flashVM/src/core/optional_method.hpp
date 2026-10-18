/* FlashVM: optional event methods reuse the rooted C method dispatcher without global fallback. */
#ifndef FVM_CORE_OPTIONAL_METHOD_HPP
#define FVM_CORE_OPTIONAL_METHOD_HPP /* Guard the private notification-call bridge. */
#include "runtime.hpp"
/* Resolve a handler once, preserving receiver/owner/GC behavior; ignore only missing/noncallable
 * members. */
extern "C" FVM_Result fvm_object_call_optional(FVM_Context *, FVM_Object *, const char *);
#endif
