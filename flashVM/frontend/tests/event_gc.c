/* Actual player/VM/raster with deterministic input allocations between slow frames.
 * [GC:] Only player safe-point calls are observed; the real collector remains enabled. */
#include "flashvm/gc.h"
#include "../common/window.h"
#define fvm_desktop_window_pump_clip_input event_gc_base_pump
#define fvm_desktop_window_events event_gc_base_events
#include "update_capture.c"
#undef fvm_desktop_window_pump_clip_input
#undef fvm_desktop_window_events
static FVM_Result event_gc_step(FVM_Context *, unsigned long *);
#define fvm_gc_step event_gc_step
#include "../common/player.c"
#undef fvm_gc_step

/* Bound waits so input has four turns between the two authored frames. */
int fvm_desktop_window_events(FVM_DesktopWindow *window, int timeout)
{
    return event_gc_base_events(window, timeout > 100 ? 100 : timeout);
}

/* [GC:] Model transient listener objects without retaining a raw pointer across collection.
 * Root and HostInput preservation are exercised by subsequent real frames/publication. */
FVM_Result fvm_desktop_window_pump_clip_input(FVM_DesktopWindow *window, FVM_Context *context,
                                              FVM_Object *root, double x, double y)
{
    unsigned long threshold, i;
    FVM_Object *temporary;
    FVM_Result result = event_gc_base_pump(window, context, root, x, y);
    if (result != FVM_OK)
        return result;
    result = fvm_gc_threshold(context, &threshold);
    if (result != FVM_OK || threshold > 100000UL)
        return FVM_ERROR;
    for (i = 0; i <= threshold; ++i)
        if (fvm_object_create(context, &temporary) != FVM_OK)
            return FVM_ERROR;
    return FVM_OK;
}

/* [GC:] Observe real reclamation, not just a scheduling counter or a forced full collection. */
static FVM_Result event_gc_step(FVM_Context *context, unsigned long *out)
{
    unsigned long reclaimed = 0;
    FVM_Result result = fvm_gc_step(context, &reclaimed);
    if (out)
        *out = reclaimed;
    fprintf(stderr, "EVENT_GC ms=%.0f reclaimed=%lu\n", milliseconds, reclaimed);
    return result;
}
