/* FlashVM: strong graph-owned TextField listeners and prepared notification snapshots. */
#ifndef FVM_STANDARD_TEXT_LISTENERS_HPP
#define FVM_STANDARD_TEXT_LISTENERS_HPP /* Guard private C++98 text event operations. */
#include "../core/runtime.hpp"
#define FVM_TEXT_LISTENER_KEY "\001FlashVM.TextField.listeners" /* Private nonenumerable managed registry. */
#define FVM_TEXT_LISTENER_MAX_COUNT 4096UL /* Bounded registration and recipient snapshot work. */
#define FVM_TEXT_LISTENER_MIN_VERSION 6U /* TextField listener events begin in SWF6. */
#define FVM_TEXT_LISTENER_CALLBACK_ADD 207UL /* Stable text callback graph identity. */
#define FVM_TEXT_LISTENER_CALLBACK_REMOVE 208UL /* Stable text callback graph identity. */
/* Register a same-context identity once; publish no partial registry on allocation failure. */
FVM_Result fvm_textfield_add_listener(FVM_Context *, FVM_Object *, unsigned int,
                                     const FVM_Value *, FVM_Value *, void *);
/* Remove one identity transactionally; report false when absent or invalid script arguments. */
FVM_Result fvm_textfield_remove_listener(FVM_Context *, FVM_Object *, unsigned int,
                                        const FVM_Value *, FVM_Value *, void *);
/* Validate and borrow the private compact managed Array; NOT_FOUND when never registered. */
FVM_Result fvm_textfield_listener_registry(FVM_Context *, FVM_Object *, FVM_Object **);
#endif
