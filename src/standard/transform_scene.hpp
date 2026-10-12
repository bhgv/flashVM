/* FlashVM: internal world-matrix, color and bounds bridge for MovieClip Transform objects. */
#ifndef FVM_TRANSFORM_SCENE_HPP
#define FVM_TRANSFORM_SCENE_HPP /* Include guard for scene-transform queries. */
#include "../core/runtime.hpp"
#include "flashvm/path.h"
/* Query the composed world transform while protecting display-list references. */
FVM_Result fvm_transform_scene_matrix(FVM_Context *, FVM_Object *, FVM_Affine *);
/* Copy local or composed RGBA multipliers/offsets into an eight-element array. */
FVM_Result fvm_transform_scene_color(FVM_Context *, FVM_Object *, bool, double *);
/* Atomically publish local RGBA color terms and the corresponding _alpha display property. */
FVM_Result fvm_transform_scene_set_color(FVM_Context *, FVM_Object *, const double *);
/* Compute world-space stroked vector bounds; empty clips yield zero bounds. */
FVM_Result fvm_transform_scene_bounds(FVM_Context *, FVM_Object *, FVM_PathBounds *);
#endif
