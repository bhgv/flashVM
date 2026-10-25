/* ActionScript content queries consume validated native import records without transport. */
#ifndef FVM_MOVIECLIP_CONTENT_METHODS_HPP
#define FVM_MOVIECLIP_CONTENT_METHODS_HPP /* Guard stable native method declarations. */
#include "../core/runtime.hpp"

#define FVM_MOVIECLIP_BYTES_VERSION_ATTRIBUTE 0x0008U /* Hide new byte methods in SWF4 and earlier. */
#define FVM_MOVIECLIP_SWF_VERSION_ATTRIBUTE 0x0400U   /* Hide getSWFVersion before SWF7. */
#define FVM_MOVIECLIP_UNKNOWN_SWF_VERSION                                                                              \
    (-1)                                              /* Image, explicit unload or unavailable version.   \
                                                */


/* Return committed loaded bytes; no progressive transfer counters are invented. */
FVM_Result fvm_movieclip_bytes_loaded(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* Return committed canonical SWF bytes or encoded image bytes, equal to loaded bytes. */
FVM_Result fvm_movieclip_bytes_total(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* Return imported SWF version, source profile for an unmarked symbol, or the negative sentinel. */
FVM_Result fvm_movieclip_swf_version(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);
#endif
