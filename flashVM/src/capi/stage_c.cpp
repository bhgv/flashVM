/* FlashVM: primitive context stage state for host backgrounds and SWF SetBackgroundColor controls.
 */
#include "../core/runtime.hpp"
#include "flashvm/stage.h"

/* Read selected stage color without invoking script or changing an unset output. */
extern "C" FVM_Result fvm_stage_get_background(FVM_Context *context, FVM_RasterColor *out)
{
    if (!context || !out)
        return FVM_INVALID_ARGUMENT;
    if (!context->stage_background_set)
        return FVM_NOT_FOUND;
    *out = context->stage_background;
    return FVM_OK;
}

/* Publish a host color override without allocating managed state. */
extern "C" FVM_Result fvm_stage_set_background(FVM_Context *context, FVM_RasterColor color)
{
    if (!context || context->destroying || context->gc_collecting)
        return FVM_INVALID_ARGUMENT;
    context->stage_background = color;
    context->stage_background_set = true;
    return FVM_OK;
}

/* Clear color selection while preserving unrelated stage and movie state. */
extern "C" FVM_Result fvm_stage_clear_background(FVM_Context *context)
{
    if (!context || context->destroying || context->gc_collecting)
        return FVM_INVALID_ARGUMENT;
    context->stage_background_set = false;
    return FVM_OK;
}

/* Read the stage policy without touching the display list or collector. */
extern "C" FVM_Result fvm_stage_get_quality(FVM_Context *context, FVM_StageQuality *out)
{
    if (!context || !out)
        return FVM_INVALID_ARGUMENT;
    *out = context->stage_quality;
    return FVM_OK;
}

/* Publish a validated host selection; explicitly selecting LOW clears old BEST preference. */
extern "C" FVM_Result fvm_stage_set_quality(FVM_Context *context, FVM_StageQuality quality)
{
    if (!context || context->destroying || context->gc_collecting || context->gc_releasing ||
        quality < FVM_STAGE_QUALITY_LOW || quality > FVM_STAGE_QUALITY_BEST)
        return FVM_INVALID_ARGUMENT;
    context->stage_quality = quality;
    context->stage_quality_best = quality == FVM_STAGE_QUALITY_BEST;
    return FVM_OK;
}

/* Legacy action restores the previous bitmap-downsampling preference without forcing allocation. */
extern "C" FVM_Result fvm_stage_toggle_quality(FVM_Context *context)
{
    if (!context || context->destroying || context->gc_collecting || context->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    context->stage_quality = context->stage_quality == FVM_STAGE_QUALITY_HIGH ||
                                     context->stage_quality == FVM_STAGE_QUALITY_BEST
                                 ? FVM_STAGE_QUALITY_LOW
                             : context->stage_quality_best ? FVM_STAGE_QUALITY_BEST
                                                           : FVM_STAGE_QUALITY_HIGH;
    return FVM_OK;
}
