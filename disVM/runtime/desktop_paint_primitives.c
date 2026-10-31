#include "desktop_chrome.h"
#include <math.h>


/* Build one solid drawing using actual MovieClip drawing methods. The copied
 * palette/extent are values; output is FlashVM-managed graphics, not a C mock. */
int dvm_chrome_box(
    FVM_Context *owner, FVM_Object *clip, double wide, double high, unsigned long rgb, unsigned long border)
{
    FVM_Value args[3];
    FVM_Value ignored;
    int index;
    double radius = fmin(7, fmin(wide, high) / 4);
    double x[25];
    double y[25];
    /* Six segments per corner keep the outline independent of font/raster jobs. */
    for (index = 0; index < 25; ++index)
    {
        int corner = (index % 24) / 6;
        double angle = ((index % 24) * 15.0 + 180.0) * 3.141592653589793 / 180.0;
        double cx = corner == 0 || corner == 3 ? radius : wide - radius;
        double cy = corner < 2 ? radius : high - radius;
        x[index] = cx + radius * cos(angle);
        y[index] = cy + radius * sin(angle);
    }
    if (fvm_object_call(owner, clip, "clear", 0, NULL, &ignored) != FVM_OK)
    {
        return 0;
    }
    {
        fvm_value_number(&args[0], border ? 2 : 0);
        fvm_value_number(&args[1], border);
        fvm_value_number(&args[2], 100);
        if (fvm_object_call(owner, clip, "lineStyle", 3, args, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    fvm_value_number(&args[0], rgb);
    fvm_value_number(&args[1], 100);
    if (fvm_object_call(owner, clip, "beginFill", 2, args, &ignored) != FVM_OK)
    {
        return 0;
    }
    for (index = 0; index < 25; ++index)
    {
        fvm_value_number(&args[0], x[index]);
        fvm_value_number(&args[1], y[index]);
        if (fvm_object_call(owner, clip, index ? "lineTo" : "moveTo", 2, args, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    return fvm_object_call(owner, clip, "endFill", 0, NULL, &ignored) == FVM_OK;
}


/* Append a copied filled polygon to owner graphics. Coordinates/colors are
 * ordinary values; raster workers later receive detached paths. Never clear
 * the clip or access a Dis object. Return zero on a drawing API failure. */
int dvm_chrome_patch(FVM_Context *owner,
                     FVM_Object *clip,
                     const double *x,
                     const double *y,
                     unsigned int count,
                     unsigned long rgb,
                     double alpha)
{
    FVM_Value values[2];
    FVM_Value ignored;
    unsigned int index;
    if (fvm_object_call(owner, clip, "lineStyle", 0, NULL, &ignored) != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(&values[0], rgb);
    fvm_value_number(&values[1], alpha);
    if (fvm_object_call(owner, clip, "beginFill", 2, values, &ignored) != FVM_OK)
    {
        return 0;
    }
    for (index = 0; index < count; index++)
    {
        fvm_value_number(&values[0], x[index]);
        fvm_value_number(&values[1], y[index]);
        if (fvm_object_call(owner, clip, index ? "lineTo" : "moveTo", 2, values, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    return fvm_object_call(owner, clip, "endFill", 0, NULL, &ignored) == FVM_OK;
}
