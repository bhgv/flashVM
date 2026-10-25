/* MovieClip location bridge: private provenance is shared by named and numeric reads. */
#ifndef FVM_MOVIECLIP_LOCATION_HPP
#define FVM_MOVIECLIP_LOCATION_HPP /* Guard location callback declarations. */
#include "../core/runtime.hpp"


/* Return nearest retained source, or an empty string for a native clip without provenance. */
FVM_Result fvm_movieclip_url_read(FVM_Context *, FVM_Object *, FVM_Value *);


/* Getter-only named property; the builtin class prototype returns undefined. */
FVM_Result fvm_movieclip_url_get(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);
#endif
