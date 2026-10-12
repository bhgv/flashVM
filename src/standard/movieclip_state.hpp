/* FlashVM: Built-in ActionScript objects: movieclip state.
 * File: src/standard/movieclip_state.hpp.
 */

#ifndef FVM_MOVIECLIP_STATE_HPP
/* Define FVM MOVIECLIP STATE HPP constant or compilation switch. */
#define FVM_MOVIECLIP_STATE_HPP
#include "../core/runtime.hpp"
/* Build constructor and prototype for movieclip. */
FVM_Result fvm_movieclip_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                               fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);

/* Validate movieclip. */
FVM_Result fvm_movieclip_validate(FVM_Context *);

/* Map a native movieclip callback to its stable serialization identifier. */
unsigned long fvm_movieclip_callback_id(FVM_NativeFunction);

/* Restore a native movieclip callback from its stable serialization identifier. */
FVM_NativeFunction fvm_movieclip_callback_from_id(unsigned long);
#endif
