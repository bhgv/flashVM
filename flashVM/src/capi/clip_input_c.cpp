/* FlashVM: bounded retained input broadcasts over a prepared native MovieClip scene. */
#include "flashvm/clip_actions.h"
#include "../core/native_call_guard.hpp"
#include "../core/optional_method.hpp"
#include "../standard/stage_update.hpp"
#include "../core/execution_work_scope.hpp"
#include "../core/depth_order.hpp"
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
    if (result != FVM_OK)
        return result;
    if (!children || children->owner != c)
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
    result = fvm_depth_order::sort(order, work);
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
/* Map a validated global input bit to its optional zero-argument MovieClip method. */
const char *handler(unsigned long event)
{
    switch (event)
    {
    case FVM_CLIP_EVENT_MOUSE_MOVE: return "onMouseMove";
    case FVM_CLIP_EVENT_MOUSE_DOWN: return "onMouseDown";
    case FVM_CLIP_EVENT_MOUSE_UP: return "onMouseUp";
    case FVM_CLIP_EVENT_KEY_DOWN: return "onKeyDown";
    case FVM_CLIP_EVENT_KEY_UP: return "onKeyUp";
    }
    return 0;
}
/* Share topology, work and teardown guards across retained, named-method and raw-key delivery. */
FVM_Result dispatch(FVM_Context *c, FVM_Object *root, unsigned long event, bool methods,
                    bool key_press = false, unsigned int key = 0)
{
    if (!c || c->destroying || c->gc_collecting || c->gc_releasing || c->movieclip_sound_busy ||
        c->clip_input_dispatching || !event || (event & (event - 1)) ||
        (event & ~(key_press ? FVM_CLIP_EVENT_KEY_PRESS : FVM_CLIP_INPUT_EVENT_MASK)))
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
            if (version && version < (key_press ? FVM_CLIP_INPUT_KEY_PRESS_MIN_VERSION :
                                                  FVM_SWF_CLIP_ACTION_MIN_VERSION))
                continue;
            /* Host KeyPress is an input scope; the older selected-record API stays unchanged. */
            FVM_StageUpdateEventScope key_redraw(c, key_press);
            result = fvm_movieclip_dispatch_clip_actions(c, batch[i].clip, event, key);
            if (result != FVM_OK)
                return result;
            if (methods && (!version || version >= FVM_CLIP_INPUT_METHOD_MIN_VERSION))
            {
                /* A retained record may have removed or moved this receiver's branch. */
                result = eligible(c, batch, i, deliver, work);
                if (result != FVM_OK)
                    return result;
                if (!deliver)
                    continue;
                result = work.step();
                if (result != FVM_OK)
                    return result;
                /* Resolve once at this receiver's turn; getters share event eligibility. */
                FVM_StageUpdateEventScope redraw(c, true);
                result = fvm_object_call_optional(c, batch[i].clip, handler(event));
                if (result != FVM_OK)
                    return result;
            }
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
} // namespace
/* Preserve the v0.626 retained-bytecode-only API and its existing work accounting. */
extern "C" FVM_Result fvm_movieclip_dispatch_clip_input(FVM_Context *c, FVM_Object *root,
                                                        unsigned long event)
{
    return dispatch(c, root, event, false);
}
/* Deliver retained records followed by the receiver's optional MovieClip input method. */
extern "C" FVM_Result fvm_movieclip_dispatch_input(FVM_Context *c, FVM_Object *root,
                                                   unsigned long event)
{
    return dispatch(c, root, event, true);
}

/* Retain exact wire selector matching without translating or modifying host keyboard state. */
extern "C" FVM_Result fvm_movieclip_dispatch_key_press(FVM_Context *c, FVM_Object *root,
                                                       unsigned int key)
{
    if (key > FVM_CLIP_KEY_MAX)
        return FVM_INVALID_ARGUMENT;
    return dispatch(c, root, FVM_CLIP_EVENT_KEY_PRESS, false, true, key);
}
