/* FlashVM: incremental and seek reconstruction of managed SWF display lists. */
#include "movieclip_scene.hpp"
#include "movieclip_constants.h"
#include "scene_constants.h"
#include "../swf/format_constants.h"
#include "../core/execution_work_scope.hpp"
#include "../swf/clip_action_constants.h"
#include "../core/native_call_guard.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/stage.h"
#include <cmath>
#include <cstring>

namespace
{
typedef fvm_detail::map<fvm_detail::string, FVM_Value> Properties;
typedef fvm_detail::map<unsigned long, FVM_Value> Slots;
typedef fvm_detail::map<unsigned long, Properties> Display;

/* Read one private property without invoking script or inserting a missing field. */
FVM_Result field(const Properties &properties, const char *key, FVM_Value &out)
{
    Properties::const_iterator i = properties.find(key);
    if (i == properties.end())
        return FVM_INVALID_ARGUMENT;
    out = i->second;
    return FVM_OK;
}

/* Read a field on an existing private managed record. */
FVM_Result field(FVM_Object *o, const char *key, FVM_Value &out)
{
    return field(o->properties, key, out);
}

/* Read a bounded integer from a private control record. */
FVM_Result integer(const Properties &properties, const char *key, unsigned long limit, unsigned long &out)
{
    FVM_Value value;
    FVM_Result r = field(properties, key, value);
    if (r != FVM_OK || value.type != FVM_VALUE_NUMBER ||
        !(value.number >= 0 && value.number <= limit) || value.number != std::floor(value.number))
        return FVM_INVALID_ARGUMENT;
    out = (unsigned long)value.number;
    return FVM_OK;
}

/* Read bounded metadata from a private managed object. */
FVM_Result integer(FVM_Object *o, const char *key, unsigned long limit, unsigned long &out)
{
    return integer(o->properties, key, limit, out);
}

/* [GC:] Read a managed object edge, checking its context before following it. */
FVM_Result object(FVM_Context *c, FVM_Object *o, const char *key, FVM_Object *&out)
{
    FVM_Value value;
    FVM_Result r = field(o, key, value);
    if (r != FVM_OK || value.type != FVM_VALUE_OBJECT || !value.object || value.object->owner != c)
        return FVM_INVALID_ARGUMENT;
    out = value.object;
    return FVM_OK;
}

/* Construct a scalar value for composed control metadata. */
FVM_Value number(double value)
{
    FVM_Value result;
    fvm_value_number(&result, value);
    return result;
}

/* Find the current live instance at a raw SWF depth in an already normalized table. */
FVM_Object *child(FVM_Object *children, unsigned long depth)
{
    Slots::iterator i = children->indexed.find(depth);
    return i != children->indexed.end() && i->second.type == FVM_VALUE_OBJECT ? i->second.object
                                                                              : 0;
}

/* Project optional fields; absent ratio/name/matrix/color must not become updates. */
FVM_Result project(FVM_Context *c, const Properties &control, unsigned long flags, Properties &out)
{
    const char *base[] = {"depth", "token", "character"};
    for (unsigned int i = 0; i < 3; ++i)
    {
        FVM_Value value;
        FVM_Result r = field(control, base[i], value);
        if (r != FVM_OK)
            return r;
        out[base[i]] = value;
    }
    const char *keys[] = {"symbol", "matrix", "color", "ratio", "name", "clip_actions"};
    const unsigned long masks[] = {FVM_SCENE_PLACE_CHARACTER, FVM_SCENE_PLACE_MATRIX,
                                    FVM_SCENE_PLACE_COLOR, FVM_SCENE_PLACE_RATIO,
                                    FVM_SCENE_PLACE_NAME, FVM_SWF_PLACE_HAS_CLIP_ACTIONS};
    for (unsigned int i = 0; i < 6; ++i)
        if (flags & masks[i])
        {
            FVM_Value value;
            FVM_Result r = field(control, keys[i], value);
            if (r != FVM_OK || (i < 3 && (value.type != FVM_VALUE_OBJECT || !value.object ||
                                          value.object->owner != c)))
                return FVM_INVALID_ARGUMENT;
            out[keys[i]] = value;
        }
    return FVM_OK;
}

/* [GC:] Materialize a composed native property map only when an instance needs it. */
FVM_Result record(FVM_Context *c, const Properties &properties, FVM_Object **out)
{
    FVM_Result r = fvm_object_create(c, out);
    if (r == FVM_OK)
        (*out)->properties = properties;
    return r;
}

/* Select the first encountered opaque SWF color, preserving a host or earlier movie selection. */
FVM_Result background(FVM_Context *context, const Properties &control)
{
    unsigned long rgb = 0;
    FVM_Result status = integer(control, "background", FVM_SCENE_MAX_RGB, rgb);
    if (status != FVM_OK)
        return status;
    if (!context->stage_background_set)
    {
        FVM_RasterColor color;
        color.r = (unsigned char)(rgb >> FVM_SCENE_RED_SHIFT);
        color.g = (unsigned char)(rgb >> FVM_SCENE_GREEN_SHIFT);
        color.b = (unsigned char)rgb;
        color.a = FVM_SCENE_OPAQUE_ALPHA;
        return fvm_stage_set_background(context, color);
    }
    return FVM_OK;
}

/* Apply one live control; sprite replacement changes pose while preserving the live timeline. */
FVM_Result apply(FVM_Context *c, FVM_Object *parent, FVM_Object *children, const Properties &control)
{
    unsigned long op = 0, depth = 0, flags = 0;
    FVM_Result r = integer(control, "op", FVM_SWF_TAG_REMOVE_OBJECT2, op);
    if (r == FVM_OK && op == FVM_SWF_TAG_SET_BACKGROUND_COLOR)
        return background(c, control);
    if (r == FVM_OK)
        r = integer(control, "depth", FVM_MOVIECLIP_MAX_AUTHORED_DEPTH, depth);
    if (r != FVM_OK || !depth)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *old = child(children, depth);
    if (op == FVM_SWF_TAG_REMOVE_OBJECT || op == FVM_SWF_TAG_REMOVE_OBJECT2)
    {
        if (!old)
            return FVM_OK;
        if (op == FVM_SWF_TAG_REMOVE_OBJECT)
        {
            FVM_Object *data = 0, *symbol = 0;
            unsigned long expected = 0, actual = 0;
            r = integer(control, "character", FVM_SCENE_MAX_CHARACTER_ID, expected);
            if (r == FVM_OK)
                r = fvm_movieclip_scene_state(old, &data);
            if (r != FVM_OK)
                return r;
            if (data->properties.find("scene_symbol") == data->properties.end())
                return FVM_OK;
            r = object(c, data, "scene_symbol", symbol);
            if (r == FVM_OK)
                r = integer(symbol, "id", FVM_SCENE_MAX_CHARACTER_ID, actual);
            if (r != FVM_OK)
                return r;
            if (expected != actual)
                return FVM_OK;
        }
        return fvm_movieclip_scene_remove(c, old);
    }
    if (op != FVM_SWF_TAG_PLACE_OBJECT && op != FVM_SWF_TAG_PLACE_OBJECT2)
        return FVM_INVALID_ARGUMENT;
    r = integer(control, "flags", FVM_SWF_PLACE_SUPPORTED_FLAGS, flags);
    if (r != FVM_OK || !(flags & FVM_SCENE_PLACE_MODE_MASK) || (flags & ~FVM_SWF_PLACE_SUPPORTED_FLAGS))
        return FVM_INVALID_ARGUMENT;
    Properties properties;
    r = project(c, control, flags, properties);
    if (r != FVM_OK)
        return r;
    if (!(flags & FVM_SCENE_PLACE_CHARACTER))
    {
        if (!old)
            return FVM_OK;
        /* Instance names are assigned on placement, never changed by Modify/Replace. */
        properties.erase("name");
        FVM_Object *update = 0;
        r = record(c, properties, &update);
        return r == FVM_OK ? fvm_movieclip_scene_update(c, old, update) : r;
    }
    if (flags & FVM_SCENE_PLACE_MOVE)
    {
        /* Replace does not exchange a MovieClip's definition; static art is handled separately. */
        if (!old)
            return FVM_OK;
        /* Instance names are assigned on placement, never changed by Modify/Replace. */
        properties.erase("name");
        FVM_Object *update = 0;
        r = record(c, properties, &update);
        return r == FVM_OK ? fvm_movieclip_scene_replace(c, old, update) : r;
    }
    FVM_Object *placement = 0, *instance = 0;
    r = record(c, properties, &placement);
    return r == FVM_OK ? fvm_movieclip_scene_insert(c, parent, placement, &instance) : r;
}

/* Test the resource kind without adding properties to an immutable definition. */
bool shape_symbol(FVM_Object *symbol)
{
    FVM_Value kind;
    return symbol && field(symbol, "kind", kind) == FVM_OK && kind.type == FVM_VALUE_NUMBER &&
           kind.number == FVM_SCENE_SYMBOL_SHAPE;
}

/* Compose authored controls without running constructors or any skipped frame actions. */
FVM_Result accumulate(FVM_Context *c, const Properties &control, Display &display)
{
    unsigned long op = 0, depth = 0, flags = 0;
    FVM_Result r = integer(control, "op", FVM_SWF_TAG_REMOVE_OBJECT2, op);
    if (r == FVM_OK && op == FVM_SWF_TAG_SET_BACKGROUND_COLOR)
        return background(c, control);
    if (r == FVM_OK)
        r = integer(control, "depth", FVM_MOVIECLIP_MAX_AUTHORED_DEPTH, depth);
    if (r != FVM_OK || !depth)
        return FVM_INVALID_ARGUMENT;
    if (op == FVM_SWF_TAG_REMOVE_OBJECT || op == FVM_SWF_TAG_REMOVE_OBJECT2)
    {
        if (op == FVM_SWF_TAG_REMOVE_OBJECT2)
            display.erase(depth);
        else
        {
            unsigned long id = 0;
            r = integer(control, "character", FVM_SCENE_MAX_CHARACTER_ID, id);
            if (r != FVM_OK)
                return r;
            Display::iterator old = display.find(depth);
            if (old != display.end() && old->second["character"].number == id)
                display.erase(depth);
        }
        return FVM_OK;
    }
    if (op != FVM_SWF_TAG_PLACE_OBJECT && op != FVM_SWF_TAG_PLACE_OBJECT2)
        return FVM_INVALID_ARGUMENT;
    r = integer(control, "flags", FVM_SWF_PLACE_SUPPORTED_FLAGS, flags);
    if (r != FVM_OK || !(flags & FVM_SCENE_PLACE_MODE_MASK) || (flags & ~FVM_SWF_PLACE_SUPPORTED_FLAGS))
        return FVM_INVALID_ARGUMENT;
    Display::iterator previous = display.find(depth);
    if ((flags & FVM_SCENE_PLACE_MOVE) && previous == display.end())
        return FVM_OK;
    if (!(flags & FVM_SCENE_PLACE_MOVE))
        display.erase(depth);
    Properties projected;
    r = project(c, control, flags, projected);
    if (r != FVM_OK)
        return r;
    Properties &target = display[depth];
    bool replace_shape = (flags & FVM_SCENE_PLACE_MODE_MASK) == FVM_SCENE_PLACE_MODE_MASK && shape_symbol(target["symbol"].object) &&
                         shape_symbol(projected["symbol"].object);
    for (Properties::iterator i = projected.begin(); i != projected.end(); ++i)
    {
        if ((flags & FVM_SCENE_PLACE_MOVE) && (i->first == "name" ||
                            (!replace_shape && (i->first == "token" || i->first == "character" ||
                                                i->first == "symbol"))))
            continue;
        target[i->first] = i->second;
    }
    return FVM_OK;
}

/* Visit exact sequential control indices for one frame, rejecting holes in restored data. */
FVM_Result controls(FVM_Context *c, FVM_Object *frames, unsigned long frame, FVM_Object *&out)
{
    out = 0;
    Slots::iterator found = frames->indexed.find(frame);
    if (found == frames->indexed.end())
        return FVM_OK;
    if (found->second.type != FVM_VALUE_OBJECT || !found->second.object ||
        found->second.object->owner != c)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *record = found->second.object;
    if (record->properties.find("controls") == record->properties.end())
        return FVM_OK;
    return object(c, record, "controls", out);
}
/* [GC:] Copy only semantic control fields before any constructor can mutate their source.
 * Managed values remain live under scene_run's native barrier, including after source deletion.
 * Resource objects (symbol/matrix/color/clip actions) retain their shared identities. */
FVM_Result snapshot_controls(FVM_Context *c, FVM_Object *list,
                              fvm_detail::vector<Properties> &snapshot, FVM_ExecutionWorkScope &work)
{
    if (!list)
        return FVM_OK;
    if (list->owner != c || list->indexed.size() > FVM_MOVIECLIP_MAX_SCENE_CONTROLS)
        return FVM_INVALID_ARGUMENT;
    const char *keys[] = {"op", "depth", "token", "character", "flags", "background",
                          "symbol", "matrix", "color", "ratio", "name", "clip_actions"};
    const unsigned long count = (unsigned long)list->indexed.size();
    for (unsigned long index = 0; index < count; ++index)
    {
        FVM_Result charged = work.step();
        if (charged != FVM_OK)
            return charged;
        Slots::const_iterator found = list->indexed.find(index);
        if (found == list->indexed.end() || found->second.type != FVM_VALUE_OBJECT ||
            !found->second.object || found->second.object->owner != c)
            return FVM_INVALID_ARGUMENT;
        Properties fields;
        const Properties &source = found->second.object->properties;
        for (size_t key = 0; key < sizeof(keys) / sizeof(keys[0]); ++key)
        {
            Properties::const_iterator value = source.find(keys[key]);
            if (value != source.end())
                fields[keys[key]] = value->second;
        }
        snapshot.push_back(fields);
    }
    return FVM_OK;
}
} // namespace

