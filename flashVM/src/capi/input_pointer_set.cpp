/* FlashVM: explicit host pointer publication independent from input event delivery. */
#include "../core/runtime.hpp"
#include <cfloat>


/* Commit both finite world coordinates together without allocating or invoking script. */
extern "C" FVM_Result fvm_input_set_pointer(FVM_Context *c, double x, double y)
{
    if (!c || c->destroying || c->gc_releasing || !(x >= -DBL_MAX && x <= DBL_MAX && y >= -DBL_MAX && y <= DBL_MAX))
    {
        return FVM_INVALID_ARGUMENT;
    }
    c->pointer_world_x = x;
    c->pointer_world_y = y;
    return FVM_OK;
}
