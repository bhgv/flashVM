/* FlashVM: ActionScript MovieClip gradient arguments copied into portable managed drawing state. */
#include "gradient.hpp"
#include "gradient_constants.h"
#include "movieclip_scene.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/standard.h"
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <cstring>

namespace
{
typedef fvm_detail::map<fvm_detail::string, FVM_Value> Properties;

/* Construct one scalar for the private drawing description. */
FVM_Value scalar(double n)
{
    FVM_Value value;
    fvm_value_number(&value, n);
    return value;
}

/* [GC:] Construct a managed edge protected by the gradient call's native barrier. */
FVM_Value managed(FVM_Object *o)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = o ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    value.object = o;
    return value;
}

/* Compare the entire counted string so embedded NUL bytes never truncate an option. */
bool equal(const FVM_Value &value, const char *text)
{
    return value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == std::strlen(text) &&
           std::memcmp(value.string, text, std::strlen(text)) == 0;
}

/* Coerce a script number and reject nonfinite or unbounded geometry. */
FVM_Result numeric(FVM_Context *c, const FVM_Value &value, double &out, double limit)
{
    FVM_Result r = fvm_value_to_number(c, &value, &out);
    return r != FVM_OK ? r : out >= -limit && out <= limit ? FVM_OK : FVM_INVALID_ARGUMENT;
}

/* Read a public numeric matrix property, using a default only for an undefined optional field. */
FVM_Result property(FVM_Context *c, FVM_Object *o, const char *key, double &out,
                    bool optional = false, double fallback = 0)
{
    FVM_Value value;
    FVM_Result r = fvm_object_get(c, o, key, &value);
    if (r != FVM_OK)
        return r;
    if (optional && value.type == FVM_VALUE_UNDEFINED)
    {
        out = fallback;
        return FVM_OK;
    }
    return numeric(c, value, out, 1000000);
}

/* Accept legacy box or Matrix-shaped a/b/c/d/tx/ty objects; nine-field matrices remain unsupported.
 */
FVM_Result matrix(FVM_Context *c, FVM_Object *o, FVM_Affine &out)
{
    FVM_Value type;
    FVM_Result r = fvm_object_get(c, o, "matrixType", &type);
    if (r != FVM_OK)
        return r;
    if (equal(type, "box"))
    {
        double width = 0, height = 0, rotation = 0, x = 0, y = 0;
        r = property(c, o, "w", width);
        if (r == FVM_OK)
            r = property(c, o, "h", height);
        if (r == FVM_OK)
            r = property(c, o, "r", rotation, true);
        if (r == FVM_OK)
            r = property(c, o, "x", x, true);
        if (r == FVM_OK)
            r = property(c, o, "y", y, true);
        if (r != FVM_OK)
            return r;
        double cosine = std::cos(rotation), sine = std::sin(rotation);
        out.xx = cosine * width / 1638.4;
        out.yx = sine * height / 1638.4;
        out.xy = -sine * width / 1638.4;
        out.yy = cosine * height / 1638.4;
        out.x0 = x + width / 2;
        out.y0 = y + height / 2;
    }
    else
    {
        FVM_Value legacy;
        r = fvm_object_get(c, o, "i", &legacy);
        if (r != FVM_OK)
            return r;
        if (legacy.type != FVM_VALUE_UNDEFINED)
            return FVM_UNSUPPORTED;
        r = property(c, o, "a", out.xx);
        if (r == FVM_OK)
            r = property(c, o, "b", out.yx);
        if (r == FVM_OK)
            r = property(c, o, "c", out.xy);
        if (r == FVM_OK)
            r = property(c, o, "d", out.yy);
        if (r == FVM_OK)
            r = property(c, o, "tx", out.x0);
        if (r == FVM_OK)
            r = property(c, o, "ty", out.y0);
        if (r != FVM_OK)
            return r;
    }
    double determinant = out.xx * out.yy - out.xy * out.yx;
    return std::fabs(determinant) * 819.2 * 819.2 >= 1e-20 ? FVM_OK : FVM_INVALID_ARGUMENT;
}

