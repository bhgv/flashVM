/* FlashVM: retained BitmapData drawing styles with copied matrices and live pixel/color sampling.
 */
#include "../render/quality_internal.hpp"

#include "bitmap_fill.hpp"
#include "bitmap_state.hpp"
#include "movieclip_scene.hpp"
#include "geometry_support.hpp"
#include "../core/native_call_guard.hpp"

namespace
{
using namespace fvm_geometry_detail;
typedef fvm_detail::map<fvm_detail::string, FVM_Value> Properties;
/* [GC:] Represent an optional retained graph edge, using null for absent active paths. */
FVM_Value link(FVM_Object *object)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = object ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    value.object = object;
    return value;
}
/* Apply ActionScript truthiness to optional repeat/smoothing flags. */
bool flag(const FVM_Value &value)
{
    if (value.type == FVM_VALUE_BOOLEAN)
        return value.boolean != 0;
    if (value.type == FVM_VALUE_NUMBER)
        return value.number != 0 && value.number == value.number;
    if (value.type == FVM_VALUE_STRING)
        return fvm_value_string_size(&value) != 0;
    return value.type == FVM_VALUE_OBJECT;
}
/* Read bounded scalar metadata without evaluating public getters or mutating the private graph. */
FVM_Result number(FVM_Object *object, const char *key, double &out)
{
    Properties::iterator i = object->properties.find(key);
    if (i == object->properties.end() || i->second.type != FVM_VALUE_NUMBER ||
        !(i->second.number >= -1000000 && i->second.number <= 1000000))
        return FVM_INVALID_ARGUMENT;
    out = i->second.number;
    return FVM_OK;
}
/* Compose source-to-clip and clip-to-destination matrices without introducing pixel coordinates
 * into SWF twips. */
FVM_Affine multiply(const FVM_Affine &a, const FVM_Affine &b)
{
    FVM_Affine out;
    out.xx = a.xx * b.xx + a.xy * b.yx;
    out.xy = a.xx * b.xy + a.xy * b.yy;
    out.yx = a.yx * b.xx + a.yy * b.yx;
    out.yy = a.yx * b.xy + a.yy * b.yy;
    out.x0 = a.xx * b.x0 + a.xy * b.y0 + a.x0;
    out.y0 = a.yx * b.x0 + a.yy * b.y0 + a.y0;
    return out;
}
} // namespace
/* [GC:] Publish a live bitmap reference and copied style only after all coercion/allocation
 * succeeds. */
