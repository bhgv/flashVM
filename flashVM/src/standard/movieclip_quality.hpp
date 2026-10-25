/* FlashVM: context quality shared by native display-property adapters. */
#ifndef FVM_MOVIECLIP_QUALITY_HPP
#define FVM_MOVIECLIP_QUALITY_HPP /* Guard the internal adapter declarations. */
#include "../core/runtime.hpp"


/* Read the uppercase quality name or the legacy numeric quality value. */
FVM_Result fvm_movieclip_quality_read(FVM_Context *, bool, FVM_Value *);


/* Apply an already converted string or number without further script calls. */
FVM_Result fvm_movieclip_quality_write(FVM_Context *, bool, const FVM_Value &);


/* Read the named quality label; userdata is unused. */
FVM_Result fvm_movieclip_quality_get(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);


/* Convert and publish a named quality label. */
FVM_Result fvm_movieclip_quality_set(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);
/* Read the named legacy numeric view. */
FVM_Result
fvm_movieclip_highquality_get(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);
/* Convert and publish a named legacy quality number. */
FVM_Result
fvm_movieclip_highquality_set(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);
#endif
