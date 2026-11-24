/* Copyright (c) 2026 bhgv
 * EN: Generic world-space picking for application-owned display lists.
 * RU: Общие запросы попадания в приложениях со своим списком отображения.
 */

/* EN: Read a borrowed world-space point; require finite coordinates.
 * RU: Получить мировую точку с конечными координатами. */
FVM_Result pick_point(FVM_Context *c, unsigned count, const FVM_Value *values, double &x, double &y)
{
    FVM_Result result = count == 2 ? args(c, count, values, 0, 0, x) : FVM_INVALID_ARGUMENT;
    if (result == FVM_OK)
    {
        result = args(c, count, values, 1, 0, y);
    }
    return result;
}


/* EN: Return the public depth of the frontmost visible MovieClip child whose bounds contain the point.
 * RU: Вернуть глубину переднего видимого дочернего MovieClip, содержащего мировую точку. */
FVM_Result pick_child_depth(FVM_Context *c, FVM_Object *clip, unsigned count, const FVM_Value *values,
    FVM_Value *out, void *)
{
    FVM_Object *data = 0, *children = 0;
    double x, y;
    FVM_Result result = state(clip, data);
    if (result == FVM_OK)
    {
        result = private_object(data, "children", children);
    }
    if (result == FVM_OK)
    {
        result = pick_point(c, count, values, x, y);
    }
    if (result != FVM_OK)
    {
        return result;
    }
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    /* [GC:] Borrow the same stable order as the existing bounds traversal.
     * Использовать стабильный порядок обхода, как при вычислении габаритов. */
    Slots snapshot = children->indexed;
    for (Slots::iterator i = snapshot.begin(); i != snapshot.end(); ++i)
    {
        FVM_Object *child = i->second.type == FVM_VALUE_OBJECT ? i->second.object : 0;
        if (!child || static_art(child))
        {
            continue;
        }
        double visible;
        result = numeric(c, child, "_visible", visible);
        if (result != FVM_OK)
        {
            return result;
        }
        if (!visible)
        {
            continue;
        }
        bool any = false;
        FVM_PathBounds extent;
        result = bounds(c, child, identity(), false, extent, any, 0);
        if (result != FVM_OK)
        {
            return result;
        }
        if (any && x >= extent.x_min && x < extent.x_max && y >= extent.y_min && y < extent.y_max)
        {
            long depth;
            result = state(child, data);
            if (result == FVM_OK)
            {
                result = depth_field(data, depth);
            }
            if (result != FVM_OK)
            {
                return result;
            }
            *out = number(depth);
        }
    }
    return FVM_OK;
}


/* EN: Propose a scalar insertion point in the TextField at a supplied public depth.
 * RU: Предложить позицию вставки в TextField на заданной публичной глубине. */
FVM_Result pick_text_scalar(FVM_Context *c, FVM_Object *clip, unsigned count, const FVM_Value *values,
    FVM_Value *out, void *)
{
    if (count != 3)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_NativeCallGuard guard(c);
    FVM_Value field_value;
    FVM_Result result = at_depth(c, clip, 1, values, &field_value, 0);
    if (result != FVM_OK)
    {
        return result;
    }
    long scalar = -1;
    if (field_value.type == FVM_VALUE_OBJECT && field_value.object->resource_kind == FVM_RESOURCE_TEXT_FIELD)
    {
        double x, y, field_x, field_y;
        result = pick_point(c, 2, values + 1, x, y);
        if (result == FVM_OK)
        {
            result = numeric(c, field_value.object, "_x", field_x);
        }
        if (result == FVM_OK)
        {
            result = numeric(c, field_value.object, "_y", field_y);
        }
        FVM_Affine local;
        if (result == FVM_OK)
        {
            result = matrix(c, clip, local);
        }
        if (result != FVM_OK)
        {
            return result;
        }
        if (invert(local))
        {
            double local_x = x * local.xx + y * local.xy + local.x0;
            double local_y = x * local.yx + y * local.yy + local.y0;
            result = fvm_textfield_scalar_at(c, field_value.object, local_x - field_x, local_y - field_y, scalar);
        }
    }
    *out = number(scalar);
    return result;
}
