/* FlashVM: encode decoded solid SWF shapes as managed vector records without native handles. */
#include "shape_graph.hpp"
#include "format_constants.h"
#include "../core/native_call_guard.hpp"

namespace
{
/* Make an initialized scalar for dictionary and vector metadata. */
FVM_Value number(double n)
{
    FVM_Value value;
    fvm_value_number(&value, n);
    return value;
}

/* [GC:] Make a managed edge protected by the definition's construction barrier. */
FVM_Value object(FVM_Object *o)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = o;
    return value;
}
/* [GC:] Encode a copied gradient ramp as ordinary graph edges owned by its paint. */
FVM_Result gradient(FVM_Context *c, const FVM_SWFShapePaint &paint, FVM_Object *part)
{
    /* [GC:] Hold temporary descriptors under the outer definition-construction barrier. */
    FVM_Object *record = 0, *stops = 0;
    /* [GC:] Allocate the gradient's immutable matrix and mode descriptor. */
    FVM_Result r = fvm_object_create(c, &record);
    if (r == FVM_OK)
        /* [GC:] Allocate the ordered control-point collection. */
        r = fvm_object_create(c, &stops);
    if (r != FVM_OK)
        return r;
    record->properties["type"] =
        number(paint.gradient_type == FVM_SWF_FILL_LINEAR_GRADIENT ? 0 : 1);
    /* Preserve optional SWF 8 modes using the same scalar-only graph as script gradients. */
    if (paint.gradient_spread)
        record->properties["spread"] = number(paint.gradient_spread);
    if (paint.gradient_interpolation)
        record->properties["interpolation"] = number(paint.gradient_interpolation);
    if (paint.gradient_focal)
        record->properties["focal"] = number(paint.gradient_focal);
    record->properties["xx"] = number(paint.gradient_matrix.xx);
    record->properties["yx"] = number(paint.gradient_matrix.yx);
    record->properties["xy"] = number(paint.gradient_matrix.xy);
    record->properties["yy"] = number(paint.gradient_matrix.yy);
    record->properties["x0"] = number(paint.gradient_matrix.x0);
    record->properties["y0"] = number(paint.gradient_matrix.y0);
    /* [GC:] The descriptor retains its stop table before any stops are allocated. */
    record->properties["stops"] = object(stops);
    /* [GC:] The vector paint owns its complete gradient description. */
    part->properties[paint.stroke ? "lineGradient" : "gradient"] = object(record);
    for (size_t i = 0; i < paint.stops.size(); ++i)
    {
        /* [GC:] Hold the newly created stop until its ordered table adopts it. */
        FVM_Object *stop = 0;
        /* [GC:] Allocate one scalar-only gradient control point. */
        r = fvm_object_create(c, &stop);
        if (r != FVM_OK)
            return r;
        stop->properties["ratio"] = number(paint.stops[i].ratio);
        stop->properties["r"] = number(paint.stops[i].color.r);
        stop->properties["g"] = number(paint.stops[i].color.g);
        stop->properties["b"] = number(paint.stops[i].color.b);
        stop->properties["a"] = number(paint.stops[i].color.a);
        /* [GC:] Publish this stop's edge before constructing the following stop. */
        stops->indexed[(unsigned long)i] = object(stop);
    }
    return FVM_OK;
}
/* [GC:] Bind one decoded bitmap style to the existing dictionary bitmap identity. */
FVM_Result bitmap(FVM_Context *c, const FVM_SWFShapePaint &paint, FVM_Object *part,
                  FVM_Object *definitions)
{
    if (!definitions || definitions->owner != c)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::map<unsigned long, FVM_Value>::iterator found =
        definitions->indexed.find(paint.bitmap_id);
    if (found == definitions->indexed.end() || found->second.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator source =
        found->second.object->properties.find("bitmap_data");
    if (source == found->second.object->properties.end() ||
        source->second.type != FVM_VALUE_OBJECT || !source->second.object ||
        source->second.object->owner != c)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] The enclosing native barrier protects the unpublished bitmap style. */
    FVM_Object *record = 0;
    /* [GC:] Allocate a scalar matrix/flags descriptor retaining its source bitmap. */
    FVM_Result r = fvm_object_create(c, &record);
    if (r != FVM_OK)
        return r;
    /* [GC:] Reuse source identity across all fills referring to this character. */
    record->properties["source"] = source->second;
    record->properties["xx"] = number(paint.bitmap_matrix.xx);
    record->properties["yx"] = number(paint.bitmap_matrix.yx);
    record->properties["xy"] = number(paint.bitmap_matrix.xy);
    record->properties["yy"] = number(paint.bitmap_matrix.yy);
    record->properties["x0"] = number(paint.bitmap_matrix.x0);
    record->properties["y0"] = number(paint.bitmap_matrix.y0);
    record->properties["repeat"] =
        number(paint.bitmap_type == FVM_SWF_FILL_BITMAP_REPEAT ||
                       paint.bitmap_type == FVM_SWF_FILL_BITMAP_REPEAT_NEAREST
                   ? 1
                   : 0);
    record->properties["smoothing"] = number(paint.bitmap_type < 0x42 ? 1 : 0);
    /* [GC:] The paint owns the completed descriptor before the next path is constructed. */
    part->properties[paint.stroke ? "lineBitmap" : "bitmap"] = object(record);
    return FVM_OK;
}
} // namespace

