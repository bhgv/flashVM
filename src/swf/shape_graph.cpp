/* FlashVM: encode decoded solid SWF shapes as managed vector records without native handles. */
#include "shape_graph.hpp"
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
} // namespace

/* [GC:] Construct all paints and commands before returning a publishable immutable definition. */
FVM_Result fvm_swf_shape_build(FVM_Context *c, const FVM_SWFShape &shape, FVM_Object **out)
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
        part->properties["even_odd"] = number(1);
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
