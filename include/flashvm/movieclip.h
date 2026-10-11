#ifndef FLASHVM_MOVIECLIP_H
#define FLASHVM_MOVIECLIP_H
#include "value.h"
#include "raster.h"
#ifdef __cplusplus
extern "C"
{
#endif
    FVM_API FVM_Result fvm_movieclip_create(FVM_Context *, const char *name, unsigned int frames,
                                            FVM_Object **);
    /* Copy a validated AVM1 frame action and optional label. Frames start at 1. */
    FVM_API FVM_Result fvm_movieclip_define_frame(FVM_Context *, FVM_Object *, unsigned int,
                                                  const char *label, const void *actions, size_t);
    FVM_API FVM_Result fvm_movieclip_advance(FVM_Context *, FVM_Object *, unsigned int ticks);
    FVM_API FVM_Result fvm_movieclip_render(FVM_Context *, FVM_Object *, FVM_RasterSurface *);
#ifdef __cplusplus
}
#endif
#endif
