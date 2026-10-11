#ifndef FLASHVM_PATH_H
#define FLASHVM_PATH_H
#include "flashvm/raster.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Owned, editable vector contours. Coordinates use doubles, independent of
     * font handles. Quadratic curves retain their original control point. */
    typedef struct FVM_Path FVM_Path;
    typedef enum FVM_PathVerb
    {
        FVM_PATH_MOVE,
        FVM_PATH_LINE,
        FVM_PATH_QUADRATIC,
        FVM_PATH_CUBIC,
        FVM_PATH_CLOSE
    } FVM_PathVerb;
    typedef struct FVM_PathCommand
    {
        FVM_PathVerb verb;
        double x[3], y[3];
    } FVM_PathCommand;
    /* x' = xx*x + xy*y + x0; y' = yx*x + yy*y + y0. */
    typedef struct FVM_Affine
    {
        double xx, yx, xy, yy, x0, y0;
    } FVM_Affine;
    typedef struct FVM_PathBounds
    {
        double x_min, y_min, x_max, y_max;
    } FVM_PathBounds;
    FVM_API FVM_Result fvm_path_create(FVM_Path **out);
    FVM_API void fvm_path_destroy(FVM_Path *path);
    FVM_API FVM_Result fvm_path_add(FVM_Path *path, const FVM_PathCommand *command);
    FVM_API size_t fvm_path_count(const FVM_Path *path);
    FVM_API FVM_Result fvm_path_get(const FVM_Path *path, size_t index, FVM_PathCommand *out);
    FVM_API FVM_Result fvm_path_set_point(FVM_Path *path, size_t command, unsigned int point,
                                          double x, double y);
    /* Transforms/append are transactional. Coordinate bound: +/- 1,000,000.
     * Append copies contours, so subsequent edits do not change the source. */
    FVM_API FVM_Result fvm_path_transform(FVM_Path *path, const FVM_Affine *matrix);
    FVM_API FVM_Result fvm_path_append(FVM_Path *path, const FVM_Path *source,
                                       const FVM_Affine *matrix);
    /* Bezier extrema, not the control-point box. Empty path -> zero bounds. */
    FVM_API FVM_Result fvm_path_bounds(const FVM_Path *path, FVM_PathBounds *out);
    /* Draw using the selected AGG/Cairo backend. Width=0 fills; positive width
     * strokes with round joins/caps. even_odd is 0 (nonzero) or 1. Rendering
     * coordinates and surface dimensions are bounded as in raster.h. */
    FVM_API FVM_Result fvm_path_draw(const FVM_Path *path, FVM_RasterSurface *surface,
                                     FVM_RasterColor color, double width, int even_odd);
    /* Use the filled path as an antialiased mask for an equally sized RGBA8 image.
     * Source-over composition; source and destination may be the same surface. */
    FVM_API FVM_Result fvm_path_mask(const FVM_Path *path, FVM_RasterSurface *destination,
                                     const FVM_RasterSurface *source, int even_odd);
#ifdef __cplusplus
}
#endif
#endif
