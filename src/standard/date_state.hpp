/* FlashVM: Built-in ActionScript objects: date state.
 * File: src/standard/date_state.hpp.
 */

#ifndef FVM_STANDARD_DATE_STATE_HPP
/* Define FVM STANDARD DATE STATE HPP constant or compilation switch. */
#define FVM_STANDARD_DATE_STATE_HPP
#include "../core/runtime.hpp"
/* Build constructor and prototype for date. */
FVM_Result fvm_date_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                          fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);

/* [GC:] Prepare the rooted invalid host prototype for atomic loaded activation; may throw. */
FVM_Object *fvm_date_prepare_loaded(FVM_Context *);

/* Validate date. */
FVM_Result fvm_date_validate(FVM_Context *);

/* Map a native date callback to its stable serialization identifier. */
unsigned long fvm_date_callback_id(FVM_NativeFunction);

/* Restore a native date callback from its stable serialization identifier. */
FVM_NativeFunction fvm_date_callback_from_id(unsigned long);
#endif
