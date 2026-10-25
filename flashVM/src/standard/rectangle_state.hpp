/* FlashVM: internal Rectangle namespace installation and transport callbacks. */
#ifndef FVM_RECTANGLE_STATE_HPP
#define FVM_RECTANGLE_STATE_HPP /* Include guard for Rectangle integration. */
#include "geometry_support.hpp"


/* [GC:] Install Rectangle into the unpublished namespace and shared factory captures. */
FVM_Result fvm_rectangle_stage(FVM_Context *,
                               FVM_Object *,
                               FVM_Object *,
                               FVM_Object *,
                               FVM_Object *,
                               fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);


/* Validate the stored Rectangle class without invoking user getters. */
bool fvm_rectangle_validate(FVM_Context *, FVM_Object *);


/* Identify Rectangle native methods and accessors by stable transport IDs. */
unsigned long fvm_rectangle_callback_id(FVM_NativeFunction);


/* Restore a Rectangle implementation using its stable callback ID. */
FVM_NativeFunction fvm_rectangle_callback_from_id(unsigned long);
#endif