/* Apply a forward frame incrementally or reconstruct its authored state after a seek/loop. */
FVM_Result fvm_movieclip_scene_run(FVM_Context *c, FVM_Object *parent, unsigned long frame,
                                   bool loop)
{
    if (!c || !parent || parent->owner != c || frame < 1 || frame > FVM_MOVIECLIP_MAX_FRAMES)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Temporary projections retain managed pointers while constructors execute. */
    FVM_NativeCallGuard guard(c);
    FVM_ExecutionWorkScope work(c);
    FVM_Object *data = 0, *frames = 0, *children = 0;
    FVM_Result r = fvm_movieclip_scene_state(parent, &data);
    if (r == FVM_OK)
        r = object(c, data, "frames", frames);
    if (r != FVM_OK)
        return r;
    unsigned long previous = 0;
    if (data->properties.find("scene_frame") != data->properties.end())
    {
        r = integer(data, "scene_frame", FVM_MOVIECLIP_MAX_FRAMES, previous);
        if (r != FVM_OK)
            return r;
    }
    r = fvm_movieclip_scene_children(parent, &children);
    if (r != FVM_OK)
        return r;
    if (!loop && frame == previous + 1)
    {
        FVM_Object *list = 0;
        r = controls(c, frames, frame, list);
        if (r != FVM_OK)
            return r;
        fvm_detail::vector<Properties> snapshot;
        r = snapshot_controls(c, list, snapshot, work);
        if (r != FVM_OK)
            return r;
        for (size_t i = 0; i < snapshot.size(); ++i)
        {
            r = work.step();
            if (r != FVM_OK)
                return r;
            r = apply(c, parent, children, snapshot[i]);
            if (r != FVM_OK)
                return r;
        }
    }
    else
    {
        Display desired;
        for (unsigned long current = 1; current <= frame; ++current)
        {
            r = work.step();
            if (r != FVM_OK)
                return r;
            FVM_Object *list = 0;
            r = controls(c, frames, current, list);
            if (r != FVM_OK)
                return r;
            fvm_detail::vector<Properties> snapshot;
            r = snapshot_controls(c, list, snapshot, work);
            if (r != FVM_OK)
                return r;
            for (size_t i = 0; i < snapshot.size(); ++i)
            {
                r = work.step();
                if (r != FVM_OK)
                    return r;
                r = accumulate(c, snapshot[i], desired);
                if (r != FVM_OK)
                    return r;
            }
        }
        /* [GC:] Snapshot child edges before unlinking changes the live table. */
        if (children->owner != c || children->indexed.size() > FVM_MOVIECLIP_ADVANCE_MAX_CHILDREN)
            return FVM_INVALID_ARGUMENT;
        for (Slots::const_iterator i = children->indexed.begin(); i != children->indexed.end(); ++i)
        {
            r = work.step();
            if (r != FVM_OK)
                return r;
        }
        Slots old = children->indexed;
        for (Slots::iterator i = old.begin(); i != old.end(); ++i)
        {
            r = work.step();
            if (r != FVM_OK)
                return r;
            if (i->second.type != FVM_VALUE_OBJECT || !i->second.object ||
                i->second.object->owner != c)
                return FVM_INVALID_ARGUMENT;
            FVM_Object *state = 0;
            r = fvm_movieclip_scene_state(i->second.object, &state);
            if (r != FVM_OK)
                return r;
            if (state->properties.find("scene_token") == state->properties.end())
                continue;
            unsigned long token = 0;
            r = integer(state, "scene_token", FVM_SCENE_MAX_TOKEN, token);
            if (r != FVM_OK)
                return r;
            if (!token)
                continue;
            Display::iterator expected = desired.find(i->first);
            if (expected == desired.end() || expected->second["token"].number != token ||
                expected->second["symbol"].object != state->properties["scene_symbol"].object)
            {
                r = fvm_movieclip_scene_remove(c, i->second.object);
                if (r != FVM_OK)
                    return r;
            }
        }
        for (Display::iterator i = desired.begin(); i != desired.end(); ++i)
        {
            r = work.step();
            if (r != FVM_OK)
                return r;
            FVM_Object *placement = 0;
            r = record(c, i->second, &placement);
            if (r != FVM_OK)
                return r;
            FVM_Object *existing = child(children, i->first), *state = 0;
            bool reusable = false;
            if (existing && fvm_movieclip_scene_state(existing, &state) == FVM_OK &&
                state->properties.find("scene_token") != state->properties.end())
                reusable = state->properties["scene_token"].number == i->second["token"].number &&
                           state->properties["scene_symbol"].object == i->second["symbol"].object;
            if (reusable)
                r = fvm_movieclip_scene_update(c, existing, placement);
            else
                r = fvm_movieclip_scene_insert(c, parent, placement, &existing);
            if (r != FVM_OK)
                return r;
        }
    }
    data->properties["scene_frame"] = number(frame);
    return FVM_OK;
}
