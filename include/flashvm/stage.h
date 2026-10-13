/* FlashVM: host access to the context-wide stage background selected by SWF controls. */
#ifndef FLASHVM_STAGE_H
#define FLASHVM_STAGE_H /* Guard for the C89 stage-background embedding API. */
#include "raster.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Query straight-alpha RGBA color; NOT_FOUND leaves output unchanged when no color is selected. */
FVM_API FVM_Result fvm_stage_get_background(FVM_Context *, FVM_RasterColor *);
/* Override the background from the host, including transparency; subsequent SWF tags preserve it. */
FVM_API FVM_Result fvm_stage_set_background(FVM_Context *, FVM_RasterColor);
/* Remove the override/selection so the next encountered SWF background control may select a color. */
FVM_API FVM_Result fvm_stage_clear_background(FVM_Context *);
#ifdef __cplusplus
}
#endif
#endif
