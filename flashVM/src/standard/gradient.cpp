/* FlashVM: restore portable gradient descriptions from managed scene paint records. */
#include "../render/quality_internal.hpp"

#include "gradient.hpp"
#include <cmath>

namespace
{
/* Read private numeric metadata without invoking script or inserting a missing property. */
FVM_Result number(FVM_Object *o, const char *key, double &out)
{
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i = o->properties.find(key);
    if (i == o->properties.end() || i->second.type != FVM_VALUE_NUMBER ||
        !(i->second.number >= -1000000 && i->second.number <= 1000000))
        return FVM_INVALID_ARGUMENT;
    out = i->second.number;
    return FVM_OK;
}

/* [GC:] Validate a managed gradient edge before following it across graph restoration. */
FVM_Result object(FVM_Context *c, FVM_Object *o, const char *key, FVM_Object *&out)
{
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i = o->properties.find(key);
    if (i == o->properties.end() || i->second.type != FVM_VALUE_OBJECT || !i->second.object ||
        i->second.object->owner != c)
        return FVM_INVALID_ARGUMENT;
    out = i->second.object;
    return FVM_OK;
}

/* Read one exact unsigned byte from managed stop metadata. */
FVM_Result byte(FVM_Object *o, const char *key, unsigned char &out)
{
    double value = 0;
    FVM_Result r = number(o, key, value);
    if (r != FVM_OK || value < 0 || value > 255 || value != std::floor(value))
        return FVM_INVALID_ARGUMENT;
    out = (unsigned char)value;
    return FVM_OK;
}
} // namespace

/* [GC:] Decode borrowed graph edges under the caller's native barrier and paint through its mask.
 */
FVM_Result fvm_movieclip_gradient_draw(FVM_Context *c, FVM_Object *paint, const FVM_Affine &world,
                                       const FVM_Path *path, FVM_RasterSurface *surface,
                                       const double multiply[4], const double add[4], int even_odd,
                                       double stroke_width)
{
    if (!c || !paint || paint->owner != c || !surface || !multiply || !add)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] The scene owns these immutable descriptors throughout the native render call. */
    FVM_Object *record = 0, *stops = 0;
    FVM_Result r = object(c, paint, stroke_width > 0 ? "lineGradient" : "gradient", record);
    if (r == FVM_OK)
        r = object(c, record, "stops", stops);
    if (r != FVM_OK)
        return r;
    if (!stops->indexed.size() || stops->indexed.size() > FVM_GRADIENT_MAX_STOPS)
        return FVM_INVALID_ARGUMENT;
    unsigned long configured = c->runtime->config.memory_limit_bytes;
    if (configured && (size_t)surface->width * surface->height * 8 > configured)
        return FVM_OUT_OF_MEMORY;
    FVM_GradientSpread spread = FVM_GRADIENT_SPREAD_PAD;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator mode = record->properties.find("spread");
    if (mode != record->properties.end())
    {
        const FVM_Value &value = mode->second;
        if (value.type != FVM_VALUE_NUMBER || !(value.number >= FVM_GRADIENT_SPREAD_PAD &&
            value.number <= FVM_GRADIENT_SPREAD_REPEAT) || value.number != std::floor(value.number))
            return FVM_INVALID_ARGUMENT;
        spread = (FVM_GradientSpread)(unsigned int)value.number;
    }
    unsigned int interpolation = FVM_GRADIENT_INTERPOLATION_RGB;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator color_mode =
        record->properties.find("interpolation");
    if (color_mode != record->properties.end())
    {
        const FVM_Value &value = color_mode->second;
        if (value.type != FVM_VALUE_NUMBER ||
            (value.number != FVM_GRADIENT_INTERPOLATION_RGB &&
             value.number != FVM_GRADIENT_INTERPOLATION_LINEAR_RGB))
            return FVM_INVALID_ARGUMENT;
        interpolation = (unsigned int)value.number;
    }
    double focal = 0;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator focal_mode =
        record->properties.find("focal");
    if (focal_mode != record->properties.end())
    {
        const FVM_Value &value = focal_mode->second;
        if (value.type != FVM_VALUE_NUMBER || !(value.number > -1 && value.number < 1))
            return FVM_INVALID_ARGUMENT;
        focal = value.number;
    }
    FVM_Gradient gradient;
    fvm_gradient_init(&gradient);
    double type = 0;
    r = number(record, "type", type);
    if (r != FVM_OK || (type != 0 && type != 1) || (focal != 0 && type != 1))
        return FVM_INVALID_ARGUMENT;
    gradient.type = type == 0 ? FVM_GRADIENT_LINEAR : FVM_GRADIENT_RADIAL;
    FVM_Affine local;
    r = number(record, "xx", local.xx);
    if (r == FVM_OK)
        r = number(record, "yx", local.yx);
    if (r == FVM_OK)
        r = number(record, "xy", local.xy);
    if (r == FVM_OK)
        r = number(record, "yy", local.yy);
    if (r == FVM_OK)
        r = number(record, "x0", local.x0);
    if (r == FVM_OK)
        r = number(record, "y0", local.y0);
    if (r != FVM_OK)
        return r;
    /* Normalize SWF's +/-16384 twips before composing shape-local and world transforms. */
    gradient.matrix.xx = 819.2 * (world.xx * local.xx + world.xy * local.yx);
    gradient.matrix.yx = 819.2 * (world.yx * local.xx + world.yy * local.yx);
    gradient.matrix.xy = 819.2 * (world.xx * local.xy + world.xy * local.yy);
    gradient.matrix.yy = 819.2 * (world.yx * local.xy + world.yy * local.yy);
    gradient.matrix.x0 = world.xx * local.x0 + world.xy * local.y0 + world.x0;
    gradient.matrix.y0 = world.yx * local.x0 + world.yy * local.y0 + world.y0;
    for (unsigned int i = 0; i < 4; ++i)
    {
        gradient.multiply[i] = multiply[i];
        gradient.add[i] = add[i];
    }
    fvm_detail::vector<FVM_GradientStop> values(stops->indexed.size());
    for (unsigned long i = 0; i < values.size(); ++i)
    {
        fvm_detail::map<unsigned long, FVM_Value>::iterator entry = stops->indexed.find(i);
        if (entry == stops->indexed.end() || entry->second.type != FVM_VALUE_OBJECT ||
            !entry->second.object || entry->second.object->owner != c)
            return FVM_INVALID_ARGUMENT;
        /* [GC:] The stop remains reachable from the paint while its scalar channels are copied. */
        FVM_Object *stop = entry->second.object;
        unsigned char ratio = 0;
        r = byte(stop, "ratio", ratio);
        if (r == FVM_OK)
            r = byte(stop, "r", values[i].color.r);
        if (r == FVM_OK)
            r = byte(stop, "g", values[i].color.g);
        if (r == FVM_OK)
            r = byte(stop, "b", values[i].color.b);
        if (r == FVM_OK)
            r = byte(stop, "a", values[i].color.a);
        if (r != FVM_OK)
            return r;
        values[i].ratio = ratio / 255.0;
    }
    gradient.stops = values.begin();
    gradient.count = values.size();
    return fvm_path_gradient_focal_quality(path, surface, &gradient, stroke_width, even_odd,
                                            spread, interpolation, focal,
                                            c->stage_quality != FVM_STAGE_QUALITY_LOW);
}
