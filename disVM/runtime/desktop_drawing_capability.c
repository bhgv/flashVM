/* Generic owner-local graphics property adapter for the bounded AS display profile. */
#include "desktop_chrome.h"
#include "desktop_round_primitive.h"
#include "flashvm/value.h"
#include "flashvm/gc.h"
#include "flashvm/avm1.h"
#include <math.h>
#include <string.h>


/* Read a finite scalar from the rooted capability. No UI policy/default is supplied. */
static int scalar(FVM_Context *owner, FVM_Object *receiver, const char *name, double *value)
{
    FVM_Value field;
    if (fvm_object_get(owner, receiver, name, &field) != FVM_OK || field.type != FVM_VALUE_NUMBER ||
        !isfinite(field.number) || fabs(field.number) > 16777215)
    {
        return 0;
    }
    *value = field.number;
    return 1;
}


/* Execute one generic primitive requested by AS. The receiver roots its target;
 * pin that borrowed MovieClip across drawing allocations. No document or worker
 * pointer is retained. Errors return through the ordinary AVM1 accessor boundary. */
static FVM_Result command(FVM_Context *owner,
                          FVM_Object *receiver,
                          unsigned int count,
                          const FVM_Value *arguments,
                          FVM_Value *result,
                          void *userdata)
{
    const char *name = (const char *)userdata;
    FVM_Value target;
    FVM_Value values[2];
    FVM_Value ignored;
    FVM_Result status = FVM_INVALID_ARGUMENT;
    double x;
    double y;
    double wide;
    double high;
    double color;
    double alpha;
    double border;
    (void)arguments;
    fvm_value_undefined(result);
    if (count != 1 || fvm_object_get(owner, receiver, "target", &target) != FVM_OK || target.type != FVM_VALUE_OBJECT ||
        !target.object || fvm_gc_pin(owner, target.object) != FVM_OK)
    {
        return status;
    }
    if (!strcmp(name, "clear") || !strcmp(name, "end"))
    {
        status = fvm_object_call(owner, target.object, !strcmp(name, "clear") ? "clear" : "endFill", 0, NULL, &ignored);
    }
    else if (!strcmp(name, "move") || !strcmp(name, "line"))
    {
        if (scalar(owner, receiver, "x", &x) && scalar(owner, receiver, "y", &y))
        {
            fvm_value_number(&values[0], x);
            fvm_value_number(&values[1], y);
            status =
                fvm_object_call(owner, target.object, !strcmp(name, "move") ? "moveTo" : "lineTo", 2, values, &ignored);
        }
    }
    else if (!strcmp(name, "begin"))
    {
        if (scalar(owner, receiver, "color", &color) && scalar(owner, receiver, "alpha", &alpha) && color >= 0 &&
            floor(color) == color && alpha >= 0 && alpha <= 100)
        {
            status = fvm_object_call(owner, target.object, "lineStyle", 0, NULL, &ignored);
            fvm_value_number(&values[0], color);
            fvm_value_number(&values[1], alpha);
            if (status == FVM_OK)
            {
                status = fvm_object_call(owner, target.object, "beginFill", 2, values, &ignored);
            }
        }
    }
    else if (scalar(owner, receiver, "w", &wide) && scalar(owner, receiver, "h", &high) &&
             scalar(owner, receiver, "color", &color) && wide >= 0 && high >= 0 && wide <= 1048576 && high <= 1048576 &&
             color >= 0 && floor(color) == color)
    {
        if (!strcmp(name, "box") && scalar(owner, receiver, "border", &border) && border >= 0 &&
            floor(border) == border)
        {
            status = dvm_chrome_box(owner, target.object, wide, high, (unsigned long)color, (unsigned long)border)
                         ? FVM_OK
                         : FVM_ERROR;
        }
        else if (!strcmp(name, "round"))
        {
            double radius;
            double stroke;
            double filled;
            if (scalar(owner, receiver, "x", &x) && scalar(owner, receiver, "y", &y) &&
                scalar(owner, receiver, "alpha", &alpha) && alpha >= 0 && alpha <= 100 &&
                scalar(owner, receiver, "radius", &radius) && radius >= 0 && radius <= 1048576 &&
                scalar(owner, receiver, "stroke", &stroke) && stroke >= 0 && stroke <= 32 &&
                scalar(owner, receiver, "filled", &filled) && (filled == 0 || filled == 1) &&
                scalar(owner, receiver, "border", &border) && border >= 0 && floor(border) == border)
            {
                status = dvm_chrome_round(owner, target.object, x, y, wide, high, radius,
                    (unsigned long)color, (unsigned long)border, stroke, alpha, (int)filled) ? FVM_OK : FVM_ERROR;
            }
        }
        else if (!strcmp(name, "rect") && scalar(owner, receiver, "x", &x) && scalar(owner, receiver, "y", &y) &&
                 scalar(owner, receiver, "alpha", &alpha) && alpha >= 0 && alpha <= 100)
        {
            double xs[5] = {x, x + wide, x + wide, x, x};
            double ys[5] = {y, y, y + high, y + high, y};
            status =
                dvm_chrome_patch(owner, target.object, xs, ys, 5, (unsigned long)color, alpha) ? FVM_OK : FVM_ERROR;
        }
    }
    fvm_gc_unpin(owner, target.object);
    return status;
}


/* Bind/reuse a same-context generic drawing capability. All target retention is
 * ordinary managed object state, never a Dis payload. No layout/style is chosen. */
int dvm_chrome_drawing_bind(FVM_Context *owner, FVM_Object *clip)
{
    FVM_Value value;
    FVM_Object *drawing;
    unsigned int index;
    static const char *names[] = {"clear", "box", "round", "rect", "begin", "move", "line", "end"};
    if (fvm_get_global(owner, "ideDraw", &value) != FVM_OK)
    {
        return 0;
    }
    if (value.type != FVM_VALUE_OBJECT)
    {
        if (fvm_object_create(owner, &drawing) != FVM_OK || fvm_gc_pin(owner, drawing) != FVM_OK)
        {
            return 0;
        }
        for (index = 0; index < sizeof(names) / sizeof(names[0]); index++)
        {
            if (fvm_object_define_accessor_rooted(
                    owner, drawing, names[index], NULL, command, (void *)names[index], NULL) != FVM_OK)
            {
                fvm_gc_unpin(owner, drawing);
                return 0;
            }
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = drawing;
        if (fvm_set_global(owner, "ideDraw", &value) != FVM_OK)
        {
            fvm_gc_unpin(owner, drawing);
            return 0;
        }
        fvm_gc_unpin(owner, drawing);
    }
    drawing = value.object;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = clip;
    return fvm_object_set(owner, drawing, "target", &value) == FVM_OK;
}
