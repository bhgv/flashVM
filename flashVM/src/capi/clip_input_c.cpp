/* FlashVM: bounded retained input broadcasts over a prepared native MovieClip scene. */
#include "flashvm/clip_actions.h"
#include "../core/native_call_guard.hpp"
#include "../core/execution_work_scope.hpp"
#include "../standard/movieclip_scene.hpp"
#include "../standard/movieclip_state.hpp"
#include "../standard/movieclip_storage.hpp"
#include "../standard/clip_input_constants.h"
#include "../swf/clip_action_constants.h"
#include <new>
namespace
{
/* One captured node and the index of its captured parent; root refers to itself. */
struct Recipient
{
    FVM_Object *clip; /* [GC:] Borrowed native clip retained by the enclosing collector barrier. */
    size_t parent; /* Earlier recipient index, or zero for the selected root. */
};
/* Restore only the guard introduced by this public delivery on every exit path. */
class DeliveryScope
{
    FVM_Context *context_; /* Owner-thread context whose guard this scope owns. */
    DeliveryScope(const DeliveryScope &); /* A live guard cannot be copied. */
    DeliveryScope &operator=(const DeliveryScope &); /* A live guard cannot be reassigned. */
  public:
    /* Enter before native topology migration, allocation or callbacks can fail. */
    explicit DeliveryScope(FVM_Context *c) : context_(c) { c->clip_input_dispatching = true; }
    /* Restore ordinary entry after success, VM errors or native allocation exceptions. */
    ~DeliveryScope() { context_->clip_input_dispatching = false; }
};
/* Validate raw native state and distinguish static artwork without invoking script properties. */
FVM_Result state(FVM_Context *c, FVM_Object *clip, bool &art)
{
    FVM_Object *data = 0;
    if (!clip || clip->owner != c || fvm_movieclip_storage::state(clip, data) != FVM_OK || data->owner != c)
        return FVM_INVALID_ARGUMENT;
    art = false;
    fvm_movieclip_storage::Properties::const_iterator marker = data->properties.find("static_art");
    if (marker != data->properties.end())
    {
        if (marker->second.type != FVM_VALUE_NUMBER ||
            (marker->second.number != 0 && marker->second.number != 1))
            return FVM_INVALID_ARGUMENT;
        art = marker->second.number == 1;
    }
    return FVM_OK;
}
/* Restore a max-heap, charging each comparison block so sorting shares the caller's budget. */
FVM_Result sift(fvm_detail::vector<unsigned long> &order, size_t root, size_t count,
                 FVM_ExecutionWorkScope &work)
{
    while (root < count / 2)
    {
        FVM_Result result = work.step();
        if (result != FVM_OK)
            return result;
        size_t child = root * 2 + 1;
        if (child + 1 < count && order[child] < order[child + 1])
            ++child;
        if (order[root] >= order[child])
            break;
        unsigned long value = order[root];
        order[root] = order[child];
        order[child] = value;
        root = child;
    }
    return FVM_OK;
}
/* Sort native depth keys without STL or quadratic insertion sorting. */
FVM_Result sort(fvm_detail::vector<unsigned long> &order, FVM_ExecutionWorkScope &work)
{
    for (size_t i = order.size() / 2; i; --i)
    {
        FVM_Result result = sift(order, i - 1, order.size(), work);
        if (result != FVM_OK)
            return result;
    }
    for (size_t count = order.size(); count > 1; --count)
    {
        FVM_Result result = work.step();
        if (result != FVM_OK)
            return result;
        unsigned long value = order[0];
        order[0] = order[count - 1];
        order[count - 1] = value;
        result = sift(order, 0, count - 1, work);
        if (result != FVM_OK)
            return result;
    }
    return FVM_OK;
}
/* Capture the complete selected topology before the first event callback; static art has no handler. */
FVM_Result capture(FVM_Context *c, FVM_Object *clip, size_t parent, unsigned int depth,
                    fvm_detail::vector<Recipient> &batch, fvm_detail::map<size_t, bool> &seen,
                    FVM_ExecutionWorkScope &work, unsigned long &visited)
{
    FVM_Result result = work.step();
    if (result != FVM_OK)
        return result;
    if (depth > FVM_CLIP_INPUT_MAX_DEPTH || visited >= FVM_CLIP_INPUT_MAX_NODES)
        return FVM_INVALID_ARGUMENT;
    ++visited;
    bool art = false;
    result = state(c, clip, art);
    if (result != FVM_OK || art)
        return result;
    if (depth > FVM_CLIP_INPUT_MAX_DEPTH || batch.size() >= FVM_CLIP_INPUT_MAX_RECIPIENTS ||
        seen.find(reinterpret_cast<size_t>(clip)) != seen.end())
        return FVM_INVALID_ARGUMENT;
    seen[reinterpret_cast<size_t>(clip)] = true;
    Recipient value;
    value.clip = clip;
    value.parent = parent;
    const size_t self = batch.size();
    batch.push_back(value);
    FVM_Object *children = 0;
    result = fvm_movieclip_scene_children(clip, &children);
    if (result != FVM_OK || !children || children->owner != c)
        return FVM_INVALID_ARGUMENT;
    if (children->indexed.size() > FVM_CLIP_INPUT_MAX_CHILDREN)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::vector<unsigned long> order;
    for (fvm_movieclip_storage::Slots::const_iterator i = children->indexed.begin();
         i != children->indexed.end(); ++i)
    {
        result = work.step();
        if (result != FVM_OK)
            return result;
        order.push_back(i->first);
    }
    result = sort(order, work);
    if (result != FVM_OK)
        return result;
    for (size_t i = 0; i < order.size(); ++i)
    {
        FVM_Value child = children->indexed.find(order[i])->second;
        if (child.type != FVM_VALUE_OBJECT || !child.object || child.object->owner != c)
            return FVM_INVALID_ARGUMENT;
        bool child_art = false;
        result = state(c, child.object, child_art);
        if (result != FVM_OK)
            return result;
        if (fvm_movieclip_storage::parent(child.object) != clip)
            continue;
        result = capture(c, child.object, self, depth + 1, batch, seen, work, visited);
        if (result != FVM_OK)
            return result;
    }
    return FVM_OK;
}
/* Reject a moved branch at any captured ancestor, including movement after that ancestor's callback. */
FVM_Result eligible(FVM_Context *c, const fvm_detail::vector<Recipient> &batch, size_t index,
                     bool &deliver, FVM_ExecutionWorkScope &work)
{
    deliver = false;
    for (size_t current = index; current; current = batch[current].parent)
    {
        FVM_Result result = work.step();
        if (result != FVM_OK)
            return result;
        if (fvm_movieclip_storage::parent(batch[current].clip) != batch[batch[current].parent].clip)
            return FVM_OK;
    }
    bool removed = false;
    FVM_Result result = fvm_movieclip_scene_removed(c, batch[index].clip, &removed);
    if (result == FVM_OK)
        deliver = !removed;
    return result;
}
} // namespace
/* Deliver only retained global clip input records; callers control native state publication. */
extern "C" FVM_Result fvm_movieclip_dispatch_clip_input(FVM_Context *c, FVM_Object *root,
                                                        unsigned long event)
{
    if (!c || c->destroying || c->gc_collecting || c->gc_releasing || c->movieclip_sound_busy ||
        c->clip_input_dispatching || !event || (event & (event - 1)) ||
        (event & ~FVM_CLIP_INPUT_EVENT_MASK))
        return FVM_INVALID_ARGUMENT;
    try
    {
        bool art = false;
        FVM_Result result = state(c, root, art);
        if (result != FVM_OK || art)
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Capture/delivery hold raw topology and code; explicit collection remains blocked. */
        FVM_NativeCallGuard barrier(c);
        DeliveryScope delivery(c);
        FVM_ExecutionWorkScope work(c);
        fvm_detail::vector<Recipient> batch;
        fvm_detail::map<size_t, bool> seen;
        unsigned long visited = 0;
        result = capture(c, root, 0, 0, batch, seen, work, visited);
        if (result != FVM_OK)
            return result;
        for (size_t i = 0; i < batch.size(); ++i)
        {
            result = work.step();
            if (result != FVM_OK)
                return result;
            bool deliver = false;
            result = eligible(c, batch, i, deliver, work);
            if (result != FVM_OK)
                return result;
            if (!deliver)
                continue;
            unsigned int version = 0;
            result = fvm_movieclip_movie_version(c, batch[i].clip, &version);
            if (result != FVM_OK)
                return result;
            if (version && version < FVM_SWF_CLIP_ACTION_MIN_VERSION)
                continue;
            result = fvm_movieclip_dispatch_clip_actions(c, batch[i].clip, event, 0);
            if (result != FVM_OK)
                return result;
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
