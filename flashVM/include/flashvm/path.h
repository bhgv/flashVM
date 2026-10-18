/* FlashVM: Public embedding API declarations: path.
 * File: include/flashvm/path.h.
 */

#ifndef FLASHVM_PATH_H
/* Define FLASHVM PATH H constant or compilation switch. */
#define FLASHVM_PATH_H
#include "flashvm/raster.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Owned, editable vector contours. Coordinates use doubles, independent of
     * font handles. Quadratic curves retain their original control point. */
    typedef struct FVM_Path FVM_Path;
    /* Define FVM_PathVerb enum and its representation. */
    typedef enum FVM_PathVerb
    {
        FVM_PATH_MOVE /* Start a new contour at the supplied point. */,
        FVM_PATH_LINE /* Append a straight segment. */,
        FVM_PATH_QUADRATIC /* Append a quadratic Bezier segment. */,
        FVM_PATH_CUBIC /* Append a cubic Bezier segment. */,
        FVM_PATH_CLOSE /* Close the current contour. */
    } FVM_PathVerb;
    /* Define FVM_PathCommand struct and its representation. */
    typedef struct FVM_PathCommand
    {
        FVM_PathVerb verb; /* Path command selecting the active coordinate layout. */
        double x[3] /* Horizontal coordinate. */, y[3]; /* Vertical coordinate. */
    } FVM_PathCommand;
    /* x' = xx*x + xy*y + x0; y' = yx*x + yy*y + y0. */
    /* Define FVM_Affine struct and its representation. */
    typedef struct FVM_Affine
    {
        double xx /* Horizontal scale component of the affine transform. */,
            yx /* Vertical contribution from the horizontal coordinate. */,
            xy /* Horizontal contribution from the vertical coordinate. */,
            yy /* Vertical scale component of the affine transform. */,
            x0 /* Horizontal coordinate. */, y0; /* Vertical coordinate. */
    } FVM_Affine;
    /* Define FVM_PathBounds struct and its representation. */
    typedef struct FVM_PathBounds
    {
        double x_min /* Minimum horizontal coordinate of the bounds. */,
            y_min /* Minimum vertical coordinate of the bounds. */,
            x_max /* Maximum horizontal coordinate of the bounds. */,
            y_max; /* Maximum vertical coordinate of the bounds. */
    } FVM_PathBounds;
    /* Create path. */
    FVM_API FVM_Result fvm_path_create(FVM_Path **out);
    /* Destroy path. */
    FVM_API void fvm_path_destroy(FVM_Path *path);
    /* Add path. */
    FVM_API FVM_Result fvm_path_add(FVM_Path *path, const FVM_PathCommand *command);
    /* Count path. */
    FVM_API size_t fvm_path_count(const FVM_Path *path);
    /* Read path. */
    FVM_API FVM_Result fvm_path_get(const FVM_Path *path, size_t index, FVM_PathCommand *out);
    /* Update path point. */
    FVM_API FVM_Result fvm_path_set_point(FVM_Path *path, size_t command, unsigned int point,
                                          double x, double y);
    /* Transforms/append are transactional. Coordinate bound: +/- 1,000,000.
     * Append copies contours, so subsequent edits do not change the source. */
    /* Implement path transform for this subsystem. */
    FVM_API FVM_Result fvm_path_transform(FVM_Path *path, const FVM_Affine *matrix);
    /* Append path. */
    FVM_API FVM_Result fvm_path_append(FVM_Path *path, const FVM_Path *source,
                                       const FVM_Affine *matrix);
    /* Bezier extrema, not the control-point box. Empty path -> zero bounds. */
    /* Implement path bounds for this subsystem. */
    FVM_API FVM_Result fvm_path_bounds(const FVM_Path *path, FVM_PathBounds *out);
    /* Point query without a graphics backend. Width=0 fills, positive width tests
     * round strokes. Curves use adaptive chords (0.0001 coordinate-unit tolerance,
     * maximum depth 20); filled boundaries use a 0.000001 tolerance. */
    FVM_API FVM_Result fvm_path_hit_test(const FVM_Path *, double x, double y, double width,
                                       int even_odd, int *hit);
    /* Draw using the selected AGG/Cairo backend. Width=0 fills; positive width
     * strokes with round joins/caps. even_odd is 0 (nonzero) or 1. Rendering
     * coordinates and surface dimensions are bounded as in raster.h. */
    /* Draw path. */
    FVM_API FVM_Result fvm_path_draw(const FVM_Path *path, FVM_RasterSurface *surface,
                                     FVM_RasterColor color, double width, int even_odd);
    /* Use the filled path as an antialiased mask for an equally sized RGBA8 image.
     * Source-over composition; source and destination may be the same surface. */
    /* Implement path mask for this subsystem. */
    FVM_API FVM_Result fvm_path_mask(const FVM_Path *path, FVM_RasterSurface *destination,
                                     const FVM_RasterSurface *source, int even_odd);
#ifdef __cplusplus
}
#endif
#endif
