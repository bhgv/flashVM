/* FlashVM: atomic native local matrix/color copy shared by C, generated code and script setters. */
#include "support.hpp"
#include "clip_copy_transform_constants.h"
#include "../../standard/transform_scene.hpp"
#include "../../standard/transform_constants.h"
#include "../../standard/movieclip_storage.hpp"
#include "flashvm/movieclip.h"

namespace
{
/* [GC:] Preserve both destination maps until matrix and color have successfully committed.
 * The caller holds a native barrier; no callbacks execute between publication and rollback. */
class CopyTransaction
{
    FVM_Object *clip_;                             /* Destination whose public numeric properties may be replaced. */
    FVM_Object *state_;                            /* [GC:] Destination private display state, retained by clip. */
    fvm_movieclip_storage::Properties properties_; /* [GC:] Original public managed edges. */
    fvm_movieclip_storage::Properties private_;    /* [GC:] Original private managed edges. */
    bool committed_;                               /* Successful publication suppresses nonallocating rollback. */
    /* [GC:] A transaction owns one rollback and cannot be copied. */
    CopyTransaction(const CopyTransaction &);
    /* [GC:] A transaction cannot transfer ownership through assignment. */
    CopyTransaction &operator=(const CopyTransaction &);

  public:


    /* [GC:] Finish both allocating snapshots before permitting destination mutation. */
    CopyTransaction(FVM_Object *clip, FVM_Object *state)
        : clip_(clip), state_(state), properties_(clip->properties), private_(state->properties), committed_(false)
    {
    }


    /* Mark both publications as complete without allocating or invoking script. */
    void commit()
    {
        committed_ = true;
    }


    /* [GC:] Restore matching public/private graphs on status errors and allocation exceptions. */
    ~CopyTransaction()
    {
        if (!committed_)
        {
            clip_->properties.swap(properties_);
            state_->properties.swap(private_);
        }
    }
};
} // namespace


/* Snapshot source before opening the destination transaction; callback side effects are retained.
 */
extern "C" FVM_Result fvm_exec_clip_copy_transform(FVM_Context *c, FVM_Object *destination, FVM_Object *source)
{
    if (!c || c->destroying || c->gc_releasing || !destination || !source || destination->owner != c ||
        source->owner != c || destination->resource_kind != FVM_RESOURCE_MOVIECLIP ||
        source->resource_kind != FVM_RESOURCE_MOVIECLIP || fvm_movieclip_is_class_prototype(destination) ||
        fvm_movieclip_is_class_prototype(source))
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        /* [GC:] Explicit operands remain roots while source geometry can execute callbacks. */
        FVM_ExecutionRoots roots(c, FVM_CLIP_COPY_ROOT_COUNT);
        roots.values[FVM_CLIP_COPY_DESTINATION_ROOT] = fvm_movieclip_storage::object(destination);
        roots.values[FVM_CLIP_COPY_SOURCE_ROOT] = fvm_movieclip_storage::object(source);


        FVM_NativeCallGuard guard(c);
        FVM_Affine matrix;
        double color[FVM_TRANSFORM_COLOR_COMPONENT_COUNT];
        FVM_Result status = fvm_movieclip_get_matrix(c, source, &matrix);
        if (status == FVM_OK)
        {
            status = fvm_transform_scene_color(c, source, false, color);
        }
        FVM_Object *state = 0;
        if (status == FVM_OK)
        {
            status = fvm_movieclip_storage::state(destination, state);
        }
        if (status != FVM_OK)
        {
            return status;
        }


        CopyTransaction transaction(destination, state);
        status = fvm_movieclip_set_matrix(c, destination, &matrix);
        if (status == FVM_OK)
        {
            status = fvm_transform_scene_set_color(c, destination, color);
        }
        if (status == FVM_OK)
        {
            transaction.commit();
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