/* Read an array-like object's bounded length through ordinary ActionScript property lookup. */
FVM_Result length(FVM_Context *c, const FVM_Value &value, unsigned int &out)
{
    if (value.type != FVM_VALUE_OBJECT || !value.object || value.object->owner != c)
        return FVM_INVALID_ARGUMENT;
    double count = 0;
    FVM_Result r = property(c, value.object, "length", count);
    if (r != FVM_OK || count < 1 || count > FVM_GRADIENT_MAX_STOPS || count != std::floor(count))
        return r != FVM_OK ? r : FVM_INVALID_ARGUMENT;
    out = (unsigned int)count;
    return FVM_OK;
}

/* Read one inherited or accessor-backed array element without separating numeric/name semantics. */
FVM_Result element(FVM_Context *c, FVM_Object *o, unsigned int index, FVM_Value &out)
{
    char key[24];
    std::sprintf(key, "%u", index);
    return fvm_object_get(c, o, key, &out);
}

/* Convert the numeric color with truncation toward zero and modulo 24 bits. */
unsigned long rgb(double value)
{
    if (!(value >= -DBL_MAX && value <= DBL_MAX))
        return 0;
    double packed = std::fmod(value < 0 ? std::ceil(value) : std::floor(value), 16777216.0);
    if (packed < 0)
        packed += 16777216.0;
    return (unsigned long)packed;
}

/* Copy and coerce the three equal-length arrays before publishing any new drawing style. */
FVM_Result stops(FVM_Context *c, const FVM_Value *args, FVM_GradientStop *out, unsigned int &count)
{
    unsigned int colors = 0, alphas = 0, ratios = 0;
    FVM_Result r = length(c, args[1], colors);
    if (r == FVM_OK)
        r = length(c, args[2], alphas);
    if (r == FVM_OK)
        r = length(c, args[3], ratios);
    if (r != FVM_OK || colors != alphas || colors != ratios)
        return r != FVM_OK ? r : FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < colors; ++i)
    {
        FVM_Value value;
        double color = 0, alpha = 0, ratio = 0;
        r = element(c, args[1].object, i, value);
        if (r == FVM_OK)
            r = fvm_value_to_number(c, &value, &color);
        if (r == FVM_OK)
            r = element(c, args[2].object, i, value);
        if (r == FVM_OK)
            r = numeric(c, value, alpha, DBL_MAX);
        if (r == FVM_OK)
            r = element(c, args[3].object, i, value);
        if (r == FVM_OK)
            r = numeric(c, value, ratio, DBL_MAX);
        if (r != FVM_OK)
            return r;
        if (!(ratio > -1 && ratio < 256))
            return FVM_INVALID_ARGUMENT;
        unsigned int position = ratio < 0 ? 0 : (unsigned int)ratio;
        if (i && position / 255.0 < out[i - 1].ratio)
            return FVM_INVALID_ARGUMENT;
        unsigned long packed = rgb(color);
        out[i].ratio = position / 255.0;
        out[i].color.r = (unsigned char)(packed >> 16);
        out[i].color.g = (unsigned char)(packed >> 8);
        out[i].color.b = (unsigned char)packed;
        out[i].color.a = (unsigned char)((alpha < 0 ? 0 : alpha > 100 ? 100 : alpha) * 255 / 100);
    }
    count = colors;
    return FVM_OK;
}

/* [GC:] Allocate a copied ramp whose ownership is later transferred atomically to the clip state.
 */
