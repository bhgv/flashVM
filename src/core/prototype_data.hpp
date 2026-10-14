/* FlashVM: raw loaded-SWF prototype data and the corresponding inheritance edge. */
#ifndef FVM_PROTOTYPE_DATA_HPP
#define FVM_PROTOTYPE_DATA_HPP /* Private prototype-data service guard. */
#include "runtime.hpp"
/* [GC:] Store one already-owned raw value and its object-only inheritance edge atomically. */
FVM_Result fvm_object_store_raw_prototype(FVM_Context *, FVM_Object *, const FVM_Value &);
#endif
