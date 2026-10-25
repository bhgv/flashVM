/* FlashVM: owned decoded SWF vector paints, independent of backend pointers. */
#ifndef FVM_SWF_SHAPE_HPP
#define FVM_SWF_SHAPE_HPP /* Include guard for decoded shape resources. */
#include "flashvm/gradient.h"
#include "format_constants.h"
#include "../details/vector.hpp"


/* One SWF gradient control point retaining its original byte ratio and straight RGBA. */
struct FVM_SWFGradientStop
{
    unsigned int ratio;    /* Ordered position from zero to 255. */
    FVM_RasterColor color; /* Red/green/blue/alpha bytes copied from the SWF record. */


    /* Initialize an opaque black control point at the start of the ramp. */
    FVM_SWFGradientStop() : ratio(0)
    {
        color.r = color.g = color.b = 0;
        color.a = 255;
    }
};


/* One decoded fill or legacy stroke with owned style and quadratic contour geometry. */
struct FVM_SWFShapePaint
{
    unsigned long rgb;                             /* Packed red/green/blue color. */
    double alpha;                                  /* Opacity percentage for the common MovieClip renderer. */
    double width;                                  /* Pixel stroke width; zero selects a fill. */
    unsigned int gradient_type;                    /* Zero for solid; linear, radial or focal fill discriminator. */
    unsigned int gradient_spread;                  /* Pad, reflect or repeat; legacy shapes always use pad. */
    unsigned int gradient_interpolation;           /* RGB or linearRGB; legacy shapes always use RGB. */
    double gradient_focal;                         /* Signed interior focal position, zero for non-focal gradients. */
    FVM_Affine gradient_matrix;                    /* Maps SWF gradient coordinates in pixels to shape-local pixels. */
    fvm_detail::vector<FVM_SWFGradientStop> stops; /* Owned sorted control points for a gradient fill. */
    unsigned int bitmap_type;                      /* Zero or bitmap fill style 0x40..0x43. */
    unsigned int bitmap_id;                        /* Referenced bitmap character ID, zero for non-bitmap paints. */
    FVM_Affine bitmap_matrix;                      /* Maps source pixel coordinates to shape-local pixels. */
    unsigned stroke_cap;                           /* Shared start/end LINESTYLE2 cap; zero retains legacy round. */
    unsigned stroke_join;                          /* LINESTYLE2 round or bevel join selector. */
    bool no_close;                                /* LINESTYLE2 keeps coincident endpoints capped rather than joined. */
    bool stroke;                                  /* Distinguishes a zero-width hairline from a fill. */
    fvm_detail::vector<FVM_PathCommand> commands; /* Owned vector commands in pixel coordinates. */


    /* Initialize a fully opaque solid fill with no geometry. */
    FVM_SWFShapePaint()
        : rgb(0), alpha(100), width(0), gradient_type(0), gradient_spread(0), gradient_interpolation(0),
          gradient_focal(0), bitmap_type(0), bitmap_id(0), stroke_cap(FVM_SWF_LINESTYLE_ROUND),
          stroke_join(FVM_SWF_LINESTYLE_ROUND), no_close(false), stroke(false)
    {
        bitmap_matrix.xx = bitmap_matrix.yy = 1;
        bitmap_matrix.xy = bitmap_matrix.yx = bitmap_matrix.x0 = bitmap_matrix.y0 = 0;
        gradient_matrix.xx = gradient_matrix.yy = 1;
        gradient_matrix.xy = gradient_matrix.yx = gradient_matrix.x0 = gradient_matrix.y0 = 0;
    }
};


/* One immutable decoded character before its managed graph representation is constructed. */
struct FVM_SWFShape
{
    unsigned int id;                              /* Nonzero dictionary character identifier. */
    FVM_PathBounds bounds;                        /* Declared SWF bounds in pixels, retained for inspection. */
    fvm_detail::vector<FVM_SWFShapePaint> paints; /* Ordered fills followed by ordered strokes. */


    /* Start an unidentified resource with empty bounds. */
    FVM_SWFShape() : id(0)
    {
        bounds.x_min = bounds.y_min = bounds.x_max = bounds.y_max = 0;
    }
};


/* Decode DefineShape/2/3 and bounded DefineShape4 styles and edge records with bounded allocations and coordinates. */
FVM_Result fvm_swf_read_shape(const unsigned char *, size_t, unsigned int tag, FVM_SWFShape &);
#endif
