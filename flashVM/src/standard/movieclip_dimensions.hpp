/* Named MovieClip dimension adapters share the intrinsic AVM1 geometry service. */
#ifndef FVM_MOVIECLIP_DIMENSIONS_HPP
#define FVM_MOVIECLIP_DIMENSIONS_HPP /* Guard private callback declarations. */
#include "../core/runtime.hpp"


/* Read the painted width in the clip parent coordinate system. */
FVM_Result fvm_movieclip_width_get(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* Convert once and resize through the intrinsic width operation. */
FVM_Result fvm_movieclip_width_set(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* Read the painted height in the clip parent coordinate system. */
FVM_Result fvm_movieclip_height_get(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* Convert once and resize through the intrinsic height operation. */
FVM_Result fvm_movieclip_height_set(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);
#endif
