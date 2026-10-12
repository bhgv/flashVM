/* FlashVM: C89 bitmap sampling through editable vector masks and affine color transforms. */
#ifndef FLASHVM_BITMAP_PAINT_H
#define FLASHVM_BITMAP_PAINT_H /* Include guard for bitmap path painting. */
#include "path.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Borrowed bitmap/style description valid throughout one draw call. */
    typedef struct FVM_BitmapPaint
    {
        const FVM_RasterSurface *source; /* Borrowed straight RGBA8 source image. */
        FVM_Affine matrix;  /* Maps source pixel coordinates to destination coordinates. */
        int repeat;         /* One tiles the image; zero extends its edge pixels. */
        int smoothing;      /* One filters bilinearly with premultiplied alpha; zero selects nearest
                               pixels. */
        double multiply[4]; /* Post-sampling RGBA multipliers, initialized to one. */
        double add[4];      /* Post-sampling byte offsets, initialized to zero. */
    } FVM_BitmapPaint;
    /* Initialize identity sampling with repeat enabled, smoothing disabled and no source. */
    FVM_API FVM_Result fvm_bitmap_paint_init(FVM_BitmapPaint *);
    /* Draw a sampled image through the filled path, using source-over composition.
     * Surface/path limits match raster.h; matrices must be finite, bounded and invertible.
     * All source pixels are sampled before destination writes, permitting aliased surfaces.
     * NO_GRAPHICS returns UNSUPPORTED after validating inputs. */
    FVM_API FVM_Result fvm_path_draw_bitmap(const FVM_Path *, FVM_RasterSurface *,
                                            const FVM_BitmapPaint *, int even_odd);
#ifdef __cplusplus
}
#endif
#endif
