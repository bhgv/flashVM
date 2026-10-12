/* FlashVM: owned decoded SWF solid-shape contours, independent of backend pointers. */
#ifndef FVM_SWF_SHAPE_HPP
#define FVM_SWF_SHAPE_HPP /* Include guard for decoded shape resources. */
#include "flashvm/path.h"
#include "../details/vector.hpp"

/* One solid fill or stroke, retaining separate contours and quadratic control points. */
struct FVM_SWFShapePaint
{
    unsigned long rgb; /* Packed red/green/blue color. */
    double alpha;      /* Opacity percentage for the common MovieClip renderer. */
    double width;      /* Pixel stroke width; zero selects a fill. */
    bool stroke;       /* Distinguishes a zero-width hairline from a fill. */
    fvm_detail::vector<FVM_PathCommand> commands; /* Owned vector commands in pixel coordinates. */
    /* Initialize a fully opaque solid fill with no geometry. */
    FVM_SWFShapePaint() : rgb(0), alpha(100), width(0), stroke(false)
    {
    }
};

/* One immutable decoded character before its managed graph representation is constructed. */
struct FVM_SWFShape
{
    unsigned int id;       /* Nonzero dictionary character identifier. */
    FVM_PathBounds bounds; /* Declared SWF bounds in pixels, retained for inspection. */
    fvm_detail::vector<FVM_SWFShapePaint> paints; /* Ordered fills followed by ordered strokes. */
    /* Start an unidentified resource with empty bounds. */
    FVM_SWFShape() : id(0)
    {
        bounds.x_min = bounds.y_min = bounds.x_max = bounds.y_max = 0;
    }
};

/* Decode DefineShape/2/3 solid styles and edge records with bounded allocations and coordinates. */
FVM_Result fvm_swf_read_shape(const unsigned char *, size_t, unsigned int tag, FVM_SWFShape &);
#endif
