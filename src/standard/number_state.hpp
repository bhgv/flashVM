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

/* [GC:] Prepare the rooted host prototype for atomic loaded activation; may allocate/throw. */
FVM_Object *fvm_number_prepare_loaded(FVM_Context *);

/* Validate number. */
FVM_Result fvm_number_validate(FVM_Context *);

/* Map a native number callback to its stable serialization identifier. */
unsigned long fvm_number_callback_id(FVM_NativeFunction);

/* Restore a native number callback from its stable serialization identifier. */
FVM_NativeFunction fvm_number_callback_from_id(unsigned long);
#endif
