/* FlashVM: portable C89 linear/radial vector gradient painting with straight RGBA stops. */
#ifndef FLASHVM_GRADIENT_H
#define FLASHVM_GRADIENT_H /* Include guard for gradient painting declarations. */
#include "path.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define FVM_GRADIENT_MAX_STROKE_WIDTH 4096.0 /* Maximum destination-space stroke width in pixels. */
#define FVM_GRADIENT_MAX_STOPS 15 /* Maximum number of ordered gradient control points. */
    /* Gradient geometry in a normalized square from -1 to +1 on each axis. */
    typedef enum FVM_GradientType
    {
        FVM_GRADIENT_LINEAR = 0 /* Ratio grows from the left edge to the right edge. */,
        FVM_GRADIENT_RADIAL = 1 /* Ratio grows from the center to the unit circle. */
    } FVM_GradientType;
    /* Extension policy outside the normalized ramp; existing APIs always select PAD. */
    typedef enum FVM_GradientSpread
    {
        FVM_GRADIENT_SPREAD_PAD = 0, /* Extend the endpoint colors. */
        FVM_GRADIENT_SPREAD_REFLECT = 1, /* Alternate forward/reverse unit intervals. */
        FVM_GRADIENT_SPREAD_REPEAT = 2 /* Repeat a half-open unit interval. */
    } FVM_GradientSpread;
    /* One ordered control point; equal ratios represent an abrupt color transition. */
    typedef struct FVM_GradientStop
    {
        double ratio;          /* Position from zero to one; stops must be nondecreasing. */
        FVM_RasterColor color; /* Straight red/green/blue/alpha bytes at this control point. */
    } FVM_GradientStop;
    /* Borrowed gradient description; the draw entry selects how the ramp extends. */
    typedef struct FVM_Gradient
    {
        FVM_GradientType type; /* Linear or centered radial geometry. */
        FVM_Affine matrix;     /* Maps the normalized gradient square to destination pixels. */
        const FVM_GradientStop *stops; /* Borrowed array valid for the complete draw call. */
        size_t count;       /* Number of control points, from one to FVM_GRADIENT_MAX_STOPS. */
        double multiply[4]; /* Post-interpolation RGBA multipliers, initialized to one. */
        double add[4];      /* Post-interpolation RGBA byte offsets, initialized to zero. */
    } FVM_Gradient;
    /* Initialize an identity linear gradient; the caller must supply its control points. */
    FVM_API FVM_Result fvm_gradient_init(FVM_Gradient *);
    /* Fill an editable path with a pad gradient and an antialiased even-odd/nonzero mask.
       Matrices must be finite, bounded and invertible; surfaces obey raster.h limits.
       Stops interpolate straight RGBA in ordinary RGB space. NO_GRAPHICS returns UNSUPPORTED. */
    FVM_API FVM_Result fvm_path_draw_gradient(const FVM_Path *, FVM_RasterSurface *,
                                              const FVM_Gradient *, int even_odd);
    /* Additive spread-aware draw; the gradient struct layout and legacy PAD call are unchanged.
       Borrow all inputs for this synchronous call. Invalid spread/geometry preserves pixels;
       allocation/backend statuses propagate. No VM, managed root or device is retained. */
    FVM_API FVM_Result fvm_path_draw_gradient_spread(const FVM_Path *, FVM_RasterSurface *,
                                                     const FVM_Gradient *, int even_odd,
                                                     unsigned int spread);
    /* Stroke a path with the borrowed gradient and PAD/REFLECT/REPEAT policy.
       Width is in destination pixels, finite and greater than zero up to 4096.
       Uses existing backend stroke caps/joins and antialiasing; no VM is retained.
       Invalid arguments and preparation failures preserve destination pixels. */
    FVM_API FVM_Result fvm_path_stroke_gradient(const FVM_Path *, FVM_RasterSurface *,
                                                 const FVM_Gradient *, double width,
                                                 unsigned int spread);
#ifdef __cplusplus
}
#endif
#endif
