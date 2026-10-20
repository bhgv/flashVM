/* Same-context clip content replacement, retained source bases and form-variable assignment. */
#include "../core/runtime.hpp"
#include "movieclip_storage.hpp"
#include "movieclip_construction.hpp"
#include "movieclip_state.hpp"
#include "movieclip_provenance_constants.h"
#include "movieclip_constants.h"
#include "../core/native_call_guard.hpp"
#include "../core/optional_method.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/swf.h"
#include "flashvm/stage.h"
#include <new>
#include <cstring>
#include <cstdlib>
#include "bitmap_state.hpp"
#include "image_loading_constants.h"
#include "../capi/swf_image_internal.h"
#include "../capi/swf_jpeg_internal.h"

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
/* [GC:] Stage all child detachments and name-map changes before atomic content publication.
 * Caller holds a native barrier and supplies a newly built private content graph. */
FVM_Result publish_content(FVM_Context *c, FVM_Object *target, FVM_Object *old, FVM_Object *next)
{
    FVM_Result r;
    const char *preserve[] = {"parent",       "depth",    "detached",    "avm1_parent",
                              "basis_x",      "basis_y",  "scene_token", "scene_symbol",
                              "clip_actions", "lock_root"};
    for (size_t i = 0; i < sizeof(preserve) / sizeof(preserve[0]); ++i)
    {
        Properties::iterator field = old->properties.find(preserve[i]);
        if (field != old->properties.end())
            next->properties[preserve[i]] = field->second;
    }
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
    Properties::iterator library = next->properties.find("library");
    if (library != next->properties.end())
    {
        if (library->second.type != FVM_VALUE_OBJECT || !library->second.object ||
            library->second.object->owner != c)
            return FVM_INVALID_ARGUMENT;
        library->second.object->properties["root"] = object(target);
    }
    /* [GC:] No allocation/callback remains: old graph edges change together. */
    for (size_t i = 0; i < detach.size(); ++i)
        detach[i].data->properties.swap(detach[i].next);
    target->properties.swap(properties);
    target->resource_result = object(next);
    return fvm_stage_request_update(c);
}
/* [GC:] Preserve the source language version without retaining the old movie dictionary. */
FVM_Result seed_library(FVM_Context *c, FVM_Object *target, FVM_Object *next)
{
    unsigned int version = 0;
    FVM_Result r = fvm_movieclip_movie_version(c, target, &version);
    if (r != FVM_OK || !version)
        return r;
    FVM_Object *library = 0, *sprites = 0, *exports = 0;
    r = fvm_object_create(c, &library);
    if (r == FVM_OK)
        r = fvm_object_create(c, &sprites);
    if (r == FVM_OK)
        r = fvm_object_create(c, &exports);
    if (r != FVM_OK)
        return r;
    library->properties[FVM_MOVIECLIP_LIBRARY_VERSION_NAME] = number(version);
    library->properties["prototype"] = object(target->prototype);
    library->properties["sprites"] = object(sprites);
    library->properties["exports"] = object(exports);
    next->properties["library"] = object(library);
    return FVM_OK;
}
/* Decoder allocations are native malloc ownership, released even if a C++ graph allocation throws.
 */
