/* FlashVM: Built-in ActionScript objects: movieclip state.
 * File: src/standard/movieclip_state.hpp.
 */

#ifndef FVM_MOVIECLIP_STATE_HPP
/* Define FVM MOVIECLIP STATE HPP constant or compilation switch. */
#define FVM_MOVIECLIP_STATE_HPP
#include "../core/runtime.hpp"
#include "flashvm/execution.h"
#include "flashvm/path.h"
/* Build constructor and prototype for movieclip. */
FVM_Result fvm_movieclip_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                               fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);

/* Validate movieclip. */
FVM_Result fvm_movieclip_validate(FVM_Context *);
/* Read optional source version directly from a retained scene library. */
FVM_Result fvm_movieclip_library_version(FVM_Context *, FVM_Object *, unsigned int *);
/* Read retained source SWF version, or the ambient policy for native/old graphs. */
FVM_Result fvm_movieclip_movie_version(FVM_Context *, FVM_Object *, unsigned int *);
/* Read physical parent or the retained loaded-AVM1 parent after native detachment. */
FVM_Result fvm_movieclip_avm1_parent(FVM_Context *, FVM_Object *, FVM_Object **out);
/* Invoke intrinsic sibling construction with already-rooted values and a native barrier. */
FVM_Result fvm_movieclip_duplicate_instance(FVM_Context *, FVM_Object *, const FVM_Value *,
                                            const FVM_Value *, const FVM_Value *, FVM_Value *);
/* Compose the physical native ancestry under the caller's collector barrier. */
FVM_Result fvm_movieclip_world_matrix(FVM_Context *, FVM_Object *, FVM_Affine &);
/* Read native stroked world bounds under the caller's collector barrier. */
FVM_Result fvm_movieclip_world_bounds(FVM_Context *, FVM_Object *, FVM_PathBounds &, bool &);
/* Test native world paint under the caller's collector barrier. */
FVM_Result fvm_movieclip_world_shape_hit(FVM_Context *, FVM_Object *, double, double, bool &);
/* Convert root-relative native hit-test coordinates to world space, ignoring _lockroot. */
FVM_Result fvm_movieclip_hit_world_point(FVM_Context *, FVM_Object *, double &x, double &y);
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
/* Apply a semantic timeline command directly, independently from replaceable script methods. */
FVM_Result fvm_movieclip_frame_control(FVM_Context *, FVM_Object *, FVM_ExecutionFrame,
                                     const FVM_Value *, int playing);
/* Resolve counted display/object paths; an explicit root supports lexical-scope starts.
 * [GC:] Both start and explicit root are retained through property callbacks. */
FVM_Result fvm_movieclip_resolve_path(FVM_Context *, FVM_Object *, const FVM_Value &, FVM_Object **,
                                    FVM_Object *display_root = 0, FVM_Object *path_this = 0);
/* Identify the retained builtin class prototype without reading script-visible properties. */
bool fvm_movieclip_is_class_prototype(FVM_Object *);
/* Identify host root/parent descriptors replaced by intrinsic paths in loaded execution. */
bool fvm_movieclip_hidden_path_accessor(FVM_Context *, const FVM_AccessorEntry &);
/* Build the current counted AVM1 dot path directly from display state. */
FVM_Result fvm_movieclip_reference_string(FVM_Context *, FVM_Object *, FVM_Value *);
/* Run only a selected local frame's actions without moving the playhead or applying scene controls.
 * Number or counted label input is already coerced; AVM1 resolves target prefixes first. */
FVM_Result fvm_movieclip_call_frame(FVM_Context *, FVM_Object *, const FVM_Value &);
#endif
