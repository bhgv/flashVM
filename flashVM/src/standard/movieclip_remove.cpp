/* FlashVM: transactional forced native MovieClip detachment, independent of Action encoding. */
#include "movieclip_scene.hpp"
#include "movieclip_storage.hpp"
#include <new>

using namespace fvm_movieclip_storage;


/* Prepare every ownership-map change before detaching a clip from its native parent. */
FVM_Result fvm_movieclip_scene_remove(FVM_Context *c, FVM_Object *o)
{
    if (!c || !o || o->owner != c)
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {
        FVM_Object *data = 0;
        FVM_Result status = state(o, data);
        if (status != FVM_OK)
        {
            return status;
        }
        FVM_Object *p = parent(o), *children = 0;
        unsigned long key = 0;
        if (p)
        {
            if (p->owner != c)
            {
                return FVM_INVALID_ARGUMENT;
            }
            FVM_Object *parent_data = 0;
            long depth = 0;
            status = state(p, parent_data);
            if (status == FVM_OK)
            {
                status = private_object(parent_data, "children", children);
            }
            if (status == FVM_OK)
            {
                status = depth_field(data, depth);
            }
            if (status == FVM_OK)
            {
                status = slot_key(parent_data, depth, key);
            }
            if (status != FVM_OK)
            {
                return status;
            }
        }
        /* [GC:] Source maps keep managed edges alive until all replacement maps are prepared. */
        Properties detached = data->properties;
        /* [GC:] Loaded AVM1 retains its former display ancestry after physical removal. */
        if (c->swf_loaded && p)
        {
            detached["avm1_parent"] = object(p);
        }
        detached["parent"] = object(0);
        detached["detached"] = number(FVM_MOVIECLIP_DETACHED_STATE);
        detached["playing"] = number(FVM_MOVIECLIP_PLAYBACK_STOPPED);
        Slots next_children;
        Properties next_bindings;
        if (p)
        {
            next_children = children->indexed;
            next_bindings = p->properties;
            Slots::iterator slot = next_children.find(key);
            if (slot != next_children.end() && slot->second.type == FVM_VALUE_OBJECT && slot->second.object == o)
            {
                next_children.erase(key);
            }
            FVM_Value name;
            fvm_value_undefined(&name);
            if (field(o, "_name", name) == FVM_OK && name.type == FVM_VALUE_STRING)
            {


                fvm_detail::string binding(name.string, fvm_string_size(name));
                Properties::iterator entry = next_bindings.find(binding);
                if (entry != next_bindings.end() && entry->second.type == FVM_VALUE_OBJECT && entry->second.object == o)
                {
                    next_bindings.erase(binding);
                }
            }
        }
        /* [GC:] Publish only prepared maps; no allocation or script occurs after the first swap. */
        if (p)
        {
            children->indexed.swap(next_children);
            p->properties.swap(next_bindings);
        }
        data->properties.swap(detached);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
