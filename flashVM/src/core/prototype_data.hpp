/* FlashVM: raw loaded-SWF prototype data and the corresponding inheritance edge. */
#ifndef FVM_PROTOTYPE_DATA_HPP
#define FVM_PROTOTYPE_DATA_HPP /* Private prototype-data service guard. */
#include "runtime.hpp"


/* [GC:] Store one already-owned raw value and its object-only inheritance edge atomically. */
FVM_Result fvm_object_store_raw_prototype(FVM_Context *, FVM_Object *, const FVM_Value &);


/* Match the intrinsic prototype name under loaded SWF case rules. */
bool fvm_object_prototype_name(FVM_Context *, const fvm_detail::string &);


/* Identify the default prototype accessor without invoking custom accessors. */
bool fvm_object_intrinsic_prototype(FVM_Context *, FVM_Object *, const fvm_detail::string *alternate = 0);
#endif