/* [GC:] Construct all paints and commands before returning a publishable immutable definition. */
FVM_Result fvm_swf_shape_build(FVM_Context *c, const FVM_SWFShape &shape, FVM_Object **out,
                               FVM_Object *bitmap_definitions)
{
    if (!c || !out || !shape.id)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    /* [GC:] The caller adopts the returned graph; collection cannot enter during construction. */
    FVM_NativeCallGuard guard(c);
    /* [GC:] Borrowed temporary graph nodes stay protected until the dictionary adopts them. */
    FVM_Object *definition = 0, *paths = 0, *frames = 0;
    /* [GC:] Allocate the definition that will own this character resource. */
    FVM_Result r = fvm_object_create(c, &definition);
    if (r == FVM_OK)
        /* [GC:] Allocate its managed collection of vector paints. */
        r = fvm_object_create(c, &paths);
    if (r == FVM_OK)
        /* [GC:] Allocate the static character's empty managed timeline. */
        r = fvm_object_create(c, &frames);
    if (r != FVM_OK)
        return r;
    definition->properties["id"] = number(shape.id);
    definition->properties["kind"] = number(2);
    definition->properties["total"] = number(1);
    definition->properties["xMin"] = number(shape.bounds.x_min);
    definition->properties["xMax"] = number(shape.bounds.x_max);
    definition->properties["yMin"] = number(shape.bounds.y_min);
    definition->properties["yMax"] = number(shape.bounds.y_max);
    /* [GC:] Retain an empty timeline and the complete vector collection in the definition. */
    definition->properties["frames"] = object(frames);
    /* [GC:] Solid geometry is an ordinary managed graph rather than a backend resource pointer. */
    definition->properties["paths"] = object(paths);
    for (size_t i = 0; i < shape.paints.size(); ++i)
    {
        const FVM_SWFShapePaint &paint = shape.paints[i];
        /* [GC:] Temporary paint and command-table edges are protected by the native barrier. */
        FVM_Object *part = 0, *commands = 0;
        /* [GC:] Allocate this independently owned paint descriptor. */
        r = fvm_object_create(c, &part);
        if (r == FVM_OK)
            /* [GC:] Allocate the paint's command table before linking its records. */
            r = fvm_object_create(c, &commands);
        if (r != FVM_OK)
            return r;
        part->properties["fill"] = number(paint.stroke ? 0 : 1);
        part->properties["stroke"] = number(paint.stroke ? 1 : 0);
        part->properties["rgb"] = part->properties["lineRgb"] = number(paint.rgb);
        part->properties["alpha"] = part->properties["lineAlpha"] = number(paint.alpha);
        part->properties["width"] = number(paint.width);
        /* Keep round defaults absent so old graphs and public drawing records retain their shape. */
        if (paint.stroke_cap)
            part->properties["lineCap"] = number(paint.stroke_cap);
        if (paint.stroke_join)
            part->properties["lineJoin"] = number(paint.stroke_join);
        part->properties["even_odd"] = number(1);
        if (paint.gradient_type)
        {
            r = gradient(c, paint, part);
            if (r != FVM_OK)
                return r;
        }
        if (paint.bitmap_type)
        {
            r = bitmap(c, paint, part, bitmap_definitions);
            if (r != FVM_OK)
                return r;
        }
        /* [GC:] Each paint owns its commands, and the collection owns every paint. */
        part->properties["commands"] = object(commands);
        /* [GC:] Publish the new paint before creating any of its command records. */
        paths->indexed[(unsigned long)i] = object(part);
        for (size_t j = 0; j < paint.commands.size(); ++j)
        {
            /* [GC:] Hold the new record until the command table retains it. */
            FVM_Object *record = 0;
            /* [GC:] Allocate a managed vector command with no external handle. */
            r = fvm_object_create(c, &record);
            if (r != FVM_OK)
                return r;
            const FVM_PathCommand &command = paint.commands[j];
            record->properties["verb"] = number(command.verb);
            record->properties["x0"] = number(command.x[0]);
            record->properties["y0"] = number(command.y[0]);
            if (command.verb == FVM_PATH_QUADRATIC)
            {
                record->properties["x1"] = number(command.x[1]);
                record->properties["y1"] = number(command.y[1]);
            }
            /* [GC:] Keep this command reachable from its paint before constructing the next. */
            commands->indexed[(unsigned long)j] = object(record);
        }
    }
    /* [GC:] Transfer the complete graph edge to the caller's dictionary construction. */
    *out = definition;
    return FVM_OK;
}
