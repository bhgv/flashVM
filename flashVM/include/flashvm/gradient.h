/* FlashVM: portable C89 linear/radial vector gradient painting with straight RGBA stops. */
#ifndef FLASHVM_GRADIENT_H
#define FLASHVM_GRADIENT_H /* Include guard for gradient painting declarations. */
#include "path.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define FVM_GRADIENT_FOCAL_LIMIT 1.0         /* Exclusive absolute bound of supported radial focal positions. */
#define FVM_GRADIENT_MAX_STROKE_WIDTH 4096.0 /* Maximum destination-space stroke width in pixels. */
#define FVM_GRADIENT_MAX_STOPS 15            /* Maximum number of ordered gradient control points. */


/* Gradient geometry in a normalized square from -1 to +1 on each axis. */
typedef enum FVM_GradientType
{
    FVM_GRADIENT_LINEAR = 0 /* Ratio grows from the left edge to the right edge. */,
    FVM_GRADIENT_RADIAL = 1 /* Radial geometry; focal-aware entries may move its origin. */
} FVM_GradientType;


/* Extension policy outside the ramp; the original no-selector fill always selects PAD. */
typedef enum FVM_GradientSpread
{
    FVM_GRADIENT_SPREAD_PAD = 0,     /* Extend the endpoint colors. */
    FVM_GRADIENT_SPREAD_REFLECT = 1, /* Alternate forward/reverse unit intervals. */
    FVM_GRADIENT_SPREAD_REPEAT = 2   /* Repeat a half-open unit interval. */
} FVM_GradientSpread;


/* Color-space policy; alpha always interpolates as a straight linear channel. */
typedef enum FVM_GradientInterpolation
{
    FVM_GRADIENT_INTERPOLATION_RGB = 0,       /* Interpolate encoded RGB bytes as before. */
    FVM_GRADIENT_INTERPOLATION_LINEAR_RGB = 1 /* Interpolate decoded linear-light sRGB. */
} FVM_GradientInterpolation;


/* One ordered control point; equal ratios represent an abrupt color transition. */
typedef struct FVM_GradientStop
{
    double ratio;          /* Position from zero to one; stops must be nondecreasing. */
    FVM_RasterColor color; /* Straight red/green/blue/alpha bytes at this control point. */
} FVM_GradientStop;


/* Borrowed gradient description; the draw entry selects how the ramp extends. */
typedef struct FVM_Gradient
{
    FVM_GradientType type;         /* Linear/radial geometry; the draw entry supplies optional focal. */
    FVM_Affine matrix;             /* Maps the normalized gradient square to destination pixels. */
    const FVM_GradientStop *stops; /* Borrowed array valid for the complete draw call. */
    size_t count;                  /* Number of control points, from one to FVM_GRADIENT_MAX_STOPS. */
    double multiply[4];            /* Post-interpolation RGBA multipliers, initialized to one. */
    double add[4];                 /* Post-interpolation RGBA byte offsets, initialized to zero. */
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


/* Fill with explicit unsigned spread and interpolation selectors; borrow all inputs.
       The legacy gradient struct/layout is unchanged. RGB selects the existing byte sampler.
       LINEAR_RGB decodes sRGB stops and re-encodes interpolated RGB before color transforms.
       Invalid selectors/geometry and preparation failures preserve destination pixels. */
FVM_API FVM_Result fvm_path_draw_gradient_interpolated(const FVM_Path *, FVM_RasterSurface *,
                                                       const FVM_Gradient *, int even_odd,
                                                       unsigned int spread,
                                                       unsigned int interpolation);


/* Stroke with the same explicit color policy and a positive destination-pixel width.
       Width/ownership/backend limits match fvm_path_stroke_gradient; no VM is retained. */
FVM_API FVM_Result fvm_path_stroke_gradient_interpolated(const FVM_Path *, FVM_RasterSurface *,
                                                         const FVM_Gradient *, double width,
                                                         unsigned int spread,
                                                         unsigned int interpolation);


/* Add an explicit radial focal position without changing FVM_Gradient layout.


       Focal must be finite and strictly inside (-1,1); zero keeps centered sampling.
       Nonzero focal requires RADIAL geometry. Borrow inputs for this synchronous call.
       Width/spread/interpolation and failure atomicity match the interpolated entries. */
FVM_API FVM_Result fvm_path_draw_gradient_focal(const FVM_Path *, FVM_RasterSurface *,
                                                const FVM_Gradient *, int even_odd,
                                                unsigned int spread, unsigned int interpolation,
                                                double focal);


/* Stroke with a positive destination-space width and the same interior focal contract. */
FVM_API FVM_Result fvm_path_stroke_gradient_focal(const FVM_Path *, FVM_RasterSurface *,
                                                  const FVM_Gradient *, double width,
                                                  unsigned int spread, unsigned int interpolation,
                                                  double focal);
#ifdef __cplusplus
}
#endif
#endif
