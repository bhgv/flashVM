/* FlashVM: Built-in ActionScript objects: number state.
 * File: src/standard/number_state.hpp.
 */

#ifndef FVM_STANDARD_NUMBER_STATE_HPP
/* Define FVM STANDARD NUMBER STATE HPP constant or compilation switch. */
#define FVM_STANDARD_NUMBER_STATE_HPP
#include "../core/runtime.hpp"
/* Build constructor and prototype for number. */
FVM_Result fvm_number_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                            fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);

/* [GC:] Prepared Number state and attribute maps are retained by the constructor capture. */
struct FVM_NumberPrototypeActivation
{
    FVM_Object *prototype; /* Unchanged zero-valued host prototype selected for activation. */
    FVM_Object *klass; /* Captured class containing canonical numeric constants. */
    fvm_detail::map<fvm_detail::string, unsigned int> prototype_attributes; /* Prepared method flags. */
    fvm_detail::map<fvm_detail::string, unsigned int> class_attributes; /* Prepared constant flags. */
    /* Start without a candidate; temporary maps own only names and numeric flags. */
    FVM_NumberPrototypeActivation() : prototype(0), klass(0) {}
};
/* Prepare the rooted host prototype and default flags without mutation; may throw. */
void fvm_number_prepare_loaded(FVM_Context *, FVM_NumberPrototypeActivation &);
/* Publish the prepared generic layout and flag maps without allocation or callbacks. */
void fvm_number_commit_loaded(FVM_NumberPrototypeActivation &);

/* Validate number. */
FVM_Result fvm_number_validate(FVM_Context *);

/* Map a native number callback to its stable serialization identifier. */
unsigned long fvm_number_callback_id(FVM_NativeFunction);

/* Restore a native number callback from its stable serialization identifier. */
FVM_NativeFunction fvm_number_callback_from_id(unsigned long);
#endif