struct ImagePixels
{
    unsigned char *bytes; /* Owned decoded straight RGBA, independent of VM roots. */
    ImagePixels() : bytes(0)
    {
    } /* Start safe for every early return. */
    ~ImagePixels()
    {
        std::free(bytes);
    } /* Match the C decoder's allocator. */
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
        r = fvm_movieclip_set_source(c, fresh, source, source_size);
        return r == FVM_OK ? publish_content(c, target, old, next) : r;
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

/* Decode and stage a complete standalone image; publish with the same child transaction as SWF. */
extern "C" FVM_Result fvm_movieclip_replace_image(FVM_Context *c, FVM_Object *target,
                                                  const void *input, size_t size,
                                                  const char *source, size_t source_size)
{
    FVM_Object *old = 0;
    if (owned(c, target, old) != FVM_OK || c->gc_native_depth || c->avm1_target_active ||
        c->movieclip_sound_busy || !input || !size || size > FVM_SWF_DEFAULT_LIMIT || !source ||
        !source_size || source_size > FVM_MOVIE_SOURCE_MAX_BYTES ||
        std::memchr(source, 0, source_size))
        return FVM_INVALID_ARGUMENT;
    try
    {
        if (fvm_movieclip_is_class_prototype(target) ||
            old->properties.find("static_art") != old->properties.end())
            return FVM_INVALID_ARGUMENT;
        const unsigned char *bytes = (const unsigned char *)input;
        bool png = size >= FVM_IMAGE_PNG_SIGNATURE_BYTES &&
                   !std::memcmp(bytes, "\211PNG\r\n\032\n", FVM_IMAGE_PNG_SIGNATURE_BYTES);
        bool gif = size >= FVM_IMAGE_GIF_SIGNATURE_BYTES &&
                   !std::memcmp(bytes, "GIF89a", FVM_IMAGE_GIF_SIGNATURE_BYTES);
        bool jpeg = size >= FVM_IMAGE_JPEG_SIGNATURE_BYTES && bytes[0] == FVM_IMAGE_JPEG_MARKER &&
                    bytes[1] == FVM_IMAGE_JPEG_SOI;
        if (!png && !gif && !jpeg)
            return FVM_UNSUPPORTED;
        unsigned int version = 0;
        FVM_Result r = fvm_movieclip_movie_version(c, target, &version);
        if (r != FVM_OK)
            return r;
        if (version && version < (jpeg ? FVM_IMAGE_JPEG_MIN_SWF : FVM_IMAGE_LOSSLESS_MIN_SWF))
            return FVM_UNSUPPORTED;
        ImagePixels decoded;
        unsigned width = 0, height = 0;
        r = png   ? fvm_swf_decode_png(bytes, size, FVM_SWF_DEFAULT_LIMIT, &width, &height,
                                       &decoded.bytes)
            : gif ? fvm_swf_decode_gif(bytes, size, FVM_SWF_DEFAULT_LIMIT, &width, &height,
                                       &decoded.bytes)
                  : fvm_swf_decode_jpeg_versioned(bytes, size, FVM_SWF_DEFAULT_LIMIT,
                                                  !version || version >= FVM_IMAGE_LOSSLESS_MIN_SWF,
                                                  &width, &height, &decoded.bytes);
        if (r != FVM_OK)
            return r;
        /* [GC:] No constructor/getter/frame script runs; temporary bitmap and clip graphs stay
         * live. */
        FVM_NativeCallGuard guard(c);
        fvm_detail::vector<unsigned char> pixels;
        size_t count = (size_t)width * height * FVM_IMAGE_RGBA_CHANNELS;
        pixels.assign(decoded.bytes, decoded.bytes + count);
        FVM_Value bitmap;
        r = fvm_bitmap_create_pixels(c, width, height, true, pixels, &bitmap);
        if (r != FVM_OK)
            return r;
        FVM_Object *fresh = 0;
        r = fvm_movieclip_image_content(c, bitmap, width, height, &fresh);
        if (r != FVM_OK)
            return r;
        FVM_Object *next = fresh->resource_result.object;
        next->properties["playing"] = number(FVM_MOVIECLIP_PLAYBACK_STOPPED);
        r = seed_library(c, target, next);
        if (r == FVM_OK)
            r = fvm_movieclip_set_source(c, fresh, source, source_size);
        return r == FVM_OK ? publish_content(c, target, old, next) : r;
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

/* Select by binary signature, never extension or MIME, while preserving strict SWF import errors.
 */
extern "C" FVM_Result fvm_movieclip_replace_resource(FVM_Context *c, FVM_Object *target,
                                                     const void *bytes, size_t size,
                                                     const char *source, size_t source_size)
{
    if (bytes && size >= FVM_IMAGE_SWF_SIGNATURE_BYTES &&
        (!std::memcmp(bytes, "FWS", FVM_IMAGE_SWF_SIGNATURE_BYTES) ||
         !std::memcmp(bytes, "CWS", FVM_IMAGE_SWF_SIGNATURE_BYTES)))
        return fvm_movieclip_replace_swf(c, target, bytes, size, source, source_size);
    return fvm_movieclip_replace_image(c, target, bytes, size, source, source_size);
}

/* Read native attachment independently from replaceable _parent/_root script descriptors. */
extern "C" FVM_Result fvm_movieclip_attached_to(FVM_Context *c, FVM_Object *clip,
                                                FVM_Object *ancestor, int *out)
{
    FVM_Object *data = 0;
    if (!out || owned(c, clip, data) != FVM_OK || owned(c, ancestor, data) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        if (fvm_movieclip_is_class_prototype(clip) || fvm_movieclip_is_class_prototype(ancestor))
            return FVM_INVALID_ARGUMENT;
        for (unsigned int depth = 0; clip; ++depth)
        {
            if (depth >= FVM_MOVIECLIP_QUERY_MAX_DEPTH || owned(c, clip, data) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
            Properties::iterator marker = data->properties.find("detached");
            if (marker != data->properties.end())
            {
                const FVM_Value &value = marker->second;
                if (value.type != FVM_VALUE_NUMBER ||
                    (value.number != FVM_MOVIECLIP_ATTACHED_STATE &&
                     value.number != FVM_MOVIECLIP_DETACHED_STATE))
                    return FVM_INVALID_ARGUMENT;
                if (value.number == FVM_MOVIECLIP_DETACHED_STATE)
                {
                    *out = 0;
                    return FVM_OK;
                }
            }
            if (clip == ancestor)
            {
                *out = 1;
                return FVM_OK;
            }
            FVM_Value owner;
            if (field(data, "parent", owner) != FVM_OK ||
                (owner.type != FVM_VALUE_OBJECT && owner.type != FVM_VALUE_NULL))
                return FVM_INVALID_ARGUMENT;
            clip = owner.type == FVM_VALUE_OBJECT ? owner.object : 0;
        }
        *out = 0;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Clear imported/drawn contents without invoking replaceable constructors or script handlers. */
extern "C" FVM_Result fvm_movieclip_unload(FVM_Context *c, FVM_Object *target)
{
    FVM_Object *old = 0;
    if (owned(c, target, old) != FVM_OK || c->gc_native_depth || c->avm1_target_active ||
        c->movieclip_sound_busy)
        return FVM_INVALID_ARGUMENT;
    try
    {
        if (fvm_movieclip_is_class_prototype(target) ||
            old->properties.find("static_art") != old->properties.end())
            return FVM_INVALID_ARGUMENT; /* Internal static/prototype nodes are not instances. */
        /* [GC:] Keep every temporary graph and old child edge reachable until publication. */
        FVM_NativeCallGuard guard(c);
        FVM_Object *fresh = 0;
        FVM_Result r = fvm_movieclip_empty_content(c, &fresh);
        if (r != FVM_OK)
            return r;
        FVM_Object *next = fresh->resource_result.object;
        next->properties["playing"] = number(FVM_MOVIECLIP_PLAYBACK_STOPPED);
        Properties::iterator source = old->properties.find("source_base");
        if (source != old->properties.end())
        {
            FVM_Value location;
            r = fvm_movieclip_get_source(c, target, &location);
            if (r != FVM_OK)
                return r;
            next->properties["source_base"] = location;
        }
        r = seed_library(c, target, next);
        if (r != FVM_OK)
            return r;
        return publish_content(c, target, old, next);
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
