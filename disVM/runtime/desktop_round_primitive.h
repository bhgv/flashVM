/* Generic additive rounded contour; appearance and layer order are AS policy. */
#ifndef DVM_DESKTOP_ROUND_PRIMITIVE_H
#define DVM_DESKTOP_ROUND_PRIMITIVE_H
#include "desktop_chrome.h"
#include <math.h>


/* Append one rounded rectangle without clearing the borrowed owner-local clip.
 * Coordinates/extents/radius/stroke are local pixels; alpha is percent. RGB
 * values and filled0/1 come from validated AS scalars. Return0 on drawing error.
 * No document access or blocking; all allocation remains on the FlashVM owner. */
static int dvm_chrome_round(FVM_Context *owner, FVM_Object *clip, double x, double y,
                            double wide, double high, double radius, unsigned long color,
                            unsigned long border, double stroke, double alpha, int filled)
{
    FVM_Value args[3];
    FVM_Value ignored;
    unsigned int i;
    radius = fmin(radius, fmin(wide, high) / 2);
    fvm_value_number(args, stroke);
    fvm_value_number(args + 1, border);
    fvm_value_number(args + 2, alpha);
    if (fvm_object_call(owner, clip, "lineStyle", stroke > 0 ? 3 : 0, args, &ignored) != FVM_OK)
    {
        return 0;
    }
    if (filled)
    {
        fvm_value_number(args, color);
        fvm_value_number(args + 1, alpha);
        if (fvm_object_call(owner, clip, "beginFill", 2, args, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    for (i = 0; i <= 32; ++i)
    {
        unsigned int corner = (i % 32) / 8;
        double angle = (i % 32) * 6.283185307179586 / 32 + 3.141592653589793;
        double cx = corner == 0 || corner == 3 ? radius : wide - radius;
        double cy = corner < 2 ? radius : high - radius;
        fvm_value_number(args, x + cx + radius * cos(angle));
        fvm_value_number(args + 1, y + cy + radius * sin(angle));
        if (fvm_object_call(owner, clip, i ? "lineTo" : "moveTo", 2, args, &ignored) != FVM_OK)
        {
            return 0;
        }
    }
    return fvm_object_call(owner, clip, "endFill", 0, NULL, &ignored) == FVM_OK;
}
#endif
