/* FlashVM: Built-in ActionScript objects: error state.
 * File: src/standard/error_state.hpp.
 */

#ifndef FVM_STANDARD_ERROR_STATE_HPP
/* Define FVM STANDARD ERROR STATE HPP constant or compilation switch. */
#define FVM_STANDARD_ERROR_STATE_HPP
#include "../core/runtime.hpp"

/* Error has no opaque native state: ordinary properties and prototype edges
 * remain editable, GC-traced and transferable as a graph. */
/* Build constructor and prototype for error. */
FVM_Result fvm_error_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
                           fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors);

/* Validate error. */
FVM_Result fvm_error_validate(FVM_Context *);

/* Implement error attach for this subsystem. */
void fvm_error_attach(FVM_Context *, FVM_Object *);

/* Map a native error callback to its stable serialization identifier. */
unsigned long fvm_error_callback_id(FVM_NativeFunction);

/* Restore a native error callback from its stable serialization identifier. */
FVM_NativeFunction fvm_error_callback_from_id(unsigned long);
#endif
