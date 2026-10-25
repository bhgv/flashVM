/* FlashVM: explicit binary32 physical world matrix query shared by C/JIT and loaded point code. */
#include "support.hpp"
#include "clip_world_matrix_constants.h"
#include "../../core/matrix_float.hpp"
#include "../../core/scene_numeric.hpp"
#include "../../standard/movieclip_storage.hpp"
#include "flashvm/movieclip.h"


/* Read local snapshots in physical ancestry order and compose with per-level twip rounding. */
extern "C" FVM_Result fvm_exec_clip_world_matrix(FVM_Context *c, FVM_Object *clip, FVM_TwipMatrix *out)
{
    if (!c || c->destroying || c->gc_releasing || !clip || clip->owner != c || !out ||
        clip->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(clip))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!fvm_matrix_detail::supported())
    {
        return FVM_UNSUPPORTED;
    }
    try
    {


        /* [GC:] The root retains the clip; the barrier protects all old/mutated ancestor graphs. */
        FVM_ExecutionRoots roots(c, FVM_CLIP_WORLD_MATRIX_ROOT_COUNT);
        roots.values[FVM_CLIP_WORLD_MATRIX_RECEIVER_ROOT] = fvm_movieclip_storage::object(clip);


        FVM_NativeCallGuard guard(c);
        FVM_TwipMatrix result = {1, 0, 0, 1, 0, 0};
        FVM_Object *current = clip;
        unsigned int levels = 0;
        while (current && levels < FVM_MOVIECLIP_MATRIX_MAX_ANCESTORS)
        {
            FVM_Affine source;
            FVM_Result status = fvm_movieclip_get_matrix(c, current, &source);
            if (status != FVM_OK)
            {
                return status;
            }
            FVM_TwipMatrix local;
            local.a = fvm_scene_detail::coefficient(source.xx);
            local.b = fvm_scene_detail::coefficient(source.yx);
            local.c = fvm_scene_detail::coefficient(source.xy);
            local.d = fvm_scene_detail::coefficient(source.yy);
            local.tx = fvm_scene_detail::input_twips(source.x0);
            local.ty = fvm_scene_detail::input_twips(source.y0);
            if (!levels)
            {
                result = local;
            }
            else
            {
                status = fvm_matrix_multiply_twips(&local, &result, &result);
                if (status != FVM_OK)
                {
                    return status;
                }
            }
            ++levels;
            current = fvm_movieclip_storage::parent(current);
        }
        if (current)
        {
            return FVM_INVALID_ARGUMENT;
        }
        /* Keep the final scene-local identity composition, including binary32 exceptional
         * coefficient behavior; scroll-rectangle transforms are not modeled here. */
        const FVM_TwipMatrix identity = {1, 0, 0, 1, 0, 0};
        FVM_Result status = fvm_matrix_multiply_twips(&result, &identity, &result);
        if (status != FVM_OK)
        {
            return status;
        }
        *out = result;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
