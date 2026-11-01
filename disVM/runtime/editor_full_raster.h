/* Full-frame fallback for copied style-bearing native vector commands. */
#ifndef DVM_EDITOR_FULL_RASTER_H
#define DVM_EDITOR_FULL_RASTER_H
#include "editor_stamp.h"


/* Borrow immutable path and exclusively writable surface. Clip is already
 * admitted in surface pixels; width/fill-rule/stamp retain captured semantics.
 * Localize a detached path before drawing, never mutate stored owner geometry.
 * Return a raster status; allocation failure occurs before drawing this entry.
 * No VM, channel, font or document access is allowed on this raster worker. */
static FVM_Result dvm_editor_full_command(const FVM_Path *path,
                                         FVM_RasterSurface *surface,
                                         FVM_RasterColor color,
                                         double width,
                                         int even_odd,
                                         int stamp,
                                         int left,
                                         int top,
                                         int right,
                                         int bottom)
{
    FVM_Path *local = NULL;
    FVM_Affine shift = {1, 0, 0, 1, 0, 0};
    FVM_RasterSurface clip;
    FVM_Result result;
    if (!path || !surface || !surface->pixels || left < 0 || top < 0 ||
        right > (int)surface->width || bottom > (int)surface->height)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (left >= right || top >= bottom)
    {
        return FVM_OK;
    }
    clip = *surface;
    clip.pixels += (size_t)top * clip.stride + (size_t)left * 4;
    clip.width = (unsigned int)(right - left);
    clip.height = (unsigned int)(bottom - top);
    shift.x0 = -left;
    shift.y0 = -top;
    result = fvm_path_create(&local);
    if (result == FVM_OK)
    {
        result = fvm_path_append(local, path, &shift);
    }
    if (result == FVM_OK)
    {
        result = stamp ? dvm_editor_stamp_draw(local, &clip, color, width, even_odd)
                       : fvm_path_draw(local, &clip, color, width, even_odd);
    }
    fvm_path_destroy(local);
    return result;
}
#endif
