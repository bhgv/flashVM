/* FlashVM: managed Transform class, retained MovieClip references and native callback IDs. */
#ifndef FVM_TRANSFORM_STATE_HPP
#define FVM_TRANSFORM_STATE_HPP /* Include guard for Transform class integration. */
#include "geometry_support.hpp"


/* [GC:] Stage Transform class/prototype and retain geometry factory captures. */
FVM_Result fvm_transform_stage(FVM_Context *,
                               FVM_Object *,
                               FVM_Object *,
                               FVM_Object *,
                               FVM_Object *,
                               fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);


/* Validate the installed Transform constructor without invoking user properties. */
bool fvm_transform_validate(FVM_Context *, FVM_Object *);


/* [GC:] Install MovieClip.transform using the staged Transform class as a capture root. */
FVM_Result fvm_transform_install_movieclip(FVM_Context *, FVM_Object *, FVM_Object *);


/* Obtain a stable Transform constructor/accessor callback identifier. */
unsigned long fvm_transform_callback_id(FVM_NativeFunction);


/* Restore one known Transform callback independently of context globals. */
FVM_NativeFunction fvm_transform_callback_from_id(unsigned long);
#endif
