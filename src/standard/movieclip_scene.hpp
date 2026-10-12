/* FlashVM: internal bridge between MovieClip instances and SWF display-list controls. */
#ifndef FVM_MOVIECLIP_SCENE_HPP
#define FVM_MOVIECLIP_SCENE_HPP /* Include guard for scene-control dispatch. */
#include "../core/runtime.hpp"

/* [GC:] Return validated private clip state owned by its managed resource edge. */
FVM_Result fvm_movieclip_scene_state(FVM_Object *, FVM_Object **);
/* [GC:] Return the parent's child table, migrating legacy slots before authored placements. */
FVM_Result fvm_movieclip_scene_children(FVM_Object *, FVM_Object **);
/* Place a new symbol with its decoded pose before invoking its registered constructor. */
FVM_Result fvm_movieclip_scene_insert(FVM_Context *, FVM_Object *parent, FVM_Object *placement,
                                      FVM_Object **out);
/* Update only explicitly present placement fields on an existing instance. */
FVM_Result fvm_movieclip_scene_update(FVM_Context *, FVM_Object *, FVM_Object *placement);
/* Force removal by the timeline, including authored depths, and stop the detached instance. */
FVM_Result fvm_movieclip_scene_remove(FVM_Context *, FVM_Object *);
/* Apply this frame's controls, reconstructing the display state on jumps and loop boundaries. */
FVM_Result fvm_movieclip_scene_run(FVM_Context *, FVM_Object *, unsigned long frame, bool loop);
#endif
