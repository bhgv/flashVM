/* FlashVM: AVM1 superclass facade creation, lookup and invocation. */
#ifndef FVM_STANDARD_SUPER_HPP
#define FVM_STANDARD_SUPER_HPP /* Include guard for superclass facades. */
#include "../core/runtime.hpp"

/* [GC:] Build a facade retaining the actual receiver and selected parent prototype. */
FVM_Result fvm_super_create(FVM_Context *, FVM_Object *, FVM_Object *, FVM_Value *);
/* Query own membership on the retained receiver without reading property values. */
FVM_Result fvm_super_has_own(FVM_Context *, FVM_Object *, const fvm_detail::string &, int *);
/* Read a parent property while binding accessors to the actual receiver. */
FVM_Result fvm_super_get(FVM_Context *, FVM_Object *, const fvm_detail::string &, FVM_Value *);
/* Write through parent setters while keeping the actual receiver. */
FVM_Result fvm_super_set(FVM_Context *, FVM_Object *, const fvm_detail::string &, const FVM_Value *);
/* Call a parent method with the actual receiver and its defining prototype owner. */
FVM_Result fvm_super_method(FVM_Context *, FVM_Object *, const fvm_detail::string &, unsigned int,
                            const FVM_Value *, FVM_Value *);
/* Invoke an AVM1 parent member with undefined for missing/noncallable values. */
FVM_Result fvm_super_invoke(FVM_Context *, FVM_Object *, const fvm_detail::string &, unsigned int,
                            const FVM_Value *, FVM_Value *);
/* Invoke an optional conversion member with the original receiver and defining owner. */
FVM_Result fvm_super_special(FVM_Context *, FVM_Object *, const fvm_detail::string &, FVM_Value *);
/* Recognize the native facade constructor callback for graph serialization. */
FVM_NativeFunction fvm_super_callback();
/* Read from a selected prototype chain with a separate accessor receiver. */
FVM_Result fvm_object_get_from(FVM_Context *, FVM_Object *, FVM_Object *, const fvm_detail::string &, FVM_Value *,
                               bool, bool allow_resolve = true);
/* Write through a selected accessor chain; prepared_watch avoids repeating an indexed watcher.
 * super_write suppresses loaded facade storage and gates lookup on receiver membership. */
FVM_Result fvm_object_set_from(FVM_Context *, FVM_Object *, FVM_Object *, const fvm_detail::string &,
                               const FVM_Value *, bool, bool prepared_watch = false, bool super_write = false);
#endif
