/* Same-context clip content replacement, retained source bases and form-variable assignment. */
#include "../core/runtime.hpp"
#include "movieclip_storage.hpp"
#include "movieclip_constants.h"
#include "../core/native_call_guard.hpp"
#include "../core/optional_method.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/swf.h"
#include "flashvm/stage.h"
#include <new>
#include <cstring>

namespace
{
using namespace fvm_movieclip_storage;
/* Validate owner-thread lifecycle and the native clip state without invoking script. */
FVM_Result owned(FVM_Context *c, FVM_Object *clip, FVM_Object *&data)
{
    if (!c || !clip || clip->owner != c || c->destroying || c->gc_collecting || c->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    FVM_Result r = state(clip, data);
    return r == FVM_OK && data->owner == c ? FVM_OK : FVM_INVALID_ARGUMENT;
}
/* Staged private detachment: no allocation is allowed during final publication. */
struct Detached
{
    FVM_Object *data; /* [GC:] Retained through the old target graph under a native barrier. */
    Properties next;  /* Fully allocated replacement metadata. */
};
/* Parse one hexadecimal digit; negative denotes malformed percent encoding. */
int hex(unsigned char value)
{
    if (value >= '0' && value <= '9')
        return value - '0';
    if (value >= 'a' && value <= 'f')
        return value - 'a' + FVM_FORM_HEX_ALPHA;
    if (value >= 'A' && value <= 'F')
        return value - 'A' + FVM_FORM_HEX_ALPHA;
    return -1;
}
/* Decode application/x-www-form-urlencoded bytes before any destination mutation. */
FVM_Result decode(const unsigned char *bytes, size_t size, fvm_detail::string &out)
{
    fvm_detail::vector<char> decoded;
    for (size_t i = 0; i < size; ++i)
    {
        unsigned char value = bytes[i];
        if (value == '+')
            value = ' ';
        else if (value == '%')
        {
            if (size - i < FVM_FORM_ESCAPE_BYTES)
                return FVM_INVALID_ARGUMENT;
            int high = hex(bytes[i + 1]), low = hex(bytes[i + 2]);
            if (high < 0 || low < 0)
                return FVM_INVALID_ARGUMENT;
            value = (unsigned char)(high * FVM_FORM_HEX_RADIX + low);
            i += FVM_FORM_ESCAPE_BYTES - 1;
        }
        decoded.push_back((char)value);
    }
    out = fvm_detail::string(decoded.begin(), decoded.size());
    size_t at = 0;
    while (at < out.size())
    {
        unsigned long scalar;
        FVM_Result status = fvm_string_read(out.data(), out.size(), at, scalar);
        if (status != FVM_OK)
            return status;
    }
    return FVM_OK;
}
/* Fully decoded values own bytes until ordered property assignment finishes. */
struct Variable
{
    fvm_detail::string name;  /* Counted property key, including decoded NUL. */
    fvm_detail::string value; /* Counted string value. */
};
} // namespace

/* Store source metadata in the clip's private managed graph, invisible to script properties. */
extern "C" FVM_Result fvm_movieclip_set_source(FVM_Context *c, FVM_Object *clip, const char *source,
                                               size_t size)
{
    FVM_Object *data = 0;
    if (owned(c, clip, data) != FVM_OK || !source || !size || size > FVM_MOVIE_SOURCE_MAX_BYTES ||
        std::memchr(source, 0, size))
        return FVM_INVALID_ARGUMENT;
    FVM_Value value;
    fvm_value_string_bytes(&value, source, size);
    return fvm_object_set(c, data, "source_base", &value);
}

/* Walk only native parents; a loaded descendant's own base overrides its source parent's base. */
extern "C" FVM_Result fvm_movieclip_get_source(FVM_Context *c, FVM_Object *clip, FVM_Value *out)
{
    if (!out || !c || !clip)
        return FVM_INVALID_ARGUMENT;
    for (unsigned int depth = 0; clip; ++depth)
    {
        FVM_Object *data = 0;
        if (depth >= FVM_MOVIECLIP_ROOT_MAX_ANCESTORS || owned(c, clip, data) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
        Properties::iterator found = data->properties.find("source_base");
        if (found != data->properties.end())
        {
            if (found->second.type != FVM_VALUE_STRING ||
                fvm_validate_value(c, &found->second) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
            size_t size = fvm_string_size(found->second);
            if (!size || size > FVM_MOVIE_SOURCE_MAX_BYTES ||
                std::memchr(found->second.string, 0, size))
                return FVM_INVALID_ARGUMENT;
            *out = found->second;
            return FVM_OK;
        }
        clip = parent(clip);
    }
    return FVM_NOT_FOUND;
}

/* Replace only content: preserve target identity, parent slot, placement and host-owned properties.
 */
extern "C" FVM_Result fvm_movieclip_replace_swf(FVM_Context *c, FVM_Object *target,
                                                const void *bytes, size_t size, const char *source,
                                                size_t source_size)
{
    FVM_Object *old = 0;
    if (owned(c, target, old) != FVM_OK || c->gc_native_depth || c->avm1_target_active ||
        c->movieclip_sound_busy || !bytes || !size || !source || !source_size ||
        source_size > FVM_MOVIE_SOURCE_MAX_BYTES || std::memchr(source, 0, source_size))
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Defer collection across import, staging and allocation-free publication. */
        FVM_NativeCallGuard guard(c);
        FVM_Object *fresh = 0;
        FVM_Result r = fvm_movieclip_load_swf(c, bytes, size, "loaded", &fresh);
        if (r != FVM_OK)
            return r;
        FVM_Object *next = fresh->resource_result.object;
        const char *preserve[] = {"parent",  "depth",   "detached",   "avm1_parent",
                                  "basis_x", "basis_y", "scene_token"};
        for (size_t i = 0; i < sizeof(preserve) / sizeof(preserve[0]); ++i)
        {
            Properties::iterator field = old->properties.find(preserve[i]);
            if (field != old->properties.end())
                next->properties[preserve[i]] = field->second;
        }
        r = fvm_movieclip_set_source(c, fresh, source, source_size);
        if (r != FVM_OK)
            return r;
        FVM_Object *children = 0;
        r = private_object(old, "children", children);
        if (r != FVM_OK || !children || children->owner != c ||
            children->indexed.size() > FVM_MOVIECLIP_ADVANCE_MAX_CHILDREN)
            return FVM_INVALID_ARGUMENT;
        Properties properties = target->properties;
        fvm_detail::vector<Detached> detach;
        for (Slots::iterator i = children->indexed.begin(); i != children->indexed.end(); ++i)
        {
            FVM_Object *child = i->second.object, *data = 0;
            if (i->second.type != FVM_VALUE_OBJECT || owned(c, child, data) != FVM_OK ||
                parent(child) != target)
                return FVM_INVALID_ARGUMENT;
            long depth = 0;
            unsigned long key = 0;
            if (depth_field(data, depth) != FVM_OK || slot_key(old, depth, key) != FVM_OK ||
                key != i->first)
                return FVM_INVALID_ARGUMENT;
            Detached staged;
            staged.data = data;
            staged.next = data->properties;
            if (staged.next.find("source_base") == staged.next.end())
            {
                FVM_Value inherited;
                FVM_Result origin = fvm_movieclip_get_source(c, child, &inherited);
                if (origin == FVM_OK)
                    staged.next["source_base"] = inherited;
                else if (origin != FVM_NOT_FOUND)
                    return origin;
            }
            staged.next["parent"] = object(0);
            staged.next["avm1_parent"] = object(target);
            staged.next["detached"] = number(FVM_MOVIECLIP_DETACHED_STATE);
            detach.push_back(staged);
            FVM_Value name;
            if (field(child, "_name", name) != FVM_OK || name.type != FVM_VALUE_STRING ||
                fvm_validate_value(c, &name) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
            fvm_detail::string binding(name.string, fvm_string_size(name));
            Properties::iterator found = properties.find(binding);
            if (found != properties.end() && found->second.type == FVM_VALUE_OBJECT &&
                found->second.object == child)
                properties.erase(binding);
        }
        FVM_Object *library = next->properties["library"].object;
        library->properties["root"] = object(target);
        /* [GC:] No allocation/callback remains: old graph edges change together. */
        for (size_t i = 0; i < detach.size(); ++i)
            detach[i].data->properties.swap(detach[i].next);
        target->properties.swap(properties);
        target->resource_result = fresh->resource_result;
        return fvm_stage_request_update(c);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}

/* Decode all fields first; ordered public setters may execute script and retain earlier effects. */
extern "C" FVM_Result fvm_object_load_variables(FVM_Context *c, FVM_Object *target,
                                                const void *input, size_t size)
{
    if (!c || !target || target->owner != c || (!input && size) || size > FVM_SWF_DEFAULT_LIMIT ||
        c->destroying || c->gc_collecting || c->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Retain the destination and staged string assignments across setter calls. */
        FVM_NativeCallGuard guard(c);
        const unsigned char *bytes = (const unsigned char *)input;
        fvm_detail::vector<Variable> variables;
        size_t fields = 0;
        for (size_t begin = 0; begin < size;)
        {
            if (++fields > FVM_FORM_MAX_FIELDS)
                return FVM_UNSUPPORTED;
            size_t end = begin, equals;
            while (end < size && bytes[end] != '&')
                ++end;
            equals = begin;
            while (equals < end && bytes[equals] != '=')
                ++equals;
            Variable variable;
            FVM_Result r = decode(bytes + begin, equals - begin, variable.name);
            if (r == FVM_OK && equals < end)
                r = decode(bytes + equals + 1, end - equals - 1, variable.value);
            if (r != FVM_OK)
                return r;
            FVM_Value value;
            fvm_value_string_bytes(&value, variable.value.data(), variable.value.size());
            if (fvm_validate_value(c, &value) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
            fvm_value_string_bytes(&value, variable.name.data(), variable.name.size());
            if (fvm_validate_value(c, &value) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
            if (!variable.name.empty())
                variables.push_back(variable);
            begin = end < size ? end + 1 : end;
            if (variables.size() > FVM_FORM_MAX_FIELDS)
                return FVM_UNSUPPORTED;
        }
        for (size_t i = 0; i < variables.size(); ++i)
        {
            FVM_Value value;
            fvm_value_string_bytes(&value, variables[i].value.data(), variables[i].value.size());
            FVM_Result r = fvm_object_set_bytes(c, target, variables[i].name.data(),
                                                variables[i].name.size(), &value);
            if (r != FVM_OK)
                return r;
        }
        FVM_Result delivered = fvm_object_call_optional(c, target, "onData");
        return delivered == FVM_OK ? fvm_stage_request_update(c) : delivered;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
