/* Native opaque round-nib raster primitive; no VM/SDL dependency. */
#ifndef DVM_EDITOR_STAMP_H
#define DVM_EDITOR_STAMP_H
#include "flashvm/path.h"


/* Draw eligible opaque move/line strokes by reusing one sampled nib bitmap.
 * Borrow immutable path and exclusively owned RGBA surface. No heap/VM/thread
 * state is retained. Unsupported geometry uses the ordinary vector rasterizer;
 * return its result or FVM_OK after stamping. Width/color retain path semantics. */
FVM_Result dvm_editor_stamp_draw(
    const FVM_Path *path, FVM_RasterSurface *surface, FVM_RasterColor color, double width, int even_odd);
#endif
