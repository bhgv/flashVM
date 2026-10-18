/* FlashVM: unlocked root-relative coordinate conversion for native point hit testing. */
#include "movieclip_state.hpp"
#include "movieclip_constants.h"
#include "flashvm/movieclip.h"
/* Find the original AVM1 root independently of locks and publish both transformed coordinates. */
FVM_Result fvm_movieclip_hit_world_point(FVM_Context *c, FVM_Object *clip, double &x, double &y)
{
    if (!c || !clip || clip->owner != c)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *root = clip;
    unsigned int visited = 0;
    for (;;)
    {
        if (++visited > FVM_MOVIECLIP_HIT_ROOT_MAX_ANCESTORS)
            return FVM_ERROR;
        FVM_Object *parent = 0;
        FVM_Result status = fvm_movieclip_avm1_parent(c, root, &parent);
        if (status != FVM_OK)
            return status;
        if (!parent)
            break;
        root = parent;
    }
    /* [GC:] The public matrix query supplies its native barrier around property conversion. */
    FVM_Affine matrix;
    FVM_Result status = fvm_movieclip_get_matrix(c, root, &matrix);
    if (status != FVM_OK)
        return status;
    double world_x = matrix.xx * x + matrix.xy * y + matrix.x0;
    double world_y = matrix.yx * x + matrix.yy * y + matrix.y0;
    x = world_x;
    y = world_y;
    return FVM_OK;
}