FVM_Result descriptor(FVM_Context *c, const FVM_Affine &matrix, bool radial,
                      FVM_GradientSpread spread, unsigned int interpolation, double focal,
                      const FVM_GradientStop *values, unsigned int count, FVM_Object **out)
{
    /* [GC:] Temporary record and stop-table edges are protected by the native barrier. */
    FVM_Object *record = 0, *table = 0;
    /* [GC:] Allocate the immutable matrix/type record. */
    FVM_Result r = fvm_object_create(c, &record);
    if (r == FVM_OK)
        /* [GC:] Allocate its managed ordered control-point table. */
        r = fvm_object_create(c, &table);
    if (r != FVM_OK)
        return r;
    record->properties["type"] = scalar(radial ? 1 : 0);
    if (spread != FVM_GRADIENT_SPREAD_PAD)
        record->properties["spread"] = scalar(spread); /* Legacy records omit the default mode. */
    if (interpolation != FVM_GRADIENT_INTERPOLATION_RGB)
        record->properties["interpolation"] = scalar(interpolation); /* Missing means legacy RGB. */
    if (focal != 0)
        record->properties["focal"] = scalar(focal); /* Legacy centered records omit the default. */
    record->properties["xx"] = scalar(matrix.xx);
    record->properties["yx"] = scalar(matrix.yx);
    record->properties["xy"] = scalar(matrix.xy);
    record->properties["yy"] = scalar(matrix.yy);
    record->properties["x0"] = scalar(matrix.x0);
    record->properties["y0"] = scalar(matrix.y0);
    /* [GC:] The descriptor owns its table before any stops are constructed. */
    record->properties["stops"] = managed(table);
    for (unsigned int i = 0; i < count; ++i)
    {
        /* [GC:] Hold one stop until its table retains the copied scalar data. */
        FVM_Object *stop = 0;
        /* [GC:] Allocate a managed control point without borrowing the caller's arrays. */
        r = fvm_object_create(c, &stop);
        if (r != FVM_OK)
            return r;
        stop->properties["ratio"] = scalar(std::floor(values[i].ratio * 255 + 0.5));
        stop->properties["r"] = scalar(values[i].color.r);
        stop->properties["g"] = scalar(values[i].color.g);
        stop->properties["b"] = scalar(values[i].color.b);
        stop->properties["a"] = scalar(values[i].color.a);
        /* [GC:] Publish the stop edge before allocating the next record. */
        table->indexed[i] = managed(stop);
    }
    /* [GC:] Return the finished graph for adoption by the clip's drawing state. */
    *out = record;
    return FVM_OK;
}
} // namespace

