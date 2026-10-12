/* FlashVM: BitmapData draw matrices, source snapshots, destination clipping and pixel blend modes.
 */
#include "bitmap_draw.hpp"
#include "bitmap_state.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/standard.h"
#include <cmath>
#include <cstring>

namespace
{
using namespace fvm_geometry_detail;
/* Round a finite color component into one saturated byte. */
unsigned char byte(double value)
{
    return (unsigned char)(value <= 0 ? 0 : value >= 255 ? 255 : std::floor(value + .5));
}
/* Preserve ordinary ActionScript truth rules for smoothing flags. */
bool truth(const FVM_Value &value)
{
    return value.type == FVM_VALUE_OBJECT || (value.type == FVM_VALUE_BOOLEAN && value.boolean) ||
           (value.type == FVM_VALUE_NUMBER && value.number && value.number == value.number) ||
           (value.type == FVM_VALUE_STRING && fvm_value_string_size(&value));
}
/* Read copied numeric public fields with an explicit arithmetic bound. */
FVM_Result fields(FVM_Context *c, FVM_Object *object, const char *const *names, unsigned count,
                  double *values, double bound)
{
    for (unsigned i = 0; i < count; ++i)
    {
        FVM_Value value;
        FVM_Result r = fvm_object_get(c, object, names[i], &value);
        if (r == FVM_OK)
            r = fvm_value_to_number(c, &value, values + i);
        if (r != FVM_OK)
            return r;
        if (values[i] != values[i] || std::fabs(values[i]) > bound)
            return FVM_INVALID_ARGUMENT;
    }
    return FVM_OK;
}
/* Read an optional public tuple while preserving identity/default values for null/undefined. */
FVM_Result optional(FVM_Context *c, unsigned n, const FVM_Value *args, unsigned index,
                    const char *const *names, unsigned count, double *values, double bound)
{
    if (n <= index || args[index].type == FVM_VALUE_NULL || args[index].type == FVM_VALUE_UNDEFINED)
        return FVM_OK;
    FVM_Object *object = 0;
    FVM_Result r = object_argument(c, n, args, index, object);
    return r == FVM_OK ? fields(c, object, names, count, values, bound) : r;
}
/* Resolve the fourteen documented integer/string blend modes without silently falling back. */
FVM_Result blend_mode(FVM_Context *c, unsigned n, const FVM_Value *args, unsigned &mode)
{
    mode = 1;
    if (n < 4 || args[3].type == FVM_VALUE_NULL || args[3].type == FVM_VALUE_UNDEFINED)
        return FVM_OK;
    if (args[3].type == FVM_VALUE_NUMBER)
    {
        if (args[3].number >= 1 && args[3].number <= 14 &&
            args[3].number == std::floor(args[3].number))
        {
            mode = (unsigned)args[3].number;
            return FVM_OK;
        }
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Value name;
    FVM_Result r = fvm_value_to_string(c, args + 3, &name);
    if (r != FVM_OK)
        return r;
    const char *const names[] = {"normal", "layer",      "multiply", "screen",   "lighten",
                                 "darken", "difference", "add",      "subtract", "invert",
                                 "alpha",  "erase",      "overlay",  "hardlight"};
    for (unsigned i = 0; i < 14; ++i)
        if (fvm_value_string_size(&name) == std::strlen(names[i]) &&
            !std::memcmp(name.string, names[i], std::strlen(names[i])))
        {
            mode = i + 1;
            return FVM_OK;
        }
    return FVM_INVALID_ARGUMENT;
}
/* Evaluate one separable RGB blend operation before alpha compositing. */
double blend_channel(unsigned mode, double source, double dest)
{
    switch (mode)
    {
    case 3:
        return source * dest / 255;
    case 4:
        return 255 - (255 - source) * (255 - dest) / 255;
    case 5:
        return source > dest ? source : dest;
    case 6:
        return source < dest ? source : dest;
    case 7:
        return std::fabs(source - dest);
    case 8:
        return source + dest > 255 ? 255 : source + dest;
    case 9:
        return dest > source ? dest - source : 0;
    case 10:
        return 255 - dest;
    case 13:
        return dest < 128 ? 2 * source * dest / 255 : 255 - 2 * (255 - source) * (255 - dest) / 255;
    case 14:
        return source < 128 ? 2 * source * dest / 255
                            : 255 - 2 * (255 - source) * (255 - dest) / 255;
    default:
        return source;
    }
}
/* Composite a flattened source pixel onto destination storage, enforcing opaque receiver alpha. */
void composite(unsigned mode, const unsigned char *source, unsigned char *dest, bool transparent,
               unsigned coverage)
{
    double sa = source[3] / 255.0, da = dest[3] / 255.0;
    if (mode == 11 || mode == 12)
    {
        if (!transparent || (mode == 11 ? !coverage : !source[3]))
            return;
        double factor = mode == 11 ? 1 - coverage / 255.0 + sa : 1 - sa;
        if (factor > 1)
            factor = 1;
        if (factor < 0)
            factor = 0;
        dest[3] = byte(dest[3] * factor);
        if (!dest[3])
            dest[0] = dest[1] = dest[2] = 0;
        return;
    }
    if (!sa)
        return;
    double alpha = sa + da * (1 - sa);
    for (unsigned i = 0; i < 3; ++i)
    {
        double value = sa * (1 - da) * source[i] + (1 - sa) * da * dest[i] +
                       sa * da * blend_channel(mode, source[i], dest[i]);
        dest[i] = alpha ? byte(value / alpha) : 0;
    }
    dest[3] = transparent ? byte(alpha * 255) : 255;
}
/* Own a temporary bitmap rectangle path through all C++ error exits. */
struct RectanglePath
{
    FVM_Path *value; /* Owned path, released after the bitmap sampler returns. */
    /* Start with no allocated geometry. */
    RectanglePath() : value(0)
    {
    }
    /* Release any adopted C raster path. */
    ~RectanglePath()
    {
        fvm_path_destroy(value);
    }

  private:
    /* Keep path ownership unique. */
    RectanglePath(const RectanglePath &);
    /* Disable shallow copies of native handles. */
    RectanglePath &operator=(const RectanglePath &);
};
/* Draw one bitmap's full local rectangle using the common non-repeating affine sampler. */
FVM_Result bitmap_source(FVM_BitmapState *source, const FVM_BitmapPaint &options,
                         FVM_RasterSurface *destination, FVM_RasterSurface *coverage)
{
    FVM_BitmapPaint paint = options;
    FVM_RasterSurface input = {source->pixels.begin(), source->width, source->height,
                               (size_t)source->width * 4};
    paint.source = &input;
    RectanglePath path;
    FVM_Result r = fvm_path_create(&path.value);
    const double x[] = {0, (double)source->width, (double)source->width, 0, 0};
    const double y[] = {0, 0, (double)source->height, (double)source->height, 0};
    for (unsigned i = 0; r == FVM_OK && i < 5; ++i)
    {
        FVM_PathCommand command;
        std::memset(&command, 0, sizeof(command));
        command.verb = i == 4 ? FVM_PATH_CLOSE : i ? FVM_PATH_LINE : FVM_PATH_MOVE;
        command.x[0] = x[i];
        command.y[0] = y[i];
        r = fvm_path_add(path.value, &command);
    }
    if (r == FVM_OK)
        r = fvm_path_transform(path.value, &paint.matrix);
    if (r == FVM_OK)
        r = fvm_path_draw_bitmap(path.value, destination, &paint, 0);
    if (r == FVM_OK && coverage)
    {
        FVM_RasterColor opaque = {255, 255, 255, 255};
        r = fvm_path_draw(path.value, coverage, opaque, 0, 0);
    }
    return r;
}
} // namespace

/* [GC:] Snapshot source rendering before committing clipped pixels to the protected receiver. */
FVM_Result fvm_bitmap_draw(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args,
                           FVM_Value *out, void *)
{
    /* [GC:] Guard every borrowed source/class/receiver while public getter coercion can run. */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    FVM_BitmapState *target = fvm_bitmap_pixels(self);
    if (!target || target->disposed)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    FVM_Object *source = 0;
    FVM_Result r = object_argument(c, n, args, 0, source);
    if (r != FVM_OK)
        return r;
    FVM_BitmapPaint paint;
    r = fvm_bitmap_paint_init(&paint);
    if (r != FVM_OK)
        return r;
    paint.repeat = 0;
    paint.smoothing = n > 5 && truth(args[5]);
    const char *const matrix_names[] = {"a", "b", "c", "d", "tx", "ty"};
    double matrix[6] = {1, 0, 0, 1, 0, 0};
    r = optional(c, n, args, 1, matrix_names, 6, matrix, 1000000);
    const char *const color_names[] = {"redMultiplier",   "greenMultiplier", "blueMultiplier",
                                       "alphaMultiplier", "redOffset",       "greenOffset",
                                       "blueOffset",      "alphaOffset"};
    double color[8] = {1, 1, 1, 1, 0, 0, 0, 0};
    if (r == FVM_OK)
        r = optional(c, n, args, 2, color_names, 8, color, 65536);
    unsigned mode = 1;
    if (r == FVM_OK)
        r = blend_mode(c, n, args, mode);
    const char *const rect_names[] = {"x", "y", "width", "height"};
    double clip[4] = {0, 0, (double)target->width, (double)target->height};
    if (r == FVM_OK)
        r = optional(c, n, args, 4, rect_names, 4, clip, 2000000);
    if (r != FVM_OK)
        return r;
    /* Getter side effects may dispose either resource; refresh storage before borrowing pixels. */
    target = fvm_bitmap_pixels(self);
    FVM_BitmapState *bitmap = fvm_bitmap_pixels(source);
    if (!target || target->disposed || (bitmap && bitmap->disposed))
        return FVM_INVALID_ARGUMENT;
    if (!bitmap && source->resource_kind != 40)
        return FVM_INVALID_ARGUMENT;
    paint.matrix.xx = matrix[0];
    paint.matrix.yx = matrix[1];
    paint.matrix.xy = matrix[2];
    paint.matrix.yy = matrix[3];
    paint.matrix.x0 = matrix[4];
    paint.matrix.y0 = matrix[5];
    for (unsigned i = 0; i < 4; ++i)
    {
        paint.multiply[i] = color[i];
        paint.add[i] = color[i + 4];
    }
    if (clip[2] <= 0 || clip[3] <= 0 ||
        std::fabs(matrix[0] * matrix[3] - matrix[1] * matrix[2]) < 1e-20)
        return FVM_OK;
    unsigned width = target->width, height = target->height;
    size_t bytes = (size_t)width * height * 4;
    unsigned long limit = c->runtime->config.memory_limit_bytes;
    if (limit && bytes > limit / (mode == 11 ? 4 : 3))
        return FVM_OUT_OF_MEMORY;
    fvm_detail::vector<unsigned char> staged(bytes), covered;
    if (mode == 11)
        covered.resize(bytes);
    FVM_RasterSurface geometry = {covered.begin(), width, height, (size_t)width * 4};
    FVM_RasterSurface *coverage = mode == 11 ? &geometry : 0;
    FVM_RasterSurface surface = {staged.begin(), width, height, (size_t)width * 4};
    r = bitmap ? bitmap_source(bitmap, paint, &surface, coverage)
               : fvm_movieclip_render_bitmap(c, source, &surface, paint.matrix, paint.multiply,
                                             paint.add, coverage);
    if (r != FVM_OK)
        return r;
    if (target->disposed || target->width != width || target->height != height)
        return FVM_INVALID_ARGUMENT;
    /* No allocations or script callbacks occur after this point: publication cannot fail halfway.
     */
    for (unsigned y = 0; y < height; ++y)
        for (unsigned x = 0; x < width; ++x)
            if (x + .5 >= clip[0] && x + .5 < clip[0] + clip[2] && y + .5 >= clip[1] &&
                y + .5 < clip[1] + clip[3])
            {
                size_t offset = ((size_t)y * width + x) * 4;
                composite(mode, staged.begin() + offset, target->pixels.begin() + offset,
                          target->transparent, coverage ? covered[offset + 3] : 0);
            }
    return FVM_OK;
}
