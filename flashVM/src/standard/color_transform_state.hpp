/* FlashVM: managed native ColorTransform state and geometry class integration. */
#ifndef FVM_COLOR_TRANSFORM_STATE_HPP
#define FVM_COLOR_TRANSFORM_STATE_HPP /* Include guard for native color transform helpers. */
#include "geometry_support.hpp"


/* Read the eight native numeric channels without evaluating public overrides. */
bool fvm_color_transform_values(FVM_Context *, FVM_Object *, double *);


/* [GC:] Create native color state with its retained class/prototype and copied numeric channels. */
FVM_Result fvm_color_transform_create(FVM_Context *, FVM_Object *, const double *, FVM_Value *);


/* [GC:] Stage ColorTransform into an unpublished namespace and shared constructor captures. */
FVM_Result fvm_color_transform_stage(FVM_Context *,
                                     FVM_Object *,
                                     FVM_Object *,
                                     FVM_Object *,
                                     FVM_Object *,
                                     fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);


/* Validate the stored color class identity without invoking user properties. */
bool fvm_color_transform_validate(FVM_Context *, FVM_Object *);


/* Obtain an append-only ColorTransform callback transport ID. */
unsigned long fvm_color_transform_callback_id(FVM_NativeFunction);


/* Restore one known native callback from its stable ID. */
FVM_NativeFunction fvm_color_transform_callback_from_id(unsigned long);
#endif
