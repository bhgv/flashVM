/* FlashVM: context-wide stage background and legacy rendering-quality policy. */
#ifndef FLASHVM_STAGE_H
#define FLASHVM_STAGE_H /* Guard for the C89 stage-background embedding API. */
#include "raster.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Classic Flash stage quality. LOW disables vector edge antialiasing; other
     * levels retain it. Bitmap downsampling differences are not yet implemented. */
typedef enum FVM_StageQuality
{
    FVM_STAGE_QUALITY_LOW = 0, /* Binary vector coverage. */
    FVM_STAGE_QUALITY_MEDIUM,  /* Antialiased vector coverage. */
    FVM_STAGE_QUALITY_HIGH,    /* Default antialiased stage rendering. */
    FVM_STAGE_QUALITY_BEST     /* Retained best preference when ToggleQuality restores quality. */
} FVM_StageQuality;


/* Read context-local quality. No allocations, callbacks or ownership transfer. */
FVM_API FVM_Result fvm_stage_get_quality(FVM_Context *, FVM_StageQuality *);


/* Select a quality and reset the remembered BEST toggle preference accordingly.
     * May be called on the VM thread during execution, but not collection/destruction. */
FVM_API FVM_Result fvm_stage_set_quality(FVM_Context *, FVM_StageQuality);


/* Toggle HIGH/BEST to LOW, otherwise restore HIGH or remembered BEST.
     * The AVM1 ToggleQuality action uses this same allocation-free operation. */
FVM_API FVM_Result fvm_stage_toggle_quality(FVM_Context *);


/* Query straight-alpha RGBA color; NOT_FOUND leaves output unchanged when no color is selected.
     */
FVM_API FVM_Result fvm_stage_get_background(FVM_Context *, FVM_RasterColor *);


/* Override the background from the host, including transparency; subsequent SWF tags preserve
     * it. */
FVM_API FVM_Result fvm_stage_set_background(FVM_Context *, FVM_RasterColor);


/* Remove the override/selection so the next encountered SWF background control may select a
     * color. */
FVM_API FVM_Result fvm_stage_clear_background(FVM_Context *);


/* Coalesce an explicit host redraw request, independently of ActionScript event eligibility.
     * Owner thread only; no allocation, callbacks, GC roots, rendering or timeline advancement.
     * INVALID_ARGUMENT during destruction/collection/release; existing pending state is preserved. */
FVM_API FVM_Result fvm_stage_request_update(FVM_Context *);


/* Read the pending flag into required output without clearing it. Errors preserve output.
     * Lifecycle/thread restrictions match request_update; plain integer results own no memory. */
FVM_API FVM_Result fvm_stage_get_update(FVM_Context *, int *requested);


/* Read and clear the pending flag atomically on the owner thread; errors preserve both.
     * Consume before drawing: a request made during presentation then survives for a later render.
     * A host which retries a failed presentation must request_update again. No rendering occurs here. */
FVM_API FVM_Result fvm_stage_take_update(FVM_Context *, int *requested);
#ifdef __cplusplus
}
#endif
#endif
