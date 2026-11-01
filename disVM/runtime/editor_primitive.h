/* Native two-endpoint geometric previews; never brush trajectories. */
#ifndef DVM_EDITOR_PRIMITIVE_H
#define DVM_EDITOR_PRIMITIVE_H
#include "flashvm/path.h"
#include <math.h>
#include <string.h>


/* Append a logical Line/Rectangle/Ellipse outline to a caller-owned empty path.
 * Packed points use1001*y+x. Return a path status; no VM/thread state is touched.
 * The ellipse is a temporary64-edge outline, never stored in the document. */
static FVM_Result dvm_primitive_path(FVM_Path *path, unsigned long tool, long a, long b, double scale)
{
    unsigned int i;
    unsigned int count = tool == 2 ? 5 : tool == 3 ? 65 : 2;
    double ax = a % 1001;
    double ay = a / 1001;
    double bx = b % 1001;
    double by = b / 1001;
    for (i = 0; i < count; ++i)
    {
        FVM_PathCommand command;
        double x = i ? bx : ax;
        double y = i ? by : ay;
        memset(&command, 0, sizeof(command));
        command.verb = i ? FVM_PATH_LINE : FVM_PATH_MOVE;
        if (tool == 2)
        {
            x = (i == 1 || i == 2) ? bx : ax;
            y = (i == 2 || i == 3) ? by : ay;
        }
        if (tool == 3)
        {
            double angle = i * 6.283185307179586 / 64;
            x = (ax + bx) / 2 + (bx - ax) / 2 * cos(angle);
            y = (ay + by) / 2 + (by - ay) / 2 * sin(angle);
        }
        command.x[0] = 40 + (20 + x) * scale;
        command.y[0] = 28 + (20 + y) * scale;
        {
            FVM_Result result = fvm_path_add(path, &command);
            if (result != FVM_OK)
            {
                return result;
            }
        }
    }
    return FVM_OK;
}
#endif
