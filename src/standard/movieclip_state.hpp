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
/* Read intrinsic parent/root for AVM1 preloads, bypassing replaceable script properties. */
FVM_Result fvm_movieclip_preload_display(FVM_Context *, FVM_Object *, bool parent, FVM_Value *);

/* Map a native movieclip callback to its stable serialization identifier. */
unsigned long fvm_movieclip_callback_id(FVM_NativeFunction);

/* Restore a native movieclip callback from its stable serialization identifier. */
FVM_NativeFunction fvm_movieclip_callback_from_id(unsigned long);
/* Read a supported intrinsic display property by the classic ActionGetProperty index. */
FVM_Result fvm_movieclip_get_action_property(FVM_Context *, FVM_Object *, unsigned int, FVM_Value *);
/* Publish an already-coerced intrinsic scalar; unsupported dimensions/system fields stay gated. */
FVM_Result fvm_movieclip_set_action_property(FVM_Context *, FVM_Object *, unsigned int,
                                            const FVM_Value &);
/* Apply a timeline Action control directly, independently from replaceable script methods. */
FVM_Result fvm_movieclip_frame_control(FVM_Context *, FVM_Object *, unsigned int,
                                     const FVM_Value *, bool playing);
/* Resolve display children and object properties along a counted AVM1 path. */
FVM_Result fvm_movieclip_resolve_path(FVM_Context *, FVM_Object *, const FVM_Value &, FVM_Object **);
/* Build the current counted AVM1 dot path directly from display state. */
FVM_Result fvm_movieclip_reference_string(FVM_Context *, FVM_Object *, FVM_Value *);
/* Run only a selected local frame's actions without moving the playhead or applying scene controls.
 * Number or counted label input is already coerced; AVM1 resolves target prefixes first. */
FVM_Result fvm_movieclip_call_frame(FVM_Context *, FVM_Object *, const FVM_Value &);
#endif
