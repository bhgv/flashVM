/* FlashVM: Interpreter state, object lifetimes and shared runtime helpers: property watch.
 * File: src/core/property_watch.hpp.
 */

#ifndef FVM_CORE_PROPERTY_WATCH_HPP
/* Define FVM CORE PROPERTY WATCH HPP constant or compilation switch. */
#define FVM_CORE_PROPERTY_WATCH_HPP
#include "runtime.hpp"
/* Implement property watchable for this subsystem. */
bool fvm_property_watchable(FVM_Object *, const char *);

/* Implement property watch prepare for this subsystem. */
FVM_Result fvm_property_watch_prepare(FVM_Context *, FVM_Object *, const char *, const FVM_Value *,
                                      FVM_Value *);
#endif
