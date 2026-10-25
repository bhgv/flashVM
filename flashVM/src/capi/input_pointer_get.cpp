/* FlashVM: allocation-free snapshot of context-owned world pointer coordinates. */
#include "../core/runtime.hpp"


/* Publish the last accepted coordinate pair only after checking both output addresses. */
extern "C" FVM_Result fvm_input_get_pointer(FVM_Context *c, double *x, double *y)
{
    if (!c || c->destroying || c->gc_releasing || !x || !y || x == y)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *x = c->pointer_world_x;
    *y = c->pointer_world_y;
    return FVM_OK;
}