/* Copy a spread/RGB gradient style; malformed arguments leave the previous style unchanged. */
static FVM_Result gradient_begin(FVM_Context *c, FVM_Object *clip, unsigned int argc,
                                   const FVM_Value *args, FVM_Value *out, bool stroke)
{
    fvm_value_undefined(out);
    if (!c || !clip || clip->owner != c)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Script coercions may re-enter VM code while native temporaries hold managed edges. */
    FVM_NativeCallGuard guard(c);
    /* [GC:] Borrow the clip's managed drawing state for the duration of this callback. */
    FVM_Object *data = 0;
    FVM_Result r = fvm_movieclip_scene_state(clip, &data);
    if (r != FVM_OK)
        return r;
    if (stroke)
    {
        if (c->swf_loaded && c->swf_info.version < FVM_MOVIECLIP_GRADIENT_SPREAD_MIN_VERSION)
            return FVM_UNSUPPORTED;
        Properties::iterator line = data->properties.find("stroke");
        /* A gradient changes an enabled stroke; it never enables a missing lineStyle itself. */
        if (line == data->properties.end() || line->second.type != FVM_VALUE_NUMBER ||
            !line->second.number || !argc || args[0].type == FVM_VALUE_UNDEFINED)
            return FVM_OK;
    }
    if (!argc || args[0].type == FVM_VALUE_UNDEFINED)
    {
        data->properties["fill"] = scalar(0);
        /* [GC:] Drop the active/current edges; existing paths still retain their own gradients. */
        data->properties["active"] = managed(0);
        /* [GC:] Releasing the current descriptor allows collection when no completed path owns it.
         */
        data->properties.erase("gradient");
        /* [GC:] No active bitmap style remains after disabling the fill. */
        data->properties.erase("bitmap");
        return FVM_OK;
    }
    if (argc < 5 || argc > 8)
        return FVM_OK;
    FVM_Value type;
    r = fvm_value_to_string(c, args, &type);
    if (r != FVM_OK)
        return r;
    if (!equal(type, "linear") && !equal(type, "radial"))
        return FVM_OK;
    bool radial = equal(type, "radial");
    FVM_GradientSpread mode = FVM_GRADIENT_SPREAD_PAD;
    if (argc > 5 && args[5].type != FVM_VALUE_UNDEFINED)
    {
        FVM_Value spread;
        r = fvm_value_to_string(c, args + 5, &spread);
        if (r != FVM_OK)
            return r;
        if (equal(spread, "reflect")) mode = FVM_GRADIENT_SPREAD_REFLECT;
        else if (equal(spread, "repeat")) mode = FVM_GRADIENT_SPREAD_REPEAT;
        else if (!equal(spread, "pad")) return FVM_UNSUPPORTED;
        if (mode != FVM_GRADIENT_SPREAD_PAD && c->swf_loaded &&
            c->swf_info.version < FVM_MOVIECLIP_GRADIENT_SPREAD_MIN_VERSION)
            return FVM_UNSUPPORTED;
    }
    unsigned int interpolation_mode = FVM_GRADIENT_INTERPOLATION_RGB;
    if (argc > 6 && args[6].type != FVM_VALUE_UNDEFINED)
    {
        FVM_Value interpolation;
        r = fvm_value_to_string(c, args + 6, &interpolation);
        if (r != FVM_OK)
            return r;
        if (equal(interpolation, "linearRGB"))
            interpolation_mode = FVM_GRADIENT_INTERPOLATION_LINEAR_RGB;
        else if (!equal(interpolation, "RGB"))
            return FVM_UNSUPPORTED;
        if (interpolation_mode != FVM_GRADIENT_INTERPOLATION_RGB && c->swf_loaded &&
            c->swf_info.version < FVM_MOVIECLIP_GRADIENT_INTERPOLATION_MIN_VERSION)
            return FVM_UNSUPPORTED;
    }
    double focal = 0;
    if (radial && argc > 7 && args[7].type != FVM_VALUE_UNDEFINED)
    {
        r = numeric(c, args[7], focal, DBL_MAX);
        if (r != FVM_OK)
            return r == FVM_INVALID_ARGUMENT ? FVM_OK : r;
        if (!(focal > -1 && focal < 1))
            return FVM_UNSUPPORTED; /* Boundary/clamped endpoint policies remain explicit. */
        if (focal != 0 && c->swf_loaded &&
            c->swf_info.version < FVM_MOVIECLIP_GRADIENT_FOCAL_MIN_VERSION)
            return FVM_UNSUPPORTED;
    }
    FVM_GradientStop values[FVM_GRADIENT_MAX_STOPS];
    unsigned int count = 0;
    r = stops(c, args, values, count);
    if (r != FVM_OK)
        return r == FVM_INVALID_ARGUMENT ? FVM_OK : r;
    if (args[4].type != FVM_VALUE_OBJECT || !args[4].object || args[4].object->owner != c)
        return FVM_OK;
    FVM_Affine local;
    r = matrix(c, args[4].object, local);
    if (r != FVM_OK)
        return r == FVM_INVALID_ARGUMENT ? FVM_OK : r;
    /* [GC:] Hold the complete new descriptor until its drawing-state map is published. */
    FVM_Object *record = 0;
    r = descriptor(c, local, radial, mode, interpolation_mode, focal, values, count, &record);
    if (r != FVM_OK)
        return r;
    /* [GC:] Stage all drawing links before adopting the new style; OOM preserves the old style. */
    Properties next = data->properties;
    /* [GC:] The clip owns the copied ramp independently of the caller's arrays and matrix. */
    next[stroke ? "lineGradient" : "gradient"] = managed(record);
    /* [GC:] A new gradient replaces the current bitmap style, not completed path descriptors. */
    if (!stroke) next.erase("bitmap");
    /* [GC:] A fresh path adopts this descriptor when the next geometry command is issued. */
    next["active"] = managed(0);
    if (!stroke)
    {
        next["fill"] = scalar(1);
        next["rgb"] = scalar(0);
        next["alpha"] = scalar(100);
    }
    /* [GC:] Publish the complete ownership graph; replaced temporary edges are then released. */
    data->properties.swap(next);
    return FVM_OK;
}

/* Preserve the existing fill callback identity and fill-only state transitions. */
FVM_Result fvm_movieclip_gradient_begin(FVM_Context *c, FVM_Object *clip, unsigned int argc,
                                        const FVM_Value *args, FVM_Value *out, void *)
{
    return gradient_begin(c, clip, argc, args, out, false);
}
/* [GC:] Share argument coercion, copied descriptors and transactional publication with fills. */
FVM_Result fvm_movieclip_line_gradient_begin(FVM_Context *c, FVM_Object *clip, unsigned int argc,
                                             const FVM_Value *args, FVM_Value *out, void *)
{
    return gradient_begin(c, clip, argc, args, out, true);
}