FVM_Result fvm_movieclip_bitmap_begin(FVM_Context *c, FVM_Object *clip, unsigned n,
                                      const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (!c || !clip || clip->owner != c)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Keep the clip and argument graphs alive during getters and unpublished allocations. */
    FVM_NativeCallGuard guard(c);
    /* [GC:] Borrow the clip's traced private drawing state for this call. */
    FVM_Object *data = 0;
    FVM_Result result = fvm_movieclip_scene_state(clip, &data);
    if (result != FVM_OK)
        return result;
    if (!n || args[0].type == FVM_VALUE_UNDEFINED || args[0].type == FVM_VALUE_NULL)
    {
        /* [GC:] Stage edge removals so allocation failure preserves the previous drawing style. */
        Properties next = data->properties;
        next["fill"] = scalar(0);
        next["active"] = link(0);
        /* [GC:] Existing path records retain their own bitmap/gradient descriptors. */
        next.erase("bitmap");
        next.erase("gradient");
        /* [GC:] Publish the staged drawing state atomically. */
        data->properties.swap(next);
        return FVM_OK;
    }
    FVM_Object *image = 0;
    result = object_argument(c, n, args, 0, image);
    FVM_BitmapPaint paint;
    fvm_bitmap_paint_init(&paint);
    if (result == FVM_OK && n > 1 && args[1].type != FVM_VALUE_UNDEFINED &&
        args[1].type != FVM_VALUE_NULL)
    {
        FVM_Object *matrix = 0;
        result = object_argument(c, n, args, 1, matrix);
        const char *keys[] = {"a", "b", "c", "d", "tx", "ty"};
        double values[6];
        if (result == FVM_OK)
            result = numbers(c, matrix, keys, 6, values);
        if (result == FVM_OK)
        {
            paint.matrix.xx = values[0];
            paint.matrix.yx = values[1];
            paint.matrix.xy = values[2];
            paint.matrix.yy = values[3];
            paint.matrix.x0 = values[4];
            paint.matrix.y0 = values[5];
            for (unsigned i = 0; i < 6; ++i)
                if (!(values[i] >= -1000000 && values[i] <= 1000000))
                    result = FVM_INVALID_ARGUMENT;
        }
    }
    if (result != FVM_OK)
        return result;
    /* [GC:] Re-resolve the bitmap after all user-controlled matrix getters may have disposed it. */
    FVM_BitmapState *pixels = fvm_bitmap_pixels(image);
    if (!pixels || pixels->disposed)
        return FVM_INVALID_ARGUMENT;
    double determinant = paint.matrix.xx * paint.matrix.yy - paint.matrix.xy * paint.matrix.yx;
    if (!(determinant >= 1e-20 || determinant <= -1e-20))
        return FVM_INVALID_ARGUMENT;
    bool repeat = n > 2 && args[2].type != FVM_VALUE_UNDEFINED ? flag(args[2]) : true;
    bool smoothing = n > 3 ? flag(args[3]) : false;
    /* [GC:] Hold the unpublished immutable descriptor under the native barrier. */
    FVM_Object *record = 0;
    /* [GC:] Allocate the record that will trace the bitmap instance. */
    result = fvm_object_create(c, &record);
    if (result != FVM_OK)
        return result;
    /* [GC:] Retain source identity; setPixel/dispose are observed by subsequent render calls. */
    record->properties["source"] = link(image);
    record->properties["xx"] = scalar(paint.matrix.xx);
    record->properties["yx"] = scalar(paint.matrix.yx);
    record->properties["xy"] = scalar(paint.matrix.xy);
    record->properties["yy"] = scalar(paint.matrix.yy);
    record->properties["x0"] = scalar(paint.matrix.x0);
    record->properties["y0"] = scalar(paint.matrix.y0);
    record->properties["repeat"] = scalar(repeat ? 1 : 0);
    record->properties["smoothing"] = scalar(smoothing ? 1 : 0);
    /* [GC:] Stage ownership links before publishing the completed style. */
    Properties next = data->properties;
    /* [GC:] The drawing state owns the new descriptor until each path retains it. */
    next["bitmap"] = link(record);
    /* [GC:] Start a new independently owned vector path on the following command. */
    next["active"] = link(0);
    next["fill"] = scalar(1);
    next["rgb"] = scalar(0);
    next["alpha"] = scalar(100);
    /* [GC:] Replaced gradient ownership stays only in previously completed paths. */
    next.erase("gradient");
    /* [GC:] Publish all links atomically; discarded old style edges can then be collected. */
    data->properties.swap(next);
    return FVM_OK;
}
/* [GC:] Render current pixels through the retained style and composed world/color transforms. */
FVM_Result fvm_movieclip_bitmap_draw(FVM_Context *c, FVM_Object *part, const FVM_Affine &world,
                                     const FVM_Path *path, FVM_RasterSurface *destination,
                                     const double *multiply_values, const double *add, int even_odd,
                                     double stroke_width)
{
    Properties::iterator b = part->properties.find(stroke_width > 0 ? "lineBitmap" : "bitmap");
    if (b == part->properties.end() || b->second.type != FVM_VALUE_OBJECT || !b->second.object ||
        b->second.object->owner != c)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] The scene paint retains this descriptor throughout the native render barrier. */
    FVM_Object *record = b->second.object;
    Properties::iterator s = record->properties.find("source");
    if (s == record->properties.end() || s->second.type != FVM_VALUE_OBJECT || !s->second.object ||
        s->second.object->owner != c)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] The retained bitmap instance owns this borrowed pixel buffer. */
    FVM_BitmapState *pixels = fvm_bitmap_pixels(s->second.object);
    if (!pixels)
        return FVM_INVALID_ARGUMENT;
    if (pixels->disposed)
        return FVM_OK;
    unsigned long limit = c->runtime->config.memory_limit_bytes;
    if (limit && (size_t)destination->width * destination->height * 8 > limit)
        return FVM_OUT_OF_MEMORY;
    FVM_BitmapPaint paint;
    fvm_bitmap_paint_init(&paint);
    FVM_Affine local;
    double repeat = 0, smoothing = 0;
    FVM_Result result = number(record, "xx", local.xx);
    if (result == FVM_OK)
        result = number(record, "yx", local.yx);
    if (result == FVM_OK)
        result = number(record, "xy", local.xy);
    if (result == FVM_OK)
        result = number(record, "yy", local.yy);
    if (result == FVM_OK)
        result = number(record, "x0", local.x0);
    if (result == FVM_OK)
        result = number(record, "y0", local.y0);
    if (result == FVM_OK)
        result = number(record, "repeat", repeat);
    if (result == FVM_OK)
        result = number(record, "smoothing", smoothing);
    if (result != FVM_OK || (repeat != 0 && repeat != 1) || (smoothing != 0 && smoothing != 1))
        return FVM_INVALID_ARGUMENT;
    FVM_RasterSurface source = {pixels->pixels.begin(), pixels->width, pixels->height,
                                (size_t)pixels->width * 4};
    paint.source = &source;
    paint.matrix = multiply(world, local);
    paint.repeat = (int)repeat;
    paint.smoothing = (int)smoothing;
    for (unsigned i = 0; i < 4; ++i)
    {
        paint.multiply[i] = multiply_values[i];
        paint.add[i] = add[i];
    }
    return fvm_path_bitmap_paint_quality(path, destination, &paint, stroke_width, even_odd,
                                         c->stage_quality != FVM_STAGE_QUALITY_LOW);
}
