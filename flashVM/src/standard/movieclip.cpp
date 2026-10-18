/* FlashVM: Built-in ActionScript objects: movieclip.
 * File: src/standard/movieclip.cpp.
 */
#include "movieclip_quality.hpp"
#include "../render/quality_internal.hpp"


#include "../core/string_internal.hpp"
#include "movieclip_state.hpp"
#include "movieclip_construction.hpp"
#include "constructor_identity.hpp"
#include "movieclip_lockroot.hpp"
#include "movieclip_pointer.hpp"
#include "movieclip_point.hpp"
#include "movieclip_drag.hpp"
#include "movieclip_constants.h"
#include "movieclip_storage.hpp"
#include "movieclip_path_constants.h"
#include "../avm1/display_property_ids.h"
#include "../avm1/block.hpp"
#include "movieclip_scene.hpp"
#include "transform_scene.hpp"
#include "gradient.hpp"
#include "bitmap_fill.hpp"
#include "bitmap_draw.hpp"
#include "bitmap_state.hpp"
#include "function_state.hpp"
#include "base_state.hpp"
#include "../core/call_scope.hpp"
#include "flashvm/property.h"
#include "flashvm/serialization.h"
#include "flashvm/movieclip.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/avm1.h"
#include "flashvm/path.h"
#include "flashvm/gc.h"
#include "../core/native_call_guard.hpp"
#include <cmath>
#include <cfloat>
#include <climits>
#include <cstring>
#include <cstdio>
#include <new>
#include <limits>
#include "string_order.hpp"
namespace
{
/* Define anonymous enum and its representation. */
enum
{
    CLIP = FVM_RESOURCE_MOVIECLIP,
    CLIP_CLASS = FVM_RESOURCE_MOVIECLIP_CLASS
};
using fvm_movieclip_storage::Slots;
using fvm_movieclip_storage::Properties;
using fvm_movieclip_storage::object;
using fvm_movieclip_storage::number;
using fvm_movieclip_storage::state;
using fvm_movieclip_storage::field;
using fvm_movieclip_storage::private_object;
using fvm_movieclip_storage::parent;
using fvm_movieclip_storage::integer_field;
using fvm_movieclip_storage::depth_field;
using fvm_movieclip_storage::slot_bias;
using fvm_movieclip_storage::slot_key;

/* Release a temporary native vector path on every return or allocation exception. */
struct OwnedPath
{
    FVM_Path *path; /* Native geometry owned by this scope, independent of managed scene edges. */
    /* Adopt an already constructed path without allocating memory. */
    explicit OwnedPath(FVM_Path *value) : path(value)
    {
    }
    /* Release native geometry when this render/bounds/hit scope exits. */
    ~OwnedPath()
    {
        fvm_path_destroy(path);
    }

  private:
    /* Prevent accidental duplicate ownership of the same native path. */
    OwnedPath(const OwnedPath &);
    /* Prevent assignment from transferring ownership implicitly. */
    OwnedPath &operator=(const OwnedPath &);
};

/* Distinguish internal static display nodes from script-visible MovieClip instances. */
bool static_art(FVM_Object *o)
{
    FVM_Object *data = 0;
    FVM_Value value;
    return state(o, data) == FVM_OK && field(data, "static_art", value) == FVM_OK &&
           value.type == FVM_VALUE_NUMBER && value.number == 1;
}

/* Identify a solid-shape definition without interpreting an absent legacy kind marker. */
bool shape_symbol(FVM_Object *o)
{
    FVM_Value value;
    return o && field(o, "kind", value) == FVM_OK && value.type == FVM_VALUE_NUMBER &&
           value.number == 2;
}

/* Compute and return numeric. */
FVM_Result numeric(FVM_Context *c, FVM_Object *o, const char *name, double &out)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value v;
    FVM_Result r = fvm_object_get(c, o, name, &v);
    if (r != FVM_OK)
        return r;
    r = fvm_value_to_number(c, &v, &out);
    return r != FVM_OK                               ? r
           : out == out && std::fabs(out) <= DBL_MAX ? FVM_OK
                                                     : FVM_INVALID_ARGUMENT;
}

/* Compute and return args. */
FVM_Result args(FVM_Context *c, unsigned int n, const FVM_Value *a, unsigned int i, double fallback,
                double &out)
{
    if (i >= n)
    {
        out = fallback;
        return FVM_OK;
    }
    FVM_Result r = fvm_value_to_number(c, a + i, &out);
    return r != FVM_OK                               ? r
           : out == out && std::fabs(out) <= DBL_MAX ? FVM_OK
                                                     : FVM_INVALID_ARGUMENT;
}

/* Convert finite RGB numbers to the unsigned 24-bit paint domain before narrowing casts. */
unsigned long packed_rgb(double value)
{
    double reduced = std::fmod(value < 0 ? std::ceil(value) : std::floor(value), 16777216.0);
    if (reduced < 0)
        reduced += 16777216.0;
    return (unsigned long)reduced;
}

/* [GC:] Upgrade a legacy child table atomically, preserving all existing object edges. */
FVM_Result ensure_slot_bias(FVM_Object *data, FVM_Object *children)
{
    unsigned long bias = 0;
    FVM_Result r = slot_bias(data, bias);
    if (r != FVM_OK || bias)
        return r;
    Slots next;
    for (Slots::iterator i = children->indexed.begin(); i != children->indexed.end(); ++i)
    {
        if (i->first > FVM_MOVIECLIP_MAX_PUBLIC_DEPTH)
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Copy the same children; migration must not clone or detach live instances. */
        next[i->first + FVM_MOVIECLIP_CHILD_SLOT_BIAS] = i->second;
    }
    data->properties["slot_bias"] = number(FVM_MOVIECLIP_CHILD_SLOT_BIAS);
    children->indexed.swap(next);
    return FVM_OK;
}


/* Compute and return make. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result make(FVM_Context *c, const char *name, unsigned int frames, FVM_Object *proto,
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Object **out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!frames || frames > FVM_MOVIECLIP_MAX_FRAMES)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    /* Custom prototype setters must not collect a partially constructed tree. */
    /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
    FVM_NativeCallGuard construction(c);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *o = 0, *data = 0, *children = 0, *paths = 0, *timeline = 0;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result r = fvm_object_create(c, &o);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_object_create(c, &data);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_object_create(c, &children);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_object_create(c, &paths);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_object_create(c, &timeline);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    o->resource_kind = CLIP;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    o->prototype = proto;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    o->resource_result_valid = true;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    o->resource_result = object(data);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["parent"] = object(0);
    /* A new native root/instance has never been detached, even without a parent. */
    data->properties["detached"] = number(FVM_MOVIECLIP_ATTACHED_STATE);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["children"] = object(children);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["paths"] = object(paths);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["frames"] = object(timeline);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["frame"] = number(1);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["total"] = number(frames);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["playing"] = number(1);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["entered"] = number(0);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["depth"] = number(0);
    /* Biased slots place authored SWF depths below dynamic ActionScript depths. */
    data->properties["slot_bias"] = number(FVM_MOVIECLIP_CHILD_SLOT_BIAS);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["cx"] = number(0);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["cy"] = number(0);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["fill"] = number(0);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["rgb"] = number(0);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["alpha"] = number(100);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["stroke"] = number(0);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["width"] = number(0);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["lineRgb"] = number(0);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["lineAlpha"] = number(100);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    data->properties["active"] = object(0);

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const char *keys[] = {
        "_x", "_y", "_xscale", "_yscale", "_rotation",
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        "_alpha", "_visible", "enabled", "useHandCursor"};
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const double defaults[] = {0, 0, 100, 100, 0, 100, 1, 1, 1};
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (unsigned int i = 0; i < 9; ++i)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->properties[keys[i]] = number(defaults[i]);
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        o->attributes[keys[i]] = 1;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value text;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_string(&text, name);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_object_set(c, o, "_name", &text);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    o->attributes["_name"] = 1;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    *out = o;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Read parent. */
FVM_Result parent_get(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *,
                      FVM_Value *out, void *)
{
    FVM_Object *p = 0;
    FVM_Result status = fvm_movieclip_avm1_parent(c, o, &p);
    if (status == FVM_OK)
        *out = object(p);
    return status;
}

/* [GC:] Resolve the versioned native root, retaining loaded removed-instance ancestry. */
FVM_Result root_get(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                    void *)
{
    unsigned int depth = 0;
    for (;;)
    {
        if (++depth > FVM_MOVIECLIP_ROOT_MAX_ANCESTORS)
            return FVM_INVALID_ARGUMENT;
        bool locked = false;
        FVM_Result status = fvm_movieclip_root_locked(c, o, &locked);
        if (status != FVM_OK)
            return status;
        if (locked)
            break;
        FVM_Object *p = 0;
        status = fvm_movieclip_avm1_parent(c, o, &p);
        if (status != FVM_OK)
            return status;
        if (!p)
            break;
        o = p;
    }
    *out = object(o);
    return FVM_OK;
}

/* [GC:] Read frame. */
FVM_Result frame_get(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                     void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    return r == FVM_OK ? field(data, "frame", *out) : r;
}

/* Read total. */
FVM_Result total_get(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                     void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    return r == FVM_OK ? field(data, "total", *out) : r;
}

/* Build counted dot/slash display paths without invoking replaceable _name accessors. */
FVM_Result display_path(FVM_Context *c, FVM_Object *clip, FVM_Value *out, bool slash)
{
    if (!c || !clip || clip->owner != c || !out)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::vector<fvm_detail::string> names;
        FVM_Object *cursor = clip, *root = clip;
        unsigned int visited = 0;
        while (cursor)
        {
            if (++visited > FVM_MOVIECLIP_PATH_MAX_VISITS)
                return FVM_ERROR;
            FVM_Object *data = 0;
            FVM_Result status = state(cursor, data);
            if (status != FVM_OK)
                return status;
            FVM_Object *next = parent(cursor);
            if (!next)
            {
                root = cursor;
                break;
            }
            FVM_Value name;
            status = field(cursor, "_name", name);
            if (status != FVM_OK || name.type != FVM_VALUE_STRING)
                return FVM_INVALID_ARGUMENT;
            names.push_back(fvm_detail::string(name.string, fvm_string_size(name)));
            cursor = next;
        }
        FVM_Object *data = 0;
        long level = 0;
        FVM_Result status = state(root, data);
        if (status == FVM_OK)
            status = depth_field(data, level);
        if (status != FVM_OK)
            return status;
        fvm_detail::vector<char> bytes;
        if (!slash || (level && !names.empty()))
        {
            char prefix[FVM_MOVIECLIP_PATH_LEVEL_PREFIX_BYTES];
            std::sprintf(prefix, "_level%ld", level);
            for (size_t i = 0; prefix[i]; ++i)
                bytes.push_back(prefix[i]);
        }
        if (slash && names.empty())
            bytes.push_back('/');
        for (size_t i = names.size(); i > 0; --i)
        {
            bytes.push_back(slash ? '/' : '.');
            for (size_t j = 0; j < names[i - 1].size(); ++j)
                bytes.push_back(names[i - 1][j]);
        }
        FVM_Value borrowed;
        status = fvm_value_string_bytes(&borrowed, bytes.begin(), bytes.size());
        /* [GC:] Copy the complete counted path to context-owned string storage. */
        return status == FVM_OK ? fvm_value_to_string(c, &borrowed, out) : status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Return the slash-syntax _target property independently from MovieClip.toString. */
FVM_Result target(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                  void *)
{
    return display_path(c, o, out, true);
}

/* Return the default dot-syntax MovieClip string used by the built-in method. */
FVM_Result clip_string(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *,
                       FVM_Value *out, void *)
{
    return fvm_movieclip_reference_string(c, o, out);
}

/* [GC:] Publish a prepared child atomically, detaching any old instance at the depth. */
FVM_Result publish_child(FVM_Context *c, FVM_Object *o, FVM_Object *child, const char *name,
                         long depth)
{
    FVM_Object *data = 0, *children = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "children", children);
    if (r != FVM_OK)
        return r;
    if (depth < 0)
    {
        r = ensure_slot_bias(data, children);
        if (r != FVM_OK)
            return r;
    }
    unsigned long key = 0;
    r = slot_key(data, depth, key);
    if (r != FVM_OK)
        return r;
    FVM_Object *child_data = child->resource_result.object;

    /* Computation / Вычисление и обновление состояния. */
    child_data->properties["parent"] = object(o);
    child_data->properties["depth"] = number(depth);

    /* Initialization / Инициализация локального состояния. */
    Slots next = children->indexed;
    fvm_detail::map<fvm_detail::string, FVM_Value> props = o->properties;
    Slots::iterator old = children->indexed.find(key);
    FVM_Object *replaced = old != children->indexed.end() && old->second.type == FVM_VALUE_OBJECT
                               ? old->second.object
                               : 0;
    if (replaced)
    {
        FVM_Value old_name;
        fvm_value_undefined(&old_name);
        if (field(replaced, "_name", old_name) == FVM_OK && old_name.type == FVM_VALUE_STRING)
        {
            /* Preserve embedded NUL bytes when removing the replaced child's binding. */
            fvm_detail::string binding(old_name.string, fvm_string_size(old_name));
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator p = props.find(binding);
            if (p != props.end() && p->second.type == FVM_VALUE_OBJECT &&
                p->second.object == replaced)
                props.erase(binding);
        }
    }
    next[key] = object(child);
    if (!static_art(child) && props.find(name) == props.end())
        props[name] = object(child);
    /* [GC:] Stage detachment before publishing anything; OOM must preserve the old child graph. */
    FVM_Object *old_data = 0;
    Properties detached;
    if (replaced && state(replaced, old_data) == FVM_OK)
    {
        /* [GC:] Copy the previous node's retained links before dropping its parent edge. */
        detached = old_data->properties;
        /* [GC:] Preserve the loaded old instance's ancestry in the same atomic replacement. */
        if (c->swf_loaded && parent(replaced))
            detached["avm1_parent"] = object(parent(replaced));
        /* [GC:] Physical detachment is independent of retained AVM1 provenance. */
        detached["parent"] = object(0);
        detached["detached"] = number(FVM_MOVIECLIP_DETACHED_STATE);
        detached["playing"] = number(FVM_MOVIECLIP_PLAYBACK_STOPPED);
    }

    /* [GC:] All following swaps publish prepared ownership maps without allocating. */
    if (old_data)
        old_data->properties.swap(detached);
    children->indexed.swap(next);
    o->properties.swap(props);
    return FVM_OK;
}

/* Validate counted string arguments before using the current NUL-terminated name model. */
FVM_Result clip_name(FVM_Context *c, const FVM_Value *input, FVM_Value &name)
{
    FVM_Result r = fvm_value_to_string(c, input, &name);
    if (r != FVM_OK)
        return r;
    return std::memchr(name.string, 0, fvm_string_size(name)) ? FVM_INVALID_ARGUMENT : FVM_OK;
}

/* Create an empty child while retaining its parent's optional SWF resource library. */
FVM_Result create_child(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                        FVM_Value *out, void *)
{
    if (n < 2)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    FVM_Value name;
    r = clip_name(c, a, name);
    if (r != FVM_OK)
        return r;
    double depth = 0;
    r = args(c, n, a, 1, 0, depth);
    if (r != FVM_OK)
        return r;
    if (depth < FVM_MOVIECLIP_MIN_PUBLIC_DEPTH || depth > FVM_MOVIECLIP_MAX_PUBLIC_DEPTH ||
        depth != std::floor(depth))
        return FVM_INVALID_ARGUMENT;
    FVM_Object *child = 0, *proto = o->prototype;
    FVM_Value library;
    bool imported = field(data, "library", library) == FVM_OK;
    if (imported)
    {
        if (library.type != FVM_VALUE_OBJECT || !library.object)
            return FVM_INVALID_ARGUMENT;
        r = private_object(library.object, "prototype", proto);
        if (r != FVM_OK)
            return r;
    }
    /* [GC:] Native dispatch protects the new child before its parent edge is installed. */
    r = make(c, name.string, 1, proto, &child);
    if (r != FVM_OK)
        return r;
    /* [GC:] Empty descendants retain the library so attachMovie can resolve linkage names. */
    if (imported)
        child->resource_result.object->properties["library"] = library;
    r = publish_child(c, o, child, name.string, (long)depth);
    if (r == FVM_OK)
        *out = object(child);
    return r;
}

/* Copy enumerable initObject properties, invoking getters/setters on their actual receivers. */
FVM_Result initialize_child(FVM_Context *c, FVM_Object *child, const FVM_Value &initial)
{
    if (initial.type != FVM_VALUE_OBJECT)
        return FVM_OK;
    FVM_Object *keys = 0;
    FVM_Result r = fvm_object_enumerate(c, initial.object, 1, &keys);
    if (r != FVM_OK)
        return r;
    /* [GC:] The native-call barrier retains this temporary key snapshot across accessor calls. */
    Slots snapshot = keys->indexed;
    for (Slots::iterator i = snapshot.begin(); i != snapshot.end(); ++i)
    {
        if (i->second.type != FVM_VALUE_STRING ||
            std::memchr(i->second.string, 0, fvm_string_size(i->second)))
            return FVM_INVALID_ARGUMENT;
        FVM_Value value;
        r = fvm_object_get(c, initial.object, i->second.string, &value);
        if (r == FVM_OK)
            r = fvm_object_set(c, child, i->second.string, &value);
        if (r != FVM_OK)
            return r;
    }
    return FVM_OK;
}


/* Initialize a published instance, then call its class constructor without replacing the clip. */
FVM_Result finish_instance(FVM_Context *c, FVM_Object *child, FVM_Object *constructor,
                           const FVM_Value *initial, FVM_Value *out)
{
    FVM_Result r = initial ? initialize_child(c, child, *initial) : FVM_OK;
    if (r != FVM_OK)
        return r;
    if (constructor)
    {
        /* [GC:] Retain constructor/owner metadata across super calls and exception unwinding. */
        FVM_CallScope scope(c, constructor, constructor, child->prototype);
        /* The symbol movie controls constructor identities even when caller SWF differs. */
        unsigned int movie_version = 0;
        r = fvm_movieclip_movie_version(c, child, &movie_version);
        if (r == FVM_OK)
            r = fvm_function_define_constructor(c, child, constructor, movie_version);
        if (r != FVM_OK)
            return r;
        FVM_Value ignored;
        r = fvm_function_invoke(c, constructor, child, 0, 0, &ignored);
        if (r != FVM_OK)
            return r;
    }
    *out = object(child);
    return FVM_OK;
}

/* Attach an exported sprite with independent playback and optional registered class/initObject. */
FVM_Result attach_movie(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                        FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (n < 3)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0, *library = 0, *names = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    if (data->properties.find("library") == data->properties.end())
        return FVM_OK;
    r = private_object(data, "library", library);
    if (r != FVM_OK)
        return r;
    r = private_object(library, "exports", names);
    if (r != FVM_OK)
        return r;
    FVM_Value linkage, name;
    r = clip_name(c, a, linkage);
    if (r == FVM_OK)
        r = clip_name(c, a + 1, name);
    if (r != FVM_OK)
        return r;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator entry =
        names->properties.find(linkage.string);
    if (entry == names->properties.end())
        return FVM_OK;
    if (entry->second.type != FVM_VALUE_OBJECT || !entry->second.object)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *definition = entry->second.object, *frames = 0;
    if (shape_symbol(definition))
        return FVM_OK;
    unsigned long total = 0;
    r = integer_field(definition, "total", FVM_MOVIECLIP_MAX_FRAMES, total);
    if (r == FVM_OK)
        r = private_object(definition, "frames", frames);
    if (r != FVM_OK || !total)
        return FVM_INVALID_ARGUMENT;
    double depth = 0;
    r = args(c, n, a, 2, 0, depth);
    if (r != FVM_OK)
        return r;
    if (depth < FVM_MOVIECLIP_MIN_PUBLIC_DEPTH || depth > FVM_MOVIECLIP_MAX_PUBLIC_DEPTH ||
        depth != std::floor(depth))
        return FVM_INVALID_ARGUMENT;

    FVM_Object *proto = 0, *constructor = 0;
    r = fvm_movieclip_sprite_class(c, library, linkage.string, proto, constructor);
    if (r != FVM_OK)
        return r;
    FVM_Object *child = 0, *child_frames = 0;
    r = make(c, name.string, (unsigned int)total, proto, &child);
    if (r != FVM_OK)
        return r;
    r = private_object(child->resource_result.object, "frames", child_frames);
    if (r != FVM_OK)
        return r;
    /* [GC:] Only immutable frame records are shared; each child owns its mutable frame table. */
    child_frames->indexed = frames->indexed;
    child->resource_result.object->properties["library"] = object(library);
    r = fvm_object_set(c, child->resource_result.object, "linkage", &linkage);
    if (r != FVM_OK)
        return r;
    r = publish_child(c, o, child, name.string, (long)depth);
    if (r != FVM_OK)
        return r;
    return finish_instance(c, child, constructor, n > 3 ? a + 3 : 0, out);
}


/* Duplicate a sibling at frame 1 with copied display/drawing state and fresh dynamic properties. */
FVM_Result duplicate_instance(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                              FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (n < FVM_MOVIECLIP_DUPLICATE_REQUIRED_ARGUMENTS)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    FVM_Object *p = parent(o);
    if (!p)
        return FVM_OK;
    FVM_Value name;
    r = clip_name(c, a + FVM_MOVIECLIP_DUPLICATE_NAME_INDEX, name);
    if (r != FVM_OK)
        return r;
    double depth = 0;
    r = args(c, n, a, FVM_MOVIECLIP_DUPLICATE_DEPTH_INDEX, 0, depth);
    if (r != FVM_OK)
        return r;
    if (depth < FVM_MOVIECLIP_MIN_PUBLIC_DEPTH || depth > FVM_MOVIECLIP_MAX_PUBLIC_DEPTH ||
        depth != std::floor(depth))
        return FVM_INVALID_ARGUMENT;
    unsigned long total = 0;
    FVM_Object *frames = 0;
    r = integer_field(data, "total", FVM_MOVIECLIP_MAX_FRAMES, total);
    if (r == FVM_OK)
        r = private_object(data, "frames", frames);
    if (r != FVM_OK || !total)
        return FVM_INVALID_ARGUMENT;

    FVM_Object *proto = o->prototype, *constructor = 0, *library = 0;
    FVM_Value linkage;
    bool imported = data->properties.find("library") != data->properties.end();
    bool has_linkage = data->properties.find("linkage") != data->properties.end();
    if (imported)
    {
        r = private_object(data, "library", library);
        if (r != FVM_OK)
            return r;
    }
    if (has_linkage)
    {
        r = field(data, "linkage", linkage);
        if (r != FVM_OK || !library || linkage.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
        r = fvm_movieclip_sprite_class(c, library, linkage.string, proto, constructor);
        if (r != FVM_OK)
            return r;
    }
    FVM_Object *child = 0, *child_frames = 0;
    r = make(c, name.string, (unsigned int)total, proto, &child);
    if (r != FVM_OK)
        return r;
    r = private_object(child->resource_result.object, "frames", child_frames);
    if (r != FVM_OK)
        return r;
    /* [GC:] Frame definitions remain shared, while the duplicate owns its replaceable table. */
    child_frames->indexed = frames->indexed;
    /* [GC:] Retain the resource library for future descendant attachments and serialization. */
    if (library)
        child->resource_result.object->properties["library"] = object(library);
    if (has_linkage)
    {
        r = fvm_object_set(c, child->resource_result.object, "linkage", &linkage);
        if (r != FVM_OK)
            return r;
    }
    const char *display[] = {"_x",     "_y",       "_xscale", "_yscale",      "_rotation",
                             "_alpha", "_visible", "enabled", "useHandCursor"};
    for (size_t i = 0; i < sizeof(display) / sizeof(display[0]); ++i)
    {
        FVM_Value value;
        r = fvm_object_get(c, o, display[i], &value);
        if (r == FVM_OK)
            r = fvm_object_set(c, child, display[i], &value);
        if (r != FVM_OK)
            return r;
    }
    /* [GC:] Color records are immutable; copies retain the record while owning their alpha field.
     */
    if (data->properties.find("swf_color") != data->properties.end())
        child->resource_result.object->properties["swf_color"] = data->properties["swf_color"];
    if (data->properties.find("basis_x") != data->properties.end())
    {
        child->resource_result.object->properties["basis_x"] = data->properties["basis_x"];
        child->resource_result.object->properties["basis_y"] = data->properties["basis_y"];
    }
    r = fvm_movieclip_copy_drawing(c, data, child->resource_result.object);
    if (r != FVM_OK)
        return r;
    r = publish_child(c, p, child, name.string, (long)depth);
    if (r != FVM_OK)
        return r;
    return finish_instance(
        c, child, constructor,
        n > FVM_MOVIECLIP_DUPLICATE_INITIAL_INDEX ? a + FVM_MOVIECLIP_DUPLICATE_INITIAL_INDEX : 0,
        out);
}

/* Keep the stable script callback while sharing explicit construction with generated bodies. */
FVM_Result duplicate_movie(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                           FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (n < FVM_MOVIECLIP_DUPLICATE_REQUIRED_ARGUMENTS)
        return FVM_INVALID_ARGUMENT;
    return fvm_exec_duplicate_clip(
        c, o, a + FVM_MOVIECLIP_DUPLICATE_NAME_INDEX, a + FVM_MOVIECLIP_DUPLICATE_DEPTH_INDEX,
        n > FVM_MOVIECLIP_DUPLICATE_INITIAL_INDEX ? a + FVM_MOVIECLIP_DUPLICATE_INITIAL_INDEX : 0,
        out);
}

/* Adapt the stable native callback signature to the shared forced-detachment operation. */
FVM_Result remove(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                  void *)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    FVM_Result status = fvm_movieclip_scene_remove(c, o);
    if (status == FVM_OK)
        fvm_value_undefined(out);
    return status;
}

/* Read depth. */
FVM_Result get_depth(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                     void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    return r == FVM_OK ? field(data, "depth", *out) : r;
}

/* Compute and return next depth. */
FVM_Result next_depth(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                      void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0, *children = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "children", children);
    if (r != FVM_OK)
        return r;
    unsigned long next = 0, bias = 0;
    r = slot_bias(data, bias);
    if (r != FVM_OK)
        return r;
    for (Slots::iterator i = children->indexed.begin(); i != children->indexed.end(); ++i)
    {
        if (i->first > FVM_MOVIECLIP_MAX_PUBLIC_DEPTH + bias)
            return FVM_INVALID_ARGUMENT;
        if (i->first >= bias && i->first - bias >= next)
            next = i->first - bias + 1;
    }
    fvm_value_number(out, next);
    return FVM_OK;
}

/* Compute and return at depth. */
FVM_Result at_depth(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                    FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0, *children = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "children", children);
    if (r != FVM_OK)
        return r;
    double depth = 0;
    r = args(c, n, a, 0, 0, depth);
    if (r != FVM_OK || depth < FVM_MOVIECLIP_MIN_PUBLIC_DEPTH ||
        depth > FVM_MOVIECLIP_MAX_PUBLIC_DEPTH ||
        depth != std::floor(depth))
        return FVM_INVALID_ARGUMENT;
    unsigned long key = 0;
    r = slot_key(data, (long)depth, key);
    if (r == FVM_NOT_FOUND)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    if (r != FVM_OK)
        return r;
    Slots::iterator i = children->indexed.find(key);
    if (i == children->indexed.end() ||
        (i->second.type == FVM_VALUE_OBJECT && static_art(i->second.object)))
        fvm_value_undefined(out);
    else
        *out = i->second;
    return FVM_OK;
}

/* Exchange depths. */
FVM_Result swap_depths(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                       FVM_Value *out, void *)
{
    if (!n || !parent(o))
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0, *pd = 0, *children = 0, *other = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = state(parent(o), pd);
    if (r != FVM_OK)
        return r;
    r = private_object(pd, "children", children);
    if (r != FVM_OK)
        return r;
    long old = 0;
    unsigned long old_key = 0, desired_key = 0;
    r = depth_field(data, old);
    if (r != FVM_OK)
        return r;
    double desired = 0;
    if (a[0].type == FVM_VALUE_OBJECT)
    {
        other = a[0].object;
        if (parent(other) != parent(o))
            return FVM_INVALID_ARGUMENT;
        FVM_Object *od = 0;
        long d = 0;
        r = state(other, od);
        if (r != FVM_OK)
            return r;
        r = depth_field(od, d);
        if (r != FVM_OK)
            return r;
        desired = d;
    }
    else
    {
        r = args(c, n, a, 0, 0, desired);
        if (r != FVM_OK || desired < FVM_MOVIECLIP_MIN_PUBLIC_DEPTH ||
            desired > FVM_MOVIECLIP_MAX_PUBLIC_DEPTH || desired != std::floor(desired))
            return FVM_INVALID_ARGUMENT;
    }
    r = ensure_slot_bias(pd, children);
    if (r != FVM_OK)
        return r;
    r = slot_key(pd, old, old_key);
    if (r == FVM_OK)
        r = slot_key(pd, (long)desired, desired_key);
    if (r != FVM_OK)
        return r;
    if (!other)
    {
        Slots::iterator i = children->indexed.find(desired_key);
        if (i != children->indexed.end())
        {
            if (i->second.type != FVM_VALUE_OBJECT)
                return FVM_INVALID_ARGUMENT;
            other = i->second.object;
        }
    }
    if (desired_key == old_key)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    Slots next = children->indexed;

    /* Computation / Вычисление и обновление состояния. */
    next.erase(old_key);
    next[desired_key] = object(o);
    if (other && other != o)
        next[old_key] = object(other);
    if (other && other != o)
    {
        FVM_Object *od = 0;
        r = state(other, od);
        if (r != FVM_OK)
            return r;
        od->properties["depth"] = number(old);
        if (od->properties.find("scene_token") != od->properties.end())
            od->properties["scene_token"] = number(0);
    }

    /* Computation / Вычисление и обновление состояния. */
    data->properties["depth"] = number(desired);
    if (data->properties.find("scene_token") != data->properties.end())
        data->properties["scene_token"] = number(0);
    children->indexed.swap(next);
    fvm_value_undefined(out);
    return FVM_OK;
}

/* Compute and return identity. */
FVM_Affine identity()
{
    FVM_Affine m;

    /* Computation / Вычисление и обновление состояния. */
    m.xx = m.yy = 1;
    m.xy = m.yx = m.x0 = m.y0 = 0;
    return m;
}

/* Compute and return multiply. */
FVM_Affine multiply(const FVM_Affine &a, const FVM_Affine &b)
{
    FVM_Affine m;

    /* Computation / Вычисление и обновление состояния. */
    m.xx = a.xx * b.xx + a.xy * b.yx;
    m.xy = a.xx * b.xy + a.xy * b.yy;
    m.yx = a.yx * b.xx + a.yy * b.yx;
    m.yy = a.yx * b.xy + a.yy * b.yy;
    m.x0 = a.xx * b.x0 + a.xy * b.y0 + a.x0;
    m.y0 = a.yx * b.x0 + a.yy * b.y0 + a.y0;
    return m;
}

/* Compose one local affine transform while preserving an authored skew basis. */
FVM_Result local_matrix(FVM_Context *c, FVM_Object *o, FVM_Affine &out)
{
    double x = 0, y = 0, sx = 0, sy = 0, rotation = 0;
    FVM_Result r = numeric(c, o, "_x", x);
    if (r == FVM_OK)
        r = numeric(c, o, "_y", y);
    if (r == FVM_OK)
        r = numeric(c, o, "_xscale", sx);
    if (r == FVM_OK)
        r = numeric(c, o, "_yscale", sy);
    if (r == FVM_OK)
        r = numeric(c, o, "_rotation", rotation);
    if (r != FVM_OK)
        return r;
    FVM_Object *data = 0;
    r = state(o, data);
    if (r != FVM_OK)
        return r;
    double bx = 0, by = 1;
    if (data->properties.find("basis_x") != data->properties.end())
    {
        r = numeric(c, data, "basis_x", bx);
        if (r == FVM_OK)
            r = numeric(c, data, "basis_y", by);
        if (r != FVM_OK)
            return r;
    }
    double radians = std::fmod(rotation, FVM_MOVIECLIP_DEGREES_PER_TURN) * std::acos(-1.0) /
                     FVM_MOVIECLIP_DEGREES_PER_HALF_TURN;
    double cs = std::cos(radians), sn = std::sin(radians);
    out.xx = cs * sx / 100;
    out.yx = sn * sx / 100;
    out.xy = (cs * bx - sn * by) * sy / 100;
    out.yy = (sn * bx + cs * by) * sy / 100;
    out.x0 = x;
    out.y0 = y;
    return FVM_OK;
}

/* Compute a vector length without squaring extremely large or tiny components. */
double vector_length(double x, double y)
{
    x = std::fabs(x);
    y = std::fabs(y);
    if (x < y)
    {
        double temporary = x;
        x = y;
        y = temporary;
    }
    if (!x)
        return 0;
    double ratio = y / x;
    return x * std::sqrt(1 + ratio * ratio);
}

/* Update native display components from an affine matrix without losing reflection or shear. */
FVM_Result apply_matrix(FVM_Object *o, const FVM_Affine &matrix)
{
    const double components[] = {matrix.xx, matrix.yx, matrix.xy, matrix.yy, matrix.x0, matrix.y0};
    for (unsigned int i = 0; i < 6; ++i)
        if (!(components[i] == components[i]) || std::fabs(components[i]) > DBL_MAX)
            return FVM_INVALID_ARGUMENT;
    double sx = vector_length(matrix.xx, matrix.yx);
    double sy = vector_length(matrix.xy, matrix.yy);
    if (sx > DBL_MAX / 100 || sy > DBL_MAX / 100)
        return FVM_INVALID_ARGUMENT;
    double radians = sx   ? std::atan2(matrix.yx, matrix.xx)
                     : sy ? std::atan2(-matrix.xy, matrix.yy)
                          : 0;
    double cs = std::cos(radians), sn = std::sin(radians);
    double bx = sy ? (cs * matrix.xy + sn * matrix.yy) / sy : 0;
    double by = sy ? (-sn * matrix.xy + cs * matrix.yy) / sy : 1;
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    /* [GC:] Stage both maps before publishing, preserving all existing managed edges on failure. */
    fvm_detail::map<fvm_detail::string, FVM_Value> properties = o->properties;
    fvm_detail::map<fvm_detail::string, FVM_Value> private_state = data->properties;
    properties["_x"] = number(matrix.x0);
    properties["_y"] = number(matrix.y0);
    properties["_xscale"] = number(sx * 100);
    properties["_yscale"] = number(sy * 100);
    properties["_rotation"] =
        number(radians * FVM_MOVIECLIP_DEGREES_PER_HALF_TURN / std::acos(-1.0));
    private_state["basis_x"] = number(bx);
    private_state["basis_y"] = number(by);
    o->properties.swap(properties);
    data->properties.swap(private_state);
    return FVM_OK;
}

/* Compose a clip's local transform with every ancestor, rejecting cyclic parent chains. */
FVM_Result matrix(FVM_Context *c, FVM_Object *o, FVM_Affine &out)
{
    out = identity();
    unsigned int levels = 0;
    while (o && levels++ < FVM_MOVIECLIP_MATRIX_MAX_ANCESTORS)
    {
        FVM_Affine local;
        FVM_Result r = local_matrix(c, o, local);
        if (r != FVM_OK)
            return r;
        out = multiply(local, out);
        o = parent(o);
    }
    return o ? FVM_INVALID_ARGUMENT : FVM_OK;
}

/* Compute and return invert. */
bool invert(FVM_Affine &m)
{
    double d = m.xx * m.yy - m.xy * m.yx;
    if (d == 0 || d != d)
        return false;
    FVM_Affine old = m;

    /* Computation / Вычисление и обновление состояния. */
    m.xx = old.yy / d;
    m.xy = -old.xy / d;
    m.yx = -old.yx / d;
    m.yy = old.xx / d;
    m.x0 = -(m.xx * old.x0 + m.xy * old.y0);
    m.y0 = -(m.yx * old.x0 + m.yy * old.y0);
    return true;
}

/* Compute and return playing. */
FVM_Result playing(FVM_Context *, FVM_Object *o, FVM_Value *out, bool enabled)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    data->properties["playing"] = number(enabled ? 1 : 0);
    fvm_value_undefined(out);
    return FVM_OK;
}

/* Compute and return play. */
FVM_Result play(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *v,
                void *)
{
    return playing(c, o, v, true);
}

/* Compute and return stop. */
FVM_Result stop(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *v,
                void *)
{
    return playing(c, o, v, false);
}

/* Compute and return seek. */
FVM_Result seek(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                bool play)
{
    if (!n)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0, *frames = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "frames", frames);
    if (r != FVM_OK)
        return r;
    unsigned long total = 0;
    r = integer_field(data, "total", FVM_MOVIECLIP_MAX_FRAMES, total);
    if (r != FVM_OK || !total)
        return FVM_INVALID_ARGUMENT;
    double desired = 0;
    if (a[0].type == FVM_VALUE_STRING)
    {
        bool found = false;
        for (Slots::iterator i = frames->indexed.begin(); i != frames->indexed.end(); ++i)
        {
            if (i->second.type != FVM_VALUE_OBJECT)
                return FVM_INVALID_ARGUMENT;
            FVM_Value label;
            if (field(i->second.object, "label", label) == FVM_OK &&
                label.type == FVM_VALUE_STRING)
            {
                int order = 0;
                r = fvm_string_order(label, a[0], order);
                if (r != FVM_OK)
                    return r;
                if (!order)
                {
                    desired = i->first;
                    found = true;
                    break;
                }
            }
        }
        if (!found)
            return FVM_NOT_FOUND;
    }
    else
    {
        r = args(c, n, a, 0, 1, desired);
        if (r != FVM_OK)
            return r;
        desired = std::floor(desired);
        if (desired < 1)
            desired = 1;
        if (desired > total)
            desired = total;
    }

    /* Computation / Вычисление и обновление состояния. */
    data->properties["frame"] = number(desired);
    data->properties["playing"] = number(play ? 1 : 0);
    data->properties["entered"] = number(0);
    fvm_value_undefined(out);
    return FVM_OK;
}

/* Compute and return goto play. */
FVM_Result goto_play(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                     FVM_Value *v, void *)
{
    return seek(c, o, n, a, v, true);
}

/* Compute and return goto stop. */
FVM_Result goto_stop(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                     FVM_Value *v, void *)
{
    return seek(c, o, n, a, v, false);
}

/* Compute and return step. */
FVM_Result step(FVM_Context *c, FVM_Object *o, FVM_Value *out, int delta)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    unsigned long frame = 0;
    r = integer_field(data, "frame", FVM_MOVIECLIP_MAX_FRAMES, frame);
    if (r != FVM_OK)
        return r;
    FVM_Value a = number((double)frame + delta);
    return seek(c, o, 1, &a, out, false);
}

/* Compute and return next. */
FVM_Result next(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *v,
                void *)
{
    return step(c, o, v, 1);
}

/* Compute and return previous. */
FVM_Result previous(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *v,
                    void *)
{
    return step(c, o, v, -1);
}

/* Fill the current value. */
FVM_Result fill(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    double rgb = 0, alpha = 100;
    r = args(c, n, a, 0, 0, rgb);
    if (r != FVM_OK)
        return r;
    r = args(c, n, a, 1, 100, alpha);
    if (r != FVM_OK)
        return r;

    /* Computation / Вычисление и обновление состояния. */
    data->properties["fill"] = number(1);
    data->properties["rgb"] = number(packed_rgb(rgb));
    data->properties["alpha"] = number(alpha < 0 ? 0 : alpha > 100 ? 100 : alpha);
    data->properties["active"] = object(0);
    /* [GC:] Release the current ramp without altering gradients retained by older paths. */
    data->properties.erase("gradient");
    /* [GC:] Release only the current bitmap style; completed paths retain their own source. */
    data->properties.erase("bitmap");
    fvm_value_undefined(out);
    return FVM_OK;
}

/* Fill end. */
FVM_Result end_fill(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                    void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;

    /* Computation / Вычисление и обновление состояния. */
    data->properties["fill"] = number(0);
    data->properties["active"] = object(0);
    /* [GC:] Release the current ramp without altering gradients retained by older paths. */
    data->properties.erase("gradient");
    /* [GC:] Release only the current bitmap style; completed paths retain their own source. */
    data->properties.erase("bitmap");
    fvm_value_undefined(out);
    return FVM_OK;
}

/* Compute and return line style. */
FVM_Result line_style(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                      FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    double width = 0, rgb = 0, alpha = 100;
    r = args(c, n, a, 0, 0, width);
    if (r != FVM_OK || width < 0 || width > 255)
        return FVM_INVALID_ARGUMENT;
    r = args(c, n, a, 1, 0, rgb);
    if (r != FVM_OK)
        return r;
    r = args(c, n, a, 2, 100, alpha);
    if (r != FVM_OK)
        return r;

    /* Computation / Вычисление и обновление состояния. */
    data->properties["stroke"] = number(n ? 1 : 0);
    data->properties["width"] = number(width);
    data->properties["lineRgb"] = number(packed_rgb(rgb));
    data->properties["lineAlpha"] = number(alpha < 0 ? 0 : alpha > 100 ? 100 : alpha);
    data->properties["active"] = object(0);
    fvm_value_undefined(out);
    return FVM_OK;
}

/* Compute and return drawing. */
FVM_Result drawing(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                   FVM_Value *out, int verb)
{
    unsigned int count = verb == FVM_PATH_QUADRATIC ? 4 : 2;
    if (n < count)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0, *paths = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "paths", paths);
    if (r != FVM_OK)
        return r;
    double values[4] = {0, 0, 0, 0};
    for (unsigned int i = 0; i < count; ++i)
    {
        r = args(c, n, a, i, 0, values[i]);
        if (r != FVM_OK || std::fabs(values[i]) > FVM_MOVIECLIP_MAX_DRAW_COORDINATE)
            return FVM_INVALID_ARGUMENT;
    }
    FVM_Value active;
    field(data, "active", active);
    FVM_Object *part = active.type == FVM_VALUE_OBJECT ? active.object : 0, *commands = 0;
    if (!part)
    {
        r = fvm_object_create(c, &part);
        if (r != FVM_OK)
            return r;
        r = fvm_object_create(c, &commands);
        if (r != FVM_OK)
            return r;
        part->properties["commands"] = object(commands);
        if (data->properties.find("gradient") != data->properties.end())
        {
            /* [GC:] A completed path retains the immutable ramp chosen when it begins. */
            part->properties["gradient"] = data->properties["gradient"];
        }
        if (data->properties.find("bitmap") != data->properties.end())
        {
            /* [GC:] This path retains the live bitmap and copied matrix chosen when it begins. */
            part->properties["bitmap"] = data->properties["bitmap"];
        }
        const char *style[] = {"fill", "rgb", "alpha", "stroke", "width", "lineRgb", "lineAlpha"};
        for (unsigned int i = 0; i < 7; ++i)
        {
            FVM_Value v;
            if (field(data, style[i], v) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
            part->properties[style[i]] = v;
        }
        if (verb != FVM_PATH_MOVE)
        {
            FVM_Object *start = 0;
            r = fvm_object_create(c, &start);
            if (r != FVM_OK)
                return r;
            start->properties["verb"] = number(FVM_PATH_MOVE);
            FVM_Value x, y;
            if (field(data, "cx", x) != FVM_OK || field(data, "cy", y) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
            start->properties["x0"] = x;
            start->properties["y0"] = y;
            commands->indexed[0] = object(start);
        }
    }
    else
    {
        r = private_object(part, "commands", commands);
        if (r != FVM_OK)
            return r;
    }
    if (commands->indexed.size() >= FVM_MOVIECLIP_MAX_DRAW_COMMANDS ||
        paths->indexed.size() >= FVM_MOVIECLIP_MAX_DRAW_PATHS)
        return FVM_OUT_OF_MEMORY;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *command = 0;
    r = fvm_object_create(c, &command);
    if (r != FVM_OK)
        return r;

    /* Computation / Вычисление и обновление состояния. */
    command->properties["verb"] = number(verb);
    command->properties["x0"] = number(values[0]);
    command->properties["y0"] = number(values[1]);
    if (count == 4)
    {
        command->properties["x1"] = number(values[2]);
        command->properties["y1"] = number(values[3]);
    }
    Slots appended = commands->indexed;
    appended[(unsigned long)appended.size()] = object(command);
    if (active.type != FVM_VALUE_OBJECT)
    {
        Slots additions = paths->indexed;
        additions[(unsigned long)additions.size()] = object(part);
        paths->indexed.swap(additions);
        data->properties["active"] = object(part);
    }

    /* Computation / Вычисление и обновление состояния. */
    commands->indexed.swap(appended);
    data->properties["cx"] = number(values[count - 2]);
    data->properties["cy"] = number(values[count - 1]);
    fvm_value_undefined(out);
    return FVM_OK;
}

/* Compute and return move to. */
FVM_Result move_to(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *v,
                   void *)
{
    return drawing(c, o, n, a, v, FVM_PATH_MOVE);
}

/* Compute and return line to. */
FVM_Result line_to(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *v,
                   void *)
{
    return drawing(c, o, n, a, v, FVM_PATH_LINE);
}

/* Compute and return curve to. */
FVM_Result curve_to(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *v,
                    void *)
{
    return drawing(c, o, n, a, v, FVM_PATH_QUADRATIC);
}

/* Clear the current value. */
FVM_Result clear(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                 void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0, *paths = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "paths", paths);
    if (r != FVM_OK)
        return r;

    /* Computation / Вычисление и обновление состояния. */
    paths->indexed.clear();
    data->properties["active"] = object(0);
    data->properties["cx"] = number(0);
    data->properties["cy"] = number(0);
    data->properties["fill"] = number(0);
    data->properties["stroke"] = number(0);
    data->properties["width"] = number(0);
    data->properties["rgb"] = data->properties["lineRgb"] = number(0);
    data->properties["alpha"] = data->properties["lineAlpha"] = number(100);
    /* [GC:] Clearing graphics releases the active/current ramp after dropping all path edges. */
    data->properties.erase("gradient");
    /* [GC:] Release only the current bitmap style; completed paths retain their own source. */
    data->properties.erase("bitmap");
    fvm_value_undefined(out);
    return FVM_OK;
}

/* Compute and return path. */
FVM_Result path(FVM_Context *c, FVM_Object *part, const FVM_Affine &matrix, FVM_Path **out)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *commands = 0;
    FVM_Result r = private_object(part, "commands", commands);
    if (r != FVM_OK)
        return r;
    r = fvm_path_create(out);
    if (r != FVM_OK)
        return r;
    /* Define Guard struct and its representation. */
    struct Guard
    {
        FVM_Path **path;
        bool active;
        /* Initialize Guard and its owned state. */
        explicit Guard(FVM_Path **p) : path(p), active(true)
        {
        }
        /* Release the resources owned by this Guard instance. */
        ~Guard()
        {
            if (active && *path)
            {
                fvm_path_destroy(*path);
                *path = 0;
            }
        }
    } guard(out);
    for (unsigned long i = 0; i < commands->indexed.size(); ++i)
    {
        Slots::iterator entry = commands->indexed.find(i);
        if (entry == commands->indexed.end() || entry->second.type != FVM_VALUE_OBJECT)
        {
            fvm_path_destroy(*out);
            *out = 0;
            return FVM_INVALID_ARGUMENT;
        }
        FVM_Object *data = entry->second.object;
        unsigned long verb = 0;
        r = integer_field(data, "verb", FVM_PATH_CLOSE, verb);
        if (r != FVM_OK || verb == FVM_PATH_CUBIC)
        {
            fvm_path_destroy(*out);
            *out = 0;
            return FVM_INVALID_ARGUMENT;
        }
        FVM_PathCommand cmd;
        std::memset(&cmd, 0, sizeof(cmd));
        cmd.verb = (FVM_PathVerb)verb;
        r = numeric(c, data, "x0", cmd.x[0]);
        if (r == FVM_OK)
            r = numeric(c, data, "y0", cmd.y[0]);
        if (r == FVM_OK && verb == FVM_PATH_QUADRATIC)
        {
            r = numeric(c, data, "x1", cmd.x[1]);
            if (r == FVM_OK)
                r = numeric(c, data, "y1", cmd.y[1]);
        }
        if (r == FVM_OK)
            r = fvm_path_add(*out, &cmd);
        if (r != FVM_OK)
        {
            fvm_path_destroy(*out);
            *out = 0;
            return r;
        }
    }
    r = fvm_path_transform(*out, &matrix);
    if (r != FVM_OK)
    {
        fvm_path_destroy(*out);
        *out = 0;
    }
    if (r == FVM_OK)
        guard.active = false;
    return r;
}

/* Collect stable geometry snapshots in world space or an accumulated parent-space transform. */
FVM_Result bounds(FVM_Context *c, FVM_Object *o, const FVM_Affine &target, bool stroke,
                  FVM_PathBounds &out, bool &any, unsigned int depth, bool parent_space = false,
                  bool transform_self = true)
{
    if (depth > FVM_MOVIECLIP_GEOMETRY_MAX_DEPTH)
        return FVM_INVALID_ARGUMENT;

    /* [GC:] Matrix/style callbacks cannot collect detached nodes retained by native snapshots. */
    FVM_NativeCallGuard native(c);
    FVM_Object *data = 0, *paths = 0, *children = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "paths", paths);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "children", children);
    if (r != FVM_OK)
        return r;
    /* Capture traversal order before replaceable transform getters can mutate the display list. */
    Slots path_snapshot = paths->indexed;
    Slots child_snapshot = children->indexed;
    FVM_Affine world;
    if (transform_self)
        r = parent_space ? local_matrix(c, o, world) : matrix(c, o, world);
    else
        world = identity();
    if (r != FVM_OK)
        return r;
    world = multiply(target, world);
    for (Slots::iterator i = path_snapshot.begin(); i != path_snapshot.end(); ++i)
    {
        if (i->second.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        FVM_Path *p = 0;
        r = path(c, i->second.object, world, &p);
        if (r != FVM_OK)
            return r;
        OwnedPath ownership(p);
        FVM_PathBounds b;
        r = fvm_path_bounds(p, &b);
        size_t count = fvm_path_count(p);
        if (r != FVM_OK)
            return r;
        if (!count)
            continue;
        FVM_Value line;
        if (stroke && field(i->second.object, "stroke", line) == FVM_OK && line.number)
        {
            double width = 0;
            r = numeric(c, i->second.object, "width", width);
            if (r != FVM_OK)
                return r;
            double scale = vector_length(world.xx, world.yx);
            double other = std::sqrt(world.xy * world.xy + world.yy * world.yy);
            if (other > scale)
                scale = other;
            width *= scale / 2;
            b.x_min -= width;
            b.y_min -= width;
            b.x_max += width;
            b.y_max += width;
        }
        if (!any)
        {
            out = b;
            any = true;
        }
        else
        {
            if (b.x_min < out.x_min)
                out.x_min = b.x_min;
            if (b.y_min < out.y_min)
                out.y_min = b.y_min;
            if (b.x_max > out.x_max)
                out.x_max = b.x_max;
            if (b.y_max > out.y_max)
                out.y_max = b.y_max;
        }
    }
    for (Slots::iterator i = child_snapshot.begin(); i != child_snapshot.end(); ++i)
    {
        if (i->second.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        r = bounds(c, i->second.object, parent_space ? world : target, stroke, out, any, depth + 1, parent_space);
        if (r != FVM_OK)
            return r;
    }
    return FVM_OK;
}

/* Undefined dimension ratios propagate to the final zero-scale fallback without dividing by zero. */
double dimension_ratio(double numerator, double denominator)
{
    return denominator != 0 ? numerator / denominator : std::numeric_limits<double>::quiet_NaN();
}

/* Resize one indexed dimension using untransformed vector bounds and the pinned rotated-scale rule. */
FVM_Result resize_dimension(FVM_Context *c, FVM_Object *clip, bool width, double requested)
{
    /* [GC:] Retain geometry snapshots and staged property edges across descendant transform getters. */
    FVM_NativeCallGuard guard(c);
    FVM_PathBounds extent;
    std::memset(&extent, 0, sizeof(extent));
    bool any = false;
    FVM_Result status = bounds(c, clip, identity(), true, extent, any, 0, true, false);
    if (status != FVM_OK)
        return status;
    FVM_Value x, y, rotation;
    if (field(clip, "_xscale", x) != FVM_OK || field(clip, "_yscale", y) != FVM_OK ||
        field(clip, "_rotation", rotation) != FVM_OK || x.type != FVM_VALUE_NUMBER ||
        y.type != FVM_VALUE_NUMBER || rotation.type != FVM_VALUE_NUMBER)
        return FVM_INVALID_ARGUMENT;
    double primary = width ? extent.x_max - extent.x_min : extent.y_max - extent.y_min;
    double other = width ? extent.y_max - extent.y_min : extent.x_max - extent.x_min;
    double aspect = dimension_ratio(other, primary);
    double target = primary != 0 ? dimension_ratio(requested, primary) : 0;
    double target_other = primary != 0 ? dimension_ratio(requested, other) : 0;
    double old_primary = (width ? x.number : y.number) / FVM_MOVIECLIP_DIMENSION_PERCENT;
    double old_other = (width ? y.number : x.number) / FVM_MOVIECLIP_DIMENSION_PERCENT;
    double radians = std::fmod(rotation.number, FVM_MOVIECLIP_DEGREES_PER_TURN) *
                     std::acos(-1.0) / FVM_MOVIECLIP_DEGREES_PER_HALF_TURN;
    double cs = std::fabs(std::cos(radians)), sn = std::fabs(std::sin(radians));
    double denominator = aspect * cs + sn;
    double changed = dimension_ratio(aspect * (cs * target + sn * target_other),
                                     (cs + aspect * sn) * denominator);
    double retained = dimension_ratio(sn * old_primary + aspect * cs * old_other, denominator);
    if (changed != changed || std::fabs(changed) > DBL_MAX)
        changed = 0;
    if (retained != retained || std::fabs(retained) > DBL_MAX)
        retained = 0;
    if (std::fabs(changed) > DBL_MAX / FVM_MOVIECLIP_DIMENSION_PERCENT ||
        std::fabs(retained) > DBL_MAX / FVM_MOVIECLIP_DIMENSION_PERCENT)
        return FVM_UNSUPPORTED;
    /* [GC:] Publish both intrinsic scales together; OOM leaves the pre-publication state intact. */
    Properties staged = clip->properties;
    staged["_xscale"] = number((width ? changed : retained) * FVM_MOVIECLIP_DIMENSION_PERCENT);
    staged["_yscale"] = number((width ? retained : changed) * FVM_MOVIECLIP_DIMENSION_PERCENT);
    clip->properties.swap(staged);
    return FVM_OK;
}

/* Read bounds. */
FVM_Result get_bounds(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                      FVM_Value *out, bool stroke)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Affine coordinate = identity();
    FVM_Result r = FVM_OK;
    if (n && a[0].type == FVM_VALUE_OBJECT)
    {
        r = matrix(c, a[0].object, coordinate);
        if (r != FVM_OK)
            return r;
        if (!invert(coordinate))
            return FVM_INVALID_ARGUMENT;
    }
    else
    {
        r = matrix(c, o, coordinate);
        if (r != FVM_OK)
            return r;
        if (!invert(coordinate))
            return FVM_INVALID_ARGUMENT;
    }
    FVM_PathBounds b;
    std::memset(&b, 0, sizeof(b));
    bool any = false;
    r = bounds(c, o, coordinate, stroke, b, any, 0);
    if (r != FVM_OK)
        return r;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *result = 0;
    r = fvm_object_create(c, &result);
    if (r != FVM_OK)
        return r;

    /* Computation / Вычисление и обновление состояния. */
    result->properties["xMin"] = number(b.x_min);
    result->properties["yMin"] = number(b.y_min);
    result->properties["xMax"] = number(b.x_max);
    result->properties["yMax"] = number(b.y_max);
    *out = object(result);
    return FVM_OK;
}

/* Compute and return bounds method. */
FVM_Result bounds_method(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                         FVM_Value *v, void *)
{
    return get_bounds(c, o, n, a, v, true);
}

/* Compute and return rect method. */
FVM_Result rect_method(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                       FVM_Value *v, void *)
{
    return get_bounds(c, o, n, a, v, false);
}

/* [GC:] Test copied paint/child edges against a world-space point under the native call barrier. */
FVM_Result shape_hit(FVM_Context *c, FVM_Object *o, double x, double y, bool &yes, unsigned depth)
{
    if (depth > FVM_MOVIECLIP_GEOMETRY_MAX_DEPTH)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0, *paths = 0, *children = 0;
    FVM_Result r = state(o, data);
    if (r == FVM_OK)
        r = private_object(data, "paths", paths);
    if (r == FVM_OK)
        r = private_object(data, "children", children);
    if (r != FVM_OK)
        return r;
    /* [GC:] Native barriers protect the copied geometry edges even if display getters mutate them.
     */
    Slots paints = paths->indexed, descendants = children->indexed;
    FVM_Affine world;
    r = matrix(c, o, world);
    if (r != FVM_OK)
        return r;
    for (Slots::iterator i = paints.begin(); i != paints.end(); ++i)
    {
        if (i->second.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        FVM_Object *paint = i->second.object;
        double fill = 0, stroke = 0, width = 0;
        r = numeric(c, paint, "fill", fill);
        if (r == FVM_OK)
            r = numeric(c, paint, "stroke", stroke);
        if (r != FVM_OK)
            return r;
        if (!fill && !stroke)
            continue;
        FVM_Path *geometry = 0;
        r = path(c, paint, world, &geometry);
        if (r != FVM_OK)
            return r;
        OwnedPath ownership(geometry);
        int hit = 0;
        FVM_Value rule;
        int even_odd = field(paint, "even_odd", rule) == FVM_OK && rule.number ? 1 : 0;
        if (fill)
            r = fvm_path_hit_test(geometry, x, y, 0, even_odd, &hit);
        if (r == FVM_OK && !hit && stroke)
        {
            r = numeric(c, paint, "width", width);
            double scale = vector_length(world.xx, world.yx);
            double other = vector_length(world.xy, world.yy);
            if (other > scale)
                scale = other;
            if (r == FVM_OK)
                r = fvm_path_hit_test(geometry, x, y, width ? width * scale : 1, even_odd, &hit);
        }
        if (r != FVM_OK)
            return r;
        if (hit)
        {
            yes = true;
            return FVM_OK;
        }
    }
    for (Slots::iterator i = descendants.begin(); i != descendants.end(); ++i)
    {
        if (i->second.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        r = shape_hit(c, i->second.object, x, y, yes, depth + 1);
        if (r != FVM_OK || yes)
            return r;
    }
    return FVM_OK;
}

/* Apply ActionScript truthiness to the optional shape flag without invoking user code. */
bool shape_flag(const FVM_Value &value)
{
    if (value.type == FVM_VALUE_BOOLEAN)
        return value.boolean != 0;
    if (value.type == FVM_VALUE_NUMBER)
        return value.number != 0 && value.number == value.number;
    if (value.type == FVM_VALUE_STRING)
        return fvm_string_size(value) != 0;
    return value.type == FVM_VALUE_OBJECT;
}

/* Test native geometry; loaded point coordinates belong to the unlocked root space. */
FVM_Result hit(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
               void *)
{
    FVM_PathBounds b;
    std::memset(&b, 0, sizeof(b));

    /* Initialization / Инициализация локального состояния. */
    bool any = false;
    bool object_form = n == FVM_MOVIECLIP_HIT_OBJECT_ARGUMENTS &&
                       (c->swf_loaded ||
                        a[FVM_MOVIECLIP_HIT_OBJECT_INDEX].type == FVM_VALUE_OBJECT);
    FVM_Result r = FVM_OK;
    if (!c->swf_loaded)
        r = bounds(c, o, identity(), true, b, any, 0);
    else
    {
        /* Reject a borrowed nonclip receiver before any argument can run script. */
        FVM_Object *data = 0;
        r = state(o, data);
    }
    if (r != FVM_OK)
        return r;
    bool yes = false;
    if (object_form)
    {
        FVM_Value target = a[FVM_MOVIECLIP_HIT_OBJECT_INDEX];
        if (c->swf_loaded)
        {
            r = fvm_exec_resolve_clip(c, o, &target, 0, &target);
            if (r != FVM_OK)
                return r;
            if (target.type != FVM_VALUE_OBJECT)
            {
                fvm_value_boolean(out, false);
                return FVM_OK;
            }
            return fvm_exec_clip_hit_object(c, o, target.object, out);
        }
        FVM_PathBounds other;
        std::memset(&other, 0, sizeof(other));
        bool has = false;
        r = bounds(c, target.object, identity(), true, other, has, 0);
        if (r != FVM_OK)
            return r;
        yes = any && has && b.x_min <= other.x_max && other.x_min <= b.x_max &&
              b.y_min <= other.y_max && other.y_min <= b.y_max;
    }
    else
    {
        if (n < FVM_MOVIECLIP_HIT_POINT_ARGUMENTS)
        {
            if (!c->swf_loaded)
                return FVM_INVALID_ARGUMENT;
            fvm_value_boolean(out, false);
            return FVM_OK;
        }
        bool shape =
            n > FVM_MOVIECLIP_HIT_SHAPE_INDEX && shape_flag(a[FVM_MOVIECLIP_HIT_SHAPE_INDEX]);
        double x = 0, y = 0;
        r = args(c, n, a, FVM_MOVIECLIP_HIT_X_INDEX, 0, x);
        if (r != FVM_OK)
            return r;
        r = args(c, n, a, FVM_MOVIECLIP_HIT_Y_INDEX, 0, y);
        if (r != FVM_OK)
            return r;
        if (c->swf_loaded)
        {
            if (n > FVM_MOVIECLIP_HIT_SHAPE_INDEX)
            {
                FVM_Value flag;
                r = fvm_exec_to_boolean(c, a + FVM_MOVIECLIP_HIT_SHAPE_INDEX, &flag);
                if (r != FVM_OK)
                    return r;
                shape = flag.boolean != 0;
            }
            const double finite = std::numeric_limits<double>::max();
            if (!(x >= -finite && x <= finite && y >= -finite && y <= finite))
            {
                fvm_value_boolean(out, 0);
                return FVM_OK;
            }
            r = fvm_movieclip_hit_world_point(c, o, x, y);
            if (r != FVM_OK)
                return r;
            return fvm_exec_clip_hit_point(c, o, x, y, shape, out);
        }
        yes = any && x >= b.x_min && x <= b.x_max && y >= b.y_min && y <= b.y_max;
        if (shape)
        {
            if (!(x >= -FVM_MOVIECLIP_MAX_HIT_COORDINATE && x <= FVM_MOVIECLIP_MAX_HIT_COORDINATE &&
                  y >= -FVM_MOVIECLIP_MAX_HIT_COORDINATE && y <= FVM_MOVIECLIP_MAX_HIT_COORDINATE))
            {
                fvm_value_boolean(out, 0);
                return FVM_OK;
            }
            yes = false;
            r = shape_hit(c, o, x, y, yes, 0);
            if (r != FVM_OK)
                return r;
        }
    }
    fvm_value_boolean(out, yes);
    return FVM_OK;
}

/* Linear RGBA transform inherited through the display-object hierarchy. */
struct ColorAffine
{
    double multiply[4]; /* Per-channel red/green/blue/alpha multipliers. */
    double add[4];      /* Per-channel red/green/blue/alpha offsets. */
    /* Initialize a neutral parent transform for root rendering. */
    ColorAffine()
    {
        for (unsigned int i = 0; i < 4; ++i)
        {
            multiply[i] = 1;
            add[i] = 0;
        }
    }
};

/* Compose authored color terms and mutable alpha with the parent's RGBA transform. */
FVM_Result display_color(FVM_Context *c, FVM_Object *o, FVM_Object *data, const ColorAffine &parent,
                         ColorAffine &out)
{
    ColorAffine local;
    double alpha = 100;
    FVM_Result r = numeric(c, o, "_alpha", alpha);
    if (r != FVM_OK)
        return r;
    /* Preserve the affine alpha coefficient; clamp the final rendered channel instead. */
    local.multiply[3] = alpha / 100;
    if (data->properties.find("swf_color") != data->properties.end())
    {
        FVM_Object *record = 0;
        r = private_object(data, "swf_color", record);
        if (r != FVM_OK)
            return r;
        const char *multipliers[] = {"rm", "gm", "bm"};
        const char *offsets[] = {"ra", "ga", "ba", "aa"};
        for (unsigned int i = 0; i < 3; ++i)
        {
            r = numeric(c, record, multipliers[i], local.multiply[i]);
            if (r != FVM_OK)
                return r;
        }
        for (unsigned int i = 0; i < 4; ++i)
        {
            r = numeric(c, record, offsets[i], local.add[i]);
            if (r != FVM_OK)
                return r;
        }
    }
    for (unsigned int i = 0; i < 4; ++i)
    {
        out.multiply[i] = parent.multiply[i] * local.multiply[i];
        out.add[i] = parent.multiply[i] * local.add[i] + parent.add[i];
    }
    return FVM_OK;
}

/* Compose ancestor color transforms when the host renders a subtree in world coordinates. */
FVM_Result ancestor_color(FVM_Context *c, FVM_Object *o, ColorAffine &out)
{
    /* [GC:] Borrow ancestor edges under the caller's render barrier while getters can run. */
    fvm_detail::vector<FVM_Object *> ancestors;
    /* [GC:] Traverse protected parent edges without adding persistent host roots. */
    for (FVM_Object *p = parent(o); p; p = parent(p))
    {
        if (p->owner != c || ancestors.size() >= 64 || p == o)
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Retain the fixed traversal order until all inherited color terms are read. */
        ancestors.push_back(p);
    }
    for (size_t i = ancestors.size(); i; --i)
    {
        /* [GC:] Validate this ancestor's private display-state edge before reading its color. */
        FVM_Object *data = 0;
        FVM_Result r = state(ancestors[i - 1], data);
        if (r != FVM_OK)
            return r;
        ColorAffine next;
        r = display_color(c, ancestors[i - 1], data, out, next);
        if (r != FVM_OK)
            return r;
        out = next;
    }
    return FVM_OK;
}

/* Clamp a computed color component before converting it to an unsigned byte. */
unsigned char component(double value)
{
    if (!(value > 0))
        return 0;
    if (value >= 255)
        return 255;
    return (unsigned char)(value + 0.5);
}

/* Apply the composed transform to one straight-alpha vector paint color. */
FVM_RasterColor color(double rgb, double alpha, const ColorAffine &transform)
{
    unsigned long packed = packed_rgb(rgb);
    const double values[] = {(double)((packed >> 16) & 255), (double)((packed >> 8) & 255),
                             (double)(packed & 255), alpha * 255 / 100};
    FVM_RasterColor result;
    result.r = component(values[0] * transform.multiply[0] + transform.add[0]);
    result.g = component(values[1] * transform.multiply[1] + transform.add[1]);
    result.b = component(values[2] * transform.multiply[2] + transform.add[2]);
    result.a = component(values[3] * transform.multiply[3] + transform.add[3]);
    return result;
}

/* Apply render-only bitmap position snapping without modifying script-visible transforms. */
FVM_Result bitmap_snap(FVM_Object *data, FVM_Affine &world)
{
    Properties::iterator i = data->properties.find("bitmap_attachment");
    if (i == data->properties.end())
        return FVM_OK;
    if (i->second.type != FVM_VALUE_NUMBER ||
        !(i->second.number == 0 || i->second.number == 1 || i->second.number == 2))
        return FVM_INVALID_ARGUMENT;
    bool snap = i->second.number == 1 || (i->second.number == 0 && world.xx == 1 && world.yy == 1 &&
                                          world.xy == 0 && world.yx == 0);
    if (snap)
    {
        world.x0 = std::floor(world.x0 + 0.5);
        world.y0 = std::floor(world.y0 + 0.5);
    }
    return FVM_OK;
}

/* Render the current value. */
FVM_Result render(FVM_Context *c, FVM_Object *o, FVM_RasterSurface *surface,
                  const ColorAffine &parent_color, unsigned int depth,
                  const FVM_Affine *drawing_base = 0, FVM_RasterSurface *coverage = 0,
                  const FVM_Affine *viewport = 0)
{
    if (depth > 64)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0, *paths = 0, *children = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    if (!drawing_base || depth)
    {
        double visible = 0;
        r = numeric(c, o, "_visible", visible);
        if (r != FVM_OK)
            return r;
        if (!visible)
            return FVM_OK;
    }
    ColorAffine transform = parent_color;
    if (!drawing_base || depth)
    {
        r = display_color(c, o, data, parent_color, transform);
        if (r != FVM_OK)
            return r;
    }
    r = private_object(data, "paths", paths);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "children", children);
    if (r != FVM_OK)
        return r;
    FVM_Affine world;
    if (drawing_base)
    {
        world = *drawing_base;
        if (depth)
        {
            FVM_Affine local;
            r = local_matrix(c, o, local);
            if (r != FVM_OK)
                return r;
            world = multiply(world, local);
        }
    }
    else
    {
        r = matrix(c, o, world);
        if (r != FVM_OK)
            return r;
    }
    /* Host viewport is applied after the script world matrix, once per node.
     * Normal child recursion recomputes authored world matrices, not this projected copy. */
    if (viewport)
        world = multiply(*viewport, world);
    if (!drawing_base || depth)
    {
        r = bitmap_snap(data, world);
        if (r != FVM_OK)
            return r;
    }
    for (Slots::iterator i = paths->indexed.begin(); i != paths->indexed.end(); ++i)
    {
        if (i->second.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        FVM_Object *part = i->second.object;
        FVM_Path *p = 0;
        r = path(c, part, world, &p);
        if (r != FVM_OK)
            return r;
        OwnedPath ownership(p);
        double fill = 0, stroke = 0, rgb = 0, a = 0, width = 0;
        r = numeric(c, part, "fill", fill);
        if (r == FVM_OK)
            r = numeric(c, part, "stroke", stroke);
        if (r == FVM_OK && fill)
        {
            r = numeric(c, part, "rgb", rgb);
            if (r == FVM_OK)
                r = numeric(c, part, "alpha", a);
            if (r == FVM_OK)
            {
                FVM_Value rule;
                int even_odd = field(part, "even_odd", rule) == FVM_OK &&
                               rule.type == FVM_VALUE_NUMBER && rule.number != 0;
                if (part->properties.find("bitmap") != part->properties.end())
                    r = fvm_movieclip_bitmap_draw(c, part, world, p, surface, transform.multiply,
                                                  transform.add, even_odd);
                else if (part->properties.find("gradient") != part->properties.end())
                    r = fvm_movieclip_gradient_draw(c, part, world, p, surface, transform.multiply,
                                                    transform.add, even_odd);
                else
                    r = fvm_path_draw_quality(p, surface, color(rgb, a, transform), 0, even_odd, c->stage_quality != FVM_STAGE_QUALITY_LOW);
                if (r == FVM_OK && coverage)
                {
                    FVM_RasterColor opaque = {255, 255, 255, 255};
                    r = fvm_path_draw_quality(p, coverage, opaque, 0, even_odd, c->stage_quality != FVM_STAGE_QUALITY_LOW);
                }
            }
        }
        if (r == FVM_OK && stroke)
        {
            r = numeric(c, part, "lineRgb", rgb);
            if (r == FVM_OK)
                r = numeric(c, part, "lineAlpha", a);
            if (r == FVM_OK)
                r = numeric(c, part, "width", width);
            double scale = vector_length(world.xx, world.yx);
            if (r == FVM_OK)
                r = fvm_path_draw_quality(p, surface, color(rgb, a, transform), width ? width * scale : 1,
                                  0, c->stage_quality != FVM_STAGE_QUALITY_LOW);
            if (r == FVM_OK && coverage)
            {
                FVM_RasterColor opaque = {255, 255, 255, 255};
                r = fvm_path_draw_quality(p, coverage, opaque, width ? width * scale : 1, 0, c->stage_quality != FVM_STAGE_QUALITY_LOW);
            }
        }
        if (r != FVM_OK)
            return r;
    }
    /* Depth order is independent of hash-table insertion order. */
    fvm_detail::vector<unsigned long> order;
    for (Slots::iterator i = children->indexed.begin(); i != children->indexed.end(); ++i)
        order.push_back(i->first);
    for (size_t i = 1; i < order.size(); ++i)
    {
        unsigned long v = order[i];
        size_t j = i;
        while (j && order[j - 1] > v)
        {
            order[j] = order[j - 1];
            --j;
        }
        order[j] = v;
    }
    for (size_t i = 0; i < order.size(); ++i)
    {
        FVM_Value child = children->indexed.find(order[i])->second;
        if (child.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        r = render(c, child.object, surface, transform, depth + 1, drawing_base ? &world : 0,
                   coverage, viewport);
        if (r != FVM_OK)
            return r;
    }
    return FVM_OK;
}

/* Decode one stored action block and execute it with this MovieClip as receiver. */
FVM_Result execute_frame_block(FVM_Context *c, FVM_Object *o, const FVM_Value &code)
{
    if (code.type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::vector<unsigned char> bytes;
    size_t size = fvm_string_size(code);
    if (size % 2 || size > 131072)
        return FVM_INVALID_ARGUMENT;
    for (size_t i = 0; i < size; i += 2)
    {
        unsigned int a = code.string[i] >= 'a' ? code.string[i] - 'a' + 10 : code.string[i] - '0';
        unsigned int b =
            code.string[i + 1] >= 'a' ? code.string[i + 1] - 'a' + 10 : code.string[i + 1] - '0';
        if (a > 15 || b > 15)
            return FVM_INVALID_ARGUMENT;
        bytes.push_back((unsigned char)(a * 16 + b));
    }
    return fvm_avm1_execute_block_receiver(c, bytes.begin(), bytes.size(), o);
}

/* Run source-library init blocks before placement/frame actions, once per graph.
 * Progress is private context state, so transferred libraries initialize afresh.
 * A failed block is not retried: its script side effects cannot be rolled back. */
FVM_Result run_initializers(FVM_Context *c, FVM_Object *data)
{
    if (data->properties.find("library") == data->properties.end())
        return FVM_OK;
    FVM_Value holder;
    FVM_Result r = field(data, "library", holder);
    if (r != FVM_OK)
        return r;
    if (holder.type == FVM_VALUE_UNDEFINED)
        return FVM_OK;
    if (holder.type != FVM_VALUE_OBJECT || !holder.object)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *library = holder.object;
    /* Imported graphs from earlier versions contain no initializer metadata. */
    if (library->properties.find("initializers") == library->properties.end())
        return FVM_OK;
    r = field(library, "initializers", holder);
    if (r != FVM_OK || holder.type == FVM_VALUE_UNDEFINED)
        return r;
    if (holder.type != FVM_VALUE_OBJECT || !holder.object)
        return FVM_INVALID_ARGUMENT;
    if (library->swf_initializer_running)
        return FVM_OK;
    FVM_Value source;
    r = field(library, "root", source);
    if (r != FVM_OK || source.type != FVM_VALUE_OBJECT || !source.object ||
        source.object->owner != c || source.object->resource_kind != CLIP)
        return FVM_INVALID_ARGUMENT;
    /* Restore the reentry guard on every interpreter/exception/OOM result. */
    struct InitScope
    {
        FVM_Object *library; /* Source library protected by the owning clip's roots. */
        /* Enter a source-library initializer sequence. */
        explicit InitScope(FVM_Object *value) : library(value)
        {
            library->swf_initializer_running = true;
        }
        /* Leave initialization on success or failure. */
        ~InitScope()
        {
            library->swf_initializer_running = false;
        }
    } scope(library);
    /* [GC:] Snapshot retained strings before callbacks can alter library properties. */
    Slots blocks = holder.object->indexed;
    if (blocks.size() > ULONG_MAX)
        return FVM_INVALID_ARGUMENT;
    for (unsigned long index = library->swf_initializer_next; index < blocks.size(); ++index)
    {
        Slots::iterator block = blocks.find(index);
        if (block == blocks.end() || index == ULONG_MAX)
            return FVM_INVALID_ARGUMENT;
        library->swf_initializer_next = index + 1;
        r = execute_frame_block(c, source.object, block->second);
        if (r != FVM_OK)
            return r;
    }
    return FVM_OK;
}

/* Advance one timeline tick, execute its independent action blocks, then visit children. */
FVM_Result advance(FVM_Context *c, FVM_Object *o, unsigned int depth)
{
    if (static_art(o))
        return FVM_OK;
    if (depth > 64)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *data = 0, *frames = 0, *children = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "frames", frames);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "children", children);
    if (r != FVM_OK)
        return r;
    unsigned long current = 0, total = 0;
    r = integer_field(data, "frame", FVM_MOVIECLIP_MAX_FRAMES, current);
    if (r != FVM_OK)
        return r;
    r = integer_field(data, "total", FVM_MOVIECLIP_MAX_FRAMES, total);
    if (r != FVM_OK || !total || current < 1 || current > total)
        return FVM_INVALID_ARGUMENT;
    r = run_initializers(c, data);
    if (r != FVM_OK)
        return r;
    FVM_Value entered, playing;

    /* Computation / Вычисление и обновление состояния. */
    field(data, "entered", entered);
    field(data, "playing", playing);
    if (!entered.number || (playing.number && total > 1))
    {
        if (entered.number)
        {
            current = current == total ? 1 : current + 1;
            data->properties["frame"] = number(current);
        }
        bool loop = entered.number && current == 1;
        r = fvm_movieclip_scene_run(c, o, current, loop);
        if (r != FVM_OK)
            return r;
        data->properties["entered"] = number(1);
        Slots::iterator entry = frames->indexed.find(current);
        if (entry != frames->indexed.end())
        {
            if (entry->second.type != FVM_VALUE_OBJECT)
                return FVM_INVALID_ARGUMENT;
            FVM_Value code;
            r = field(entry->second.object, "code", code);
            if (r != FVM_OK || code.type != FVM_VALUE_STRING)
                return FVM_INVALID_ARGUMENT;
            FVM_Value segments;
            r = field(entry->second.object, "segments", segments);
            if (r == FVM_OK && segments.type == FVM_VALUE_OBJECT)
            {
                /* [GC:] Copy the managed block values before script callbacks can change state. */
                Slots blocks = segments.object->indexed;
                for (Slots::iterator i = blocks.begin(); i != blocks.end(); ++i)
                {
                    r = execute_frame_block(c, o, i->second);
                    if (r != FVM_OK)
                        return r;
                }
            }
            else if (entry->second.object->properties.find("segments") ==
                     entry->second.object->properties.end())
                r = execute_frame_block(c, o, code);
            else
                return FVM_INVALID_ARGUMENT;
            if (r != FVM_OK)
                return r;
        }
    }
    FVM_Value handler;
    r = fvm_object_get(c, o, "onEnterFrame", &handler);
    if (r != FVM_OK)
        return r;
    if (handler.type == FVM_VALUE_OBJECT && handler.object->invocable.callback)
    {
        FVM_Value result;
        r = fvm_function_invoke(c, handler.object, o, 0, 0, &result);
        if (r != FVM_OK)
            return r;
    }
    Slots snapshot = children->indexed;
    for (Slots::iterator i = snapshot.begin(); i != snapshot.end(); ++i)
    {
        if (i->second.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        if (parent(i->second.object) != o)
            continue;
        r = advance(c, i->second.object, depth + 1);
        if (r != FVM_OK)
            return r;
    }
    return FVM_OK;
}

/* Compute and return constructor. */
FVM_Result constructor(FVM_Context *c, FVM_Object *, unsigned int, const FVM_Value *,
                       FVM_Value *out, void *ud)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value proto;
    FVM_Result r = fvm_object_get(c, (FVM_Object *)ud, "prototype", &proto);
    if (r != FVM_OK)
        return r;
    if (proto.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *o = 0;
    r = make(c, "", FVM_MOVIECLIP_DEFAULT_FRAMES, proto.object, &o);
    if (r == FVM_OK)
        *out = object(o);
    return r;
}

/* [GC:] Attach a live bitmap as a hidden static display node at the requested signed depth. */
FVM_Result attach_bitmap(FVM_Context *c, FVM_Object *parent_clip, unsigned n,
                         const FVM_Value *args_in, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (n < 2 || args_in[0].type != FVM_VALUE_OBJECT || !args_in[0].object ||
        args_in[0].object->owner != c)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Keep the texture, parent and unpublished node alive across coercions and allocations.
     */
    FVM_NativeCallGuard guard(c);
    double depth = 0;
    FVM_Result r = args(c, n, args_in, 1, 0, depth);
    if (r != FVM_OK)
        return r;
    if (depth < FVM_MOVIECLIP_MIN_PUBLIC_DEPTH || depth > FVM_MOVIECLIP_MAX_PUBLIC_DEPTH ||
        depth != std::floor(depth))
        return FVM_INVALID_ARGUMENT;
    unsigned snapping = 0;
    if (n > 2 && args_in[2].type != FVM_VALUE_UNDEFINED)
    {
        FVM_Value mode;
        r = fvm_value_to_string(c, args_in + 2, &mode);
        if (r != FVM_OK)
            return r;
        size_t length = fvm_string_size(mode);
        if (length == 6 && !std::memcmp(mode.string, "always", 6))
            snapping = 1;
        else if (length == 5 && !std::memcmp(mode.string, "never", 5))
            snapping = 2;
        else if (!(length == 4 && !std::memcmp(mode.string, "auto", 4)))
            return FVM_INVALID_ARGUMENT;
    }
    /* [GC:] Resolve the texture after user-controlled option/depth coercions can dispose it. */
    FVM_BitmapState *pixels = fvm_bitmap_pixels(args_in[0].object);
    if (!pixels || pixels->disposed)
        return FVM_INVALID_ARGUMENT;
    unsigned width = pixels->width, height = pixels->height;
    bool smoothing = n > 3 && shape_flag(args_in[3]);
    /* [GC:] The unpublished static node inherits the parent's transported MovieClip prototype. */
    FVM_Object *node = 0;
    r = make(c, "", 1, parent_clip->prototype, &node);
    if (r != FVM_OK)
        return r;
    /* [GC:] The node owns its private state even before the parent's child table adopts it. */
    FVM_Object *data = node->resource_result.object;
    data->properties["static_art"] = number(1);
    data->properties["bitmap_attachment"] = number(snapping);
    data->properties["playing"] = number(0);
    FVM_Value fill_args[4];
    /* [GC:] Supply the same managed source instance rather than copying its pixel allocation. */
    fill_args[0] = args_in[0];
    fvm_value_undefined(fill_args + 1);
    fvm_value_boolean(fill_args + 2, 0);
    fvm_value_boolean(fill_args + 3, smoothing);
    FVM_Value result;
    r = fvm_movieclip_bitmap_begin(c, node, 4, fill_args, &result, 0);
    if (r != FVM_OK)
        return r;
    const double vertices[] = {
        0, 0, (double)width, 0, (double)width, (double)height, 0, (double)height, 0, 0};
    for (unsigned i = 0; i < 10; i += 2)
    {
        FVM_Value vertex[2];
        fvm_value_number(vertex, vertices[i]);
        fvm_value_number(vertex + 1, vertices[i + 1]);
        r = drawing(c, node, 2, vertex, &result, i ? FVM_PATH_LINE : FVM_PATH_MOVE);
        if (r != FVM_OK)
            return r;
    }
    r = end_fill(c, node, 0, 0, &result, 0);
    if (r != FVM_OK)
        return r;
    /* [GC:] Publish only the fully constructed hidden node, replacing the old depth atomically. */
    return publish_child(c, parent_clip, node, "", (long)depth);
}

/* Define Method struct and its representation. */
struct Method
{
    const char *name;
    FVM_NativeFunction callback;
};
const Method methods[] = {{"toString", clip_string},
                          {"getDepth", get_depth},
                          {"getNextHighestDepth", next_depth},
                          {"getInstanceAtDepth", at_depth},
                          {"createEmptyMovieClip", create_child},
                          {"removeMovieClip", remove},
                          {"swapDepths", swap_depths},
                          {"localToGlobal", fvm_movieclip_local_global},
                          {"globalToLocal", fvm_movieclip_global_local},
                          {"play", play},
                          {"stop", stop},
                          {"gotoAndPlay", goto_play},
                          {"gotoAndStop", goto_stop},
                          {"nextFrame", next},
                          {"prevFrame", previous},
                          {"beginFill", fill},
                          {"endFill", end_fill},
                          {"lineStyle", line_style},
                          {"moveTo", move_to},
                          {"lineTo", line_to},
                          {"curveTo", curve_to},
                          {"clear", clear},
                          {"getBounds", bounds_method},
                          {"getRect", rect_method},
                          {"hitTest", hit},
                          {"attachMovie", attach_movie},
                          {"duplicateMovieClip", duplicate_movie},
                          {"beginGradientFill", fvm_movieclip_gradient_begin},
                          {"beginBitmapFill", fvm_movieclip_bitmap_begin},
                          {"attachBitmap", attach_bitmap},
                          {"startDrag", fvm_movieclip_start_drag},
                          {"stopDrag", fvm_movieclip_stop_drag}};
} // namespace

/* Compose intrinsic world geometry under the caller's root/native-barrier scope. */
FVM_Result fvm_movieclip_world_matrix(FVM_Context *c, FVM_Object *clip, FVM_Affine &out)
{
    return matrix(c, clip, out);
}

/* Bridge explicit rooted operands to the intrinsic construction routine, without method lookup. */
FVM_Result fvm_movieclip_duplicate_instance(FVM_Context *c, FVM_Object *clip,
                                            const FVM_Value *name, const FVM_Value *depth,
                                            const FVM_Value *initial, FVM_Value *out)
{
    FVM_Value values[FVM_MOVIECLIP_DUPLICATE_ARGUMENT_CAPACITY];
    values[FVM_MOVIECLIP_DUPLICATE_NAME_INDEX] = *name;
    values[FVM_MOVIECLIP_DUPLICATE_DEPTH_INDEX] = *depth;
    if (initial)
        values[FVM_MOVIECLIP_DUPLICATE_INITIAL_INDEX] = *initial;
    return duplicate_instance(c, clip,
                              initial ? FVM_MOVIECLIP_DUPLICATE_ARGUMENT_CAPACITY
                                      : FVM_MOVIECLIP_DUPLICATE_REQUIRED_ARGUMENTS,
                              values, out, 0);
}

/* Read world-space geometry; callers retain the instance and block collection. */
FVM_Result fvm_movieclip_world_bounds(FVM_Context *c, FVM_Object *clip, FVM_PathBounds &out,
                                      bool &any)
{
    return bounds(c, clip, identity(), true, out, any, 0);
}

/* Read painted world-space geometry under the caller's collector barrier. */
FVM_Result fvm_movieclip_world_shape_hit(FVM_Context *c, FVM_Object *clip, double x, double y,
                                         bool &yes)
{
    return shape_hit(c, clip, x, y, yes, 0);
}

/* [GC:] Expose the validated managed state without publishing a new host ownership claim. */
FVM_Result fvm_movieclip_scene_state(FVM_Object *o, FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    return state(o, *out);
}

/* [GC:] Normalize the parent's child slot table before direct SWF depth lookup. */
FVM_Result fvm_movieclip_scene_children(FVM_Object *o, FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r == FVM_OK)
        r = private_object(data, "children", *out);
    if (r == FVM_OK)
        r = ensure_slot_bias(data, *out);
    return r;
}

/* Apply matrix/color/ratio/name fields supplied by one composed or incremental placement. */
FVM_Result fvm_movieclip_scene_update(FVM_Context *c, FVM_Object *o, FVM_Object *placement)
{
    if (!c || !o || o->owner != c || !placement || placement->owner != c)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    if (placement->properties.find("matrix") != placement->properties.end())
    {
        FVM_Object *record = 0;
        r = private_object(placement, "matrix", record);
        if (r != FVM_OK)
            return r;
        FVM_Affine matrix;
        r = numeric(c, record, "xx", matrix.xx);
        if (r == FVM_OK)
            r = numeric(c, record, "yx", matrix.yx);
        if (r == FVM_OK)
            r = numeric(c, record, "xy", matrix.xy);
        if (r == FVM_OK)
            r = numeric(c, record, "yy", matrix.yy);
        if (r == FVM_OK)
            r = numeric(c, record, "x0", matrix.x0);
        if (r == FVM_OK)
            r = numeric(c, record, "y0", matrix.y0);
        if (r == FVM_OK)
            r = apply_matrix(o, matrix);
        if (r != FVM_OK)
            return r;
    }
    if (placement->properties.find("color") != placement->properties.end())
    {
        FVM_Object *color = 0;
        r = private_object(placement, "color", color);
        if (r != FVM_OK)
            return r;
        double alpha = 1;
        r = numeric(c, color, "am", alpha);
        if (r != FVM_OK)
            return r;
        /* [GC:] Retain the immutable color record; alpha remains a mutable display property. */
        data->properties["swf_color"] = object(color);
        o->properties["_alpha"] = number(alpha * 100);
    }
    if (placement->properties.find("ratio") != placement->properties.end())
        data->properties["ratio"] = placement->properties["ratio"];
    if (!static_art(o) && placement->properties.find("name") != placement->properties.end())
    {
        FVM_Value name = placement->properties["name"];
        if (name.type != FVM_VALUE_STRING || std::memchr(name.string, 0, fvm_string_size(name)))
            return FVM_INVALID_ARGUMENT;
        FVM_Value old_name;
        r = field(o, "_name", old_name);
        if (r != FVM_OK)
            return r;
        FVM_Object *p = parent(o);
        fvm_detail::map<fvm_detail::string, FVM_Value> properties = o->properties;
        properties["_name"] = name;
        if (p)
        {
            /* [GC:] Stage bindings before publishing; names reference the same live child. */
            fvm_detail::map<fvm_detail::string, FVM_Value> bindings = p->properties;
            if (old_name.type == FVM_VALUE_STRING)
            {
                /* Match the entire previous name, including embedded NUL bytes. */
                fvm_detail::string binding(old_name.string, fvm_string_size(old_name));
                fvm_detail::map<fvm_detail::string, FVM_Value>::iterator old =
                    bindings.find(binding);
                if (old != bindings.end() && old->second.type == FVM_VALUE_OBJECT &&
                    old->second.object == o)
                    bindings.erase(binding);
            }
            bindings[name.string] = object(o);
            p->properties.swap(bindings);
        }
        o->properties.swap(properties);
    }
    return FVM_OK;
}

/* Instantiate one timeline symbol, publish its pose/identity, then invoke the selected class. */
FVM_Result fvm_movieclip_scene_insert(FVM_Context *c, FVM_Object *p, FVM_Object *placement,
                                      FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || !p || p->owner != c || !placement || placement->owner != c)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *pd = 0, *library = 0, *symbol = 0, *frames = 0;
    FVM_Result r = state(p, pd);
    if (r == FVM_OK)
        r = private_object(pd, "library", library);
    if (r == FVM_OK)
        r = private_object(placement, "symbol", symbol);
    if (r == FVM_OK)
        r = private_object(symbol, "frames", frames);
    if (r != FVM_OK)
        return r;
    unsigned long total = 0, depth = 0, token = 0;
    r = integer_field(symbol, "total", FVM_MOVIECLIP_MAX_FRAMES, total);
    if (r == FVM_OK)
        r = integer_field(placement, "depth", FVM_MOVIECLIP_MAX_AUTHORED_DEPTH, depth);
    if (r == FVM_OK)
        r = integer_field(placement, "token", 0xffffffffUL, token);
    if (r != FVM_OK || !total || !depth || !token)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *proto = 0, *constructor = 0;
    r = private_object(library, "prototype", proto);
    if (r != FVM_OK)
        return r;
    FVM_Value linkage;
    bool art = shape_symbol(symbol);
    bool exported = !art && symbol->properties.find("linkage") != symbol->properties.end();
    if (exported)
    {
        r = field(symbol, "linkage", linkage);
        if (r != FVM_OK || linkage.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
        r = fvm_movieclip_sprite_class(c, library, linkage.string, proto, constructor);
        if (r != FVM_OK)
            return r;
    }
    FVM_Value name;
    if (art)
        fvm_value_string(&name, "");
    else if (placement->properties.find("name") != placement->properties.end())
    {
        r = field(placement, "name", name);
        if (r != FVM_OK || name.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
    }
    else
    {
        char generated[32];
        std::sprintf(generated, "instance%lu", token);
        FVM_Object *holder = 0;
        r = fvm_object_create(c, &holder);
        if (r != FVM_OK)
            return r;
        fvm_value_string(&name, generated);
        r = fvm_object_set(c, holder, "name", &name);
        if (r == FVM_OK)
            r = field(holder, "name", name);
        if (r != FVM_OK)
            return r;
    }
    FVM_Object *child = 0, *child_frames = 0;
    r = make(c, name.string, (unsigned int)total, art ? 0 : proto, &child);
    if (r == FVM_OK)
        r = private_object(child->resource_result.object, "frames", child_frames);
    if (r != FVM_OK)
        return r;
    /* [GC:] The instance owns its frame table and retains its source library/definition. */
    child_frames->indexed = frames->indexed;
    FVM_Object *data = child->resource_result.object;
    data->properties["library"] = object(library);
    data->properties["scene_symbol"] = object(symbol);
    data->properties["scene_token"] = number(token);
    if (art)
    {
        FVM_Object *paths = 0;
        r = private_object(symbol, "paths", paths);
        if (r != FVM_OK)
            return r;
        /* [GC:] Static nodes retain immutable contours through their source definition. */
        data->properties["paths"] = object(paths);
        data->properties["static_art"] = number(1);
        data->properties["playing"] = number(0);
    }
    if (exported)
    {
        r = fvm_object_set(c, data, "linkage", &linkage);
        if (r != FVM_OK)
            return r;
    }
    r = fvm_movieclip_scene_update(c, child, placement);
    if (r == FVM_OK)
        r = publish_child(c, p, child, name.string, (long)depth - FVM_MOVIECLIP_SWF_DEPTH_OFFSET);
    if (r != FVM_OK)
        return r;
    FVM_Value value;
    r = finish_instance(c, child, constructor, 0, &value);
    if (r == FVM_OK)
        *out = child;
    return r;
}

/* Replace static art in place; a MovieClip retains its original character and timeline. */
FVM_Result fvm_movieclip_scene_replace(FVM_Context *c, FVM_Object *o, FVM_Object *placement)
{
    if (!c || !o || o->owner != c || !placement || placement->owner != c)
        return FVM_INVALID_ARGUMENT;
    if (static_art(o))
    {
        FVM_Object *symbol = 0, *paths = 0;
        FVM_Result r = private_object(placement, "symbol", symbol);
        if (r != FVM_OK)
            return r;
        if (shape_symbol(symbol))
        {
            r = private_object(symbol, "paths", paths);
            if (r != FVM_OK)
                return r;
            FVM_Value token;
            r = field(placement, "token", token);
            if (r != FVM_OK)
                return r;
            FVM_Object *data = o->resource_result.object;
            /* [GC:] Stage the resource edges before publishing the replacement geometry. */
            fvm_detail::map<fvm_detail::string, FVM_Value> next = data->properties;
            /* [GC:] Adopt the new immutable path collection while retaining display identity. */
            next["paths"] = object(paths);
            /* [GC:] Keep the new definition reachable for later rewind and graph transport. */
            next["scene_symbol"] = object(symbol);
            next["scene_token"] = token;
            data->properties.swap(next);
        }
    }
    return fvm_movieclip_scene_update(c, o, placement);
}


/* Read intrinsic fields independently from replaceable named property accessors. */
FVM_Result fvm_movieclip_get_action_property(FVM_Context *c, FVM_Object *clip,
                                            unsigned int index, FVM_Value *out)
{
    if (!c || !clip || clip->owner != c || !out)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0;
    FVM_Result status = state(clip, data);
    if (status != FVM_OK)
        return status;
    if (index == FVM_AVM1_PROPERTY_XMOUSE || index == FVM_AVM1_PROPERTY_YMOUSE)
    {
        FVM_Value x, y;
        status = fvm_exec_clip_pointer(c, clip, &x, &y);
        if (status == FVM_OK)
            *out = index == FVM_AVM1_PROPERTY_XMOUSE ? x : y;
        return status;
    }
    if (index == FVM_AVM1_PROPERTY_WIDTH || index == FVM_AVM1_PROPERTY_HEIGHT)
    {
        FVM_PathBounds extent;
        std::memset(&extent, 0, sizeof(extent));
        bool any = false;
        status = bounds(c, clip, identity(), true, extent, any, 0, true);
        if (status != FVM_OK)
            return status;
        fvm_value_number(out, index == FVM_AVM1_PROPERTY_WIDTH ? extent.x_max - extent.x_min
                                                              : extent.y_max - extent.y_min);
        return FVM_OK;
    }
    if (index == FVM_AVM1_PROPERTY_HIGH_QUALITY || index == FVM_AVM1_PROPERTY_QUALITY)
        return fvm_movieclip_quality_read(c, index == FVM_AVM1_PROPERTY_HIGH_QUALITY, out);
    if (index > FVM_AVM1_PROPERTY_NAME)
        return FVM_UNSUPPORTED;
    if (index == FVM_AVM1_PROPERTY_TARGET)
        return display_path(c, clip, out, true);
    if (index == FVM_AVM1_PROPERTY_CURRENT_FRAME || index == FVM_AVM1_PROPERTY_TOTAL_FRAMES ||
        index == FVM_AVM1_PROPERTY_FRAMES_LOADED)
    {
        unsigned long value = 0;
        status = integer_field(data, index == FVM_AVM1_PROPERTY_CURRENT_FRAME ? "frame" : "total",
                               FVM_MOVIECLIP_MAX_FRAMES, value);
        if (status != FVM_OK || !value)
            return FVM_INVALID_ARGUMENT;
        fvm_value_number(out, value);
        return FVM_OK;
    }
    const char *names[] = {"_x", "_y", "_xscale", "_yscale", "", "", "_alpha", "_visible",
                           "", "", "_rotation", "", "", "_name"};
    FVM_Value value;
    status = field(clip, names[index], value);
    if (status != FVM_OK)
        return status;
    if (index == FVM_AVM1_PROPERTY_NAME)
    {
        if (value.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
        *out = value;
    }
    else
    {
        if (value.type != FVM_VALUE_NUMBER &&
            !(index == FVM_AVM1_PROPERTY_VISIBLE && value.type == FVM_VALUE_BOOLEAN))
            return FVM_INVALID_ARGUMENT;
        if (index == FVM_AVM1_PROPERTY_VISIBLE)
            fvm_value_boolean(out, value.type == FVM_VALUE_BOOLEAN ? value.boolean : value.number != 0);
        else
            fvm_value_number(out, value.number);
    }
    return FVM_OK;
}

/* Publish typed display scalars with twip position conversion and normalized rotation. */
FVM_Result fvm_movieclip_set_action_property(FVM_Context *c, FVM_Object *clip,
                                            unsigned int index, const FVM_Value &input)
{
    if (!c || !clip || clip->owner != c || fvm_validate_value(c, &input) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0;
    FVM_Result status = state(clip, data);
    if (status != FVM_OK)
        return status;
    if (index == FVM_AVM1_PROPERTY_WIDTH || index == FVM_AVM1_PROPERTY_HEIGHT)
    {
        if (input.type != FVM_VALUE_NUMBER)
            return FVM_INVALID_ARGUMENT;
        if (input.number != input.number)
            return FVM_OK;
        return resize_dimension(c, clip, index == FVM_AVM1_PROPERTY_WIDTH, input.number);
    }
    if (index == FVM_AVM1_PROPERTY_HIGH_QUALITY || index == FVM_AVM1_PROPERTY_QUALITY)
        return fvm_movieclip_quality_write(c, index == FVM_AVM1_PROPERTY_HIGH_QUALITY, input);
    if (index > FVM_AVM1_PROPERTY_NAME)
        return FVM_UNSUPPORTED;
    if (index == FVM_AVM1_PROPERTY_CURRENT_FRAME || index == FVM_AVM1_PROPERTY_TOTAL_FRAMES ||
        index == FVM_AVM1_PROPERTY_TARGET || index == FVM_AVM1_PROPERTY_FRAMES_LOADED)
        return FVM_OK;
    const char *names[] = {"_x", "_y", "_xscale", "_yscale", "", "", "_alpha", "_visible",
                           "", "", "_rotation", "", "", "_name"};
    FVM_Value value = input;
    if (index == FVM_AVM1_PROPERTY_NAME)
    {
        if (value.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
    }
    else
    {
        if (value.type != FVM_VALUE_NUMBER)
            return FVM_INVALID_ARGUMENT;
        double number = value.number;
        if (number != number)
            return FVM_OK;
        if (index <= FVM_AVM1_PROPERTY_Y)
        {
            double twips = number * FVM_MOVIECLIP_TWIPS_PER_PIXEL;
            if (number == std::numeric_limits<double>::infinity() ||
                number == -std::numeric_limits<double>::infinity())
                twips = FVM_INT32_MIN_NUMBER;
            if (twips >= FVM_INT32_MAX_NUMBER)
                twips = FVM_INT32_MAX_NUMBER;
            else if (twips <= FVM_INT32_MIN_NUMBER)
                twips = FVM_INT32_MIN_NUMBER;
            else
                twips = twips < 0 ? std::ceil(twips) : std::floor(twips);
            number = twips / FVM_MOVIECLIP_TWIPS_PER_PIXEL;
        }
        else if (number == std::numeric_limits<double>::infinity() ||
                 number == -std::numeric_limits<double>::infinity())
            return FVM_UNSUPPORTED;
        if (index == FVM_AVM1_PROPERTY_VISIBLE)
            number = number != 0 ? 1 : 0;
        else if (index == FVM_AVM1_PROPERTY_ROTATION)
        {
            number = std::fmod(number, FVM_MOVIECLIP_DEGREES_PER_TURN);
            if (number < -FVM_MOVIECLIP_DEGREES_PER_HALF_TURN)
                number += FVM_MOVIECLIP_DEGREES_PER_TURN;
            else if (number > FVM_MOVIECLIP_DEGREES_PER_HALF_TURN)
                number -= FVM_MOVIECLIP_DEGREES_PER_TURN;
        }
        fvm_value_number(&value, number);
    }
    try
    {
        /* [GC:] Adopt the already-rooted converted value before request roots are released. */
        clip->properties[names[index]] = value;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Apply timeline controls through built-in operations rather than replaceable method properties. */
FVM_Result fvm_movieclip_frame_control(FVM_Context *c, FVM_Object *clip, FVM_ExecutionFrame command,
                                       const FVM_Value *frame, int enabled)
{
    if (!c || !clip || clip->owner != c)
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Native seek/state updates cannot collect through host conversion callbacks. */
        FVM_NativeCallGuard guard(c);
        FVM_Value ignored;
        if (command == FVM_EXEC_NEXT_FRAME)
            return next(c, clip, 0, 0, &ignored, 0);
        if (command == FVM_EXEC_PREVIOUS_FRAME)
            return previous(c, clip, 0, 0, &ignored, 0);
        if (command == FVM_EXEC_PLAY || command == FVM_EXEC_STOP)
            return playing(c, clip, &ignored, command == FVM_EXEC_PLAY);
        if (command != FVM_EXEC_GOTO_FRAME || !frame)
            return FVM_INVALID_ARGUMENT;
        return seek(c, clip, 1, frame, &ignored, enabled != 0);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Identify the retained builtin class prototype without allocation or script lookup. */
bool fvm_movieclip_is_class_prototype(FVM_Object *clip)
{
    FVM_Object *data = 0;
    if (state(clip, data) != FVM_OK)
        return false;
    /* Read private state without allocating a temporary key or invoking script. */
    for (Properties::iterator entry = data->properties.begin(); entry != data->properties.end();
         ++entry)
        if (entry->first.size() == sizeof(FVM_MOVIECLIP_CLASS_PROTOTYPE_KEY) - 1 &&
            std::memcmp(entry->first.data(), FVM_MOVIECLIP_CLASS_PROTOTYPE_KEY,
                        sizeof(FVM_MOVIECLIP_CLASS_PROTOTYPE_KEY) - 1) == 0)
            return entry->second.type == FVM_VALUE_BOOLEAN && entry->second.boolean;
    return false;
}

/* Project the builtin class prototype as Object while real clips retain display paths. */
FVM_Result fvm_movieclip_reference_string(FVM_Context *c, FVM_Object *clip, FVM_Value *out)
{
    if (!c || !clip || clip->owner != c || !out)
        return FVM_INVALID_ARGUMENT;
    if (c->swf_loaded && fvm_movieclip_is_class_prototype(clip))
        return fvm_value_string(out, "[object Object]");
    return display_path(c, clip, out, false);
}

/* [GC:] Retain every selected block and receiver while nested frame calls execute script. */
FVM_Result fvm_movieclip_call_frame(FVM_Context *c, FVM_Object *clip, const FVM_Value &argument)
{
    if (!c || !clip || clip->owner != c || c->script_depth >= 64)
        return FVM_ERROR;
    try
    {
        FVM_Object *data = 0, *frames = 0;
        FVM_Result status = state(clip, data);
        if (status == FVM_OK)
            status = private_object(data, "frames", frames);
        if (status != FVM_OK)
            return status;
        unsigned long selected = 0;
        double numeric = std::numeric_limits<double>::quiet_NaN();
        if (argument.type == FVM_VALUE_NUMBER)
            numeric = argument.number;
        else if (argument.type == FVM_VALUE_STRING)
        {
            size_t size = fvm_string_size(argument);
            bool decimal = size != 0;
            for (size_t i = 0; i < size; ++i)
            {
                unsigned char ch = (unsigned char)argument.string[i];
                if (ch == ':' || ch == '/' )
                    return FVM_UNSUPPORTED;
                if (!((ch >= '0' && ch <= '9') || ch == '+' || ch == '-' ||
                      ch == '.' || ch == 'e' || ch == 'E'))
                    decimal = false;
            }
            if (decimal)
            {
                status = fvm_value_to_number(c, &argument, &numeric);
                if (status != FVM_OK)
                    return status;
            }
            if (numeric != numeric)
                for (Slots::iterator i = frames->indexed.begin(); i != frames->indexed.end(); ++i)
                {
                    if (i->second.type != FVM_VALUE_OBJECT || !i->second.object)
                        return FVM_INVALID_ARGUMENT;
                    FVM_Value label;
                    status = field(i->second.object, "label", label);
                    if (status != FVM_OK || label.type != FVM_VALUE_STRING)
                        return FVM_INVALID_ARGUMENT;
                    int order = 0;
                    status = fvm_string_order(label, argument, order);
                    if (status != FVM_OK)
                        return status;
                    if (!order)
                    {
                        selected = i->first;
                        break;
                    }
                }
        }
        else
            return FVM_INVALID_ARGUMENT;
        if (numeric == numeric && numeric != std::numeric_limits<double>::infinity() &&
            numeric != -std::numeric_limits<double>::infinity())
        {
            numeric = numeric < 0 ? std::ceil(numeric) : std::floor(numeric);
            numeric = std::fmod(numeric, 4294967296.0);
            selected = (unsigned long)(numeric < 0 ? numeric + 4294967296.0 : numeric);
        }
        if (!selected || selected > FVM_MOVIECLIP_MAX_FRAMES)
            return FVM_OK;
        Slots::iterator entry = frames->indexed.find(selected);
        if (entry == frames->indexed.end())
            return FVM_OK;
        if (entry->second.type != FVM_VALUE_OBJECT || !entry->second.object)
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Snapshot guard also bounds recursion and restores the caller's debug cursor. */
        struct FrameScope
        {
            FVM_Context *context; /* Context owning the root chain and debug cursor. */
            FVM_GCRootFrame frame; /* [GC:] Temporary block/receiver roots. */
            fvm_detail::vector<FVM_Value> blocks; /* [GC:] Action strings copied before script entry. */
            fvm_detail::string function; /* Caller debug function name restored on exit. */
            unsigned long offset; /* Caller bytecode offset restored on exit. */
            /* [GC:] Link empty block roots before preparing the snapshot. */
            FrameScope(FVM_Context *owner, FVM_Object *receiver)
                : context(owner), function(owner->debug_function), offset(owner->debug_offset)
            {
                owner->debug_function = "[Frame Call]";
                frame.previous = owner->gc_frames;
                frame.stack = &blocks;
                frame.locals = 0;
                frame.receiver = receiver;
                owner->gc_frames = &frame;
                ++owner->script_depth;
            }
            /* [GC:] Unlink the frame and restore nesting/debug state on every exit. */
            ~FrameScope()
            {
                --context->script_depth;
                context->gc_frames = frame.previous;
                context->debug_function.swap(function);
                context->debug_offset = offset;
            }
          private:
            /* Keep root-chain ownership unique. */
            FrameScope(const FrameScope &);
            /* Keep root-chain ownership unique. */
            FrameScope &operator=(const FrameScope &);
        } scope(c, clip);
        FVM_Object *record = entry->second.object;
        Properties::iterator segments = record->properties.find("segments");
        if (segments != record->properties.end())
        {
            if (segments->second.type != FVM_VALUE_OBJECT || !segments->second.object)
                return FVM_INVALID_ARGUMENT;
            for (Slots::iterator i = segments->second.object->indexed.begin();
                 i != segments->second.object->indexed.end(); ++i)
                scope.blocks.push_back(i->second);
        }
        else
        {
            FVM_Value code;
            status = field(record, "code", code);
            if (status != FVM_OK)
                return status;
            scope.blocks.push_back(code);
        }
        for (size_t i = 0; i < scope.blocks.size(); ++i)
        {
            status = execute_frame_block(c, clip, scope.blocks[i]);
            if (status != FVM_OK)
                return status;
        }
        return FVM_OK;
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

/* Create movieclip. */
extern "C" FVM_Result fvm_movieclip_create(FVM_Context *c, const char *name, unsigned int frames,
                                           FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || !name)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_Value klass;
        FVM_Result r = fvm_get_global(c, "MovieClip", &klass);
        if (r != FVM_OK)
            return r;
        if (klass.type != FVM_VALUE_OBJECT)
            return FVM_UNSUPPORTED;
        FVM_Value proto;
        r = fvm_object_get(c, klass.object, "prototype", &proto);
        if (r != FVM_OK)
            return r;
        if (proto.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        return make(c, name, frames, proto.object, out);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Return a clip's local pixel-space affine transform, including authored shear. */
extern "C" FVM_Result fvm_movieclip_get_matrix(FVM_Context *c, FVM_Object *o, FVM_Affine *out)
{
    if (!c || !o || o->owner != c || !out)
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Property conversions may invoke script; protect the receiver during the query. */
        FVM_NativeCallGuard guard(c);
        FVM_Affine value;
        FVM_Result r = local_matrix(c, o, value);
        if (r == FVM_OK)
            *out = value;
        return r;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Set a finite local affine transform atomically across public and private display state. */
extern "C" FVM_Result fvm_movieclip_set_matrix(FVM_Context *c, FVM_Object *o,
                                               const FVM_Affine *value)
{
    if (!c || !o || o->owner != c || !value)
        return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Retain the receiver and staged maps until the complete transform is published. */
        FVM_NativeCallGuard guard(c);
        return apply_matrix(o, *value);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Query a clip's accumulated matrix under a barrier protecting display-list references. */
FVM_Result fvm_transform_scene_matrix(FVM_Context *c, FVM_Object *o, FVM_Affine *out)
{
    if (!c || !o || o->owner != c || !out)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Protect the parent graph while public coordinate conversions execute. */
    FVM_NativeCallGuard guard(c);
    return matrix(c, o, *out);
}

/* Query local or inherited color terms in the same order used by subtree rendering. */
FVM_Result fvm_transform_scene_color(FVM_Context *c, FVM_Object *o, bool world, double *out)
{
    if (!c || !o || o->owner != c || !out)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Retain display/color records while getters and parent traversal can execute. */
    FVM_NativeCallGuard guard(c);
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    ColorAffine inherited, result;
    if (r == FVM_OK && world)
        r = ancestor_color(c, o, inherited);
    if (r == FVM_OK)
        r = display_color(c, o, data, inherited, result);
    if (r == FVM_OK)
        for (unsigned int i = 0; i < 4; ++i)
        {
            out[i] = result.multiply[i];
            out[i + 4] = result.add[i];
        }
    return r;
}

/* Publish copied RGBA terms and _alpha together, preserving old state on allocation failure. */
FVM_Result fvm_transform_scene_set_color(FVM_Context *c, FVM_Object *o, const double *values)
{
    if (!c || !o || o->owner != c || !values)
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < 8; ++i)
        if (values[i] != values[i] || std::fabs(values[i]) > DBL_MAX ||
            (i == 3 && std::fabs(values[i]) > DBL_MAX / 100))
            return FVM_INVALID_ARGUMENT;
    /* [GC:] Protect unpublished color records and both staged property maps. */
    FVM_NativeCallGuard guard(c);
    FVM_Object *data = 0, *record = 0;
    FVM_Result r = state(o, data);
    if (r == FVM_OK)
        /* [GC:] Allocate independent numeric storage instead of retaining the source
         * ColorTransform. */
        r = fvm_object_create(c, &record);
    if (r != FVM_OK)
        return r;
    const char *names[] = {"rm", "gm", "bm", "am", "ra", "ga", "ba", "aa"};
    for (unsigned int i = 0; i < 8; ++i)
        record->properties[names[i]] = number(values[i]);
    /* [GC:] Stage public properties and preserve every existing managed edge. */
    fvm_detail::map<fvm_detail::string, FVM_Value> properties = o->properties;
    /* [GC:] Stage private display properties before publishing the new color edge. */
    fvm_detail::map<fvm_detail::string, FVM_Value> private_state = data->properties;
    properties["_alpha"] = number(values[3] * 100);
    /* [GC:] The staged private map retains the independent color record. */
    private_state["swf_color"] = object(record);
    /* [GC:] Commit both complete maps using nonallocating swaps. */
    o->properties.swap(properties);
    /* [GC:] Publish the matching private color graph after the corresponding numeric properties. */
    data->properties.swap(private_state);
    return FVM_OK;
}

/* Query world-space stroked vector bounds without requiring an invertible display matrix. */
FVM_Result fvm_transform_scene_bounds(FVM_Context *c, FVM_Object *o, FVM_PathBounds *out)
{
    if (!c || !o || o->owner != c || !out)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Protect child/path records during recursive bounds computation. */
    FVM_NativeCallGuard guard(c);
    FVM_PathBounds result;
    std::memset(&result, 0, sizeof(result));
    bool any = false;
    FVM_Result r = bounds(c, o, identity(), true, result, any, 0);
    if (r == FVM_OK)
        *out = result;
    return r;
}

/* [GC:] Define movieclip frame. */
extern "C" FVM_Result fvm_movieclip_define_frame(FVM_Context *c, FVM_Object *o, unsigned int frame,
                                                 const char *label, const void *actions,
                                                 size_t size)
{
    if (!c || !o || o->owner != c || (!actions && size) ||
        size > FVM_MOVIECLIP_MAX_FRAME_ACTION_BYTES)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_Object *data = 0, *frames = 0;
        FVM_Result r = state(o, data);
        if (r != FVM_OK)
            return r;
        unsigned long total = 0;
        r = integer_field(data, "total", FVM_MOVIECLIP_MAX_FRAMES, total);
        if (r != FVM_OK || frame < 1 || frame > total)
            return FVM_INVALID_ARGUMENT;
        unsigned char empty = 0;
        const unsigned char *bytes = size ? (const unsigned char *)actions : &empty;
        size_t count = size ? size : 1;
        FVM_AVM1Info info;
        info.struct_size = sizeof(info);
        r = fvm_avm1_inspect(bytes, count, &info);
        if (r != FVM_OK)
            return r;
        r = private_object(data, "frames", frames);
        if (r != FVM_OK)
            return r;
        FVM_Object *record = 0;
        r = fvm_object_create(c, &record);
        if (r != FVM_OK)
            return r;
        fvm_detail::vector<char> code;
        const char *digits = "0123456789abcdef";
        for (size_t i = 0; i < count; ++i)
        {
            code.push_back(digits[bytes[i] >> 4]);
            code.push_back(digits[bytes[i] & 15]);
        }
        code.push_back(0);
        FVM_Value v;
        fvm_value_string(&v, code.begin());
        r = fvm_object_set(c, record, "code", &v);
        if (r != FVM_OK)
            return r;
        fvm_value_string(&v, label ? label : "");
        r = fvm_object_set(c, record, "label", &v);
        if (r != FVM_OK)
            return r;
        frames->indexed[frame] = object(record);
        /* Re-enter a replaced current frame once, including single-frame clips. */
        FVM_Value current;
        if (field(data, "frame", current) == FVM_OK && current.number == frame)
            data->properties["entered"] = number(0);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Compute and return movieclip advance. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_movieclip_advance(FVM_Context *c, FVM_Object *o, unsigned int ticks)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!c || !o || o->owner != c || ticks > FVM_MOVIECLIP_MAX_TICKS_PER_CALL)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
        FVM_NativeCallGuard guard(c);
        /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
        for (unsigned int i = 0; i < ticks; ++i)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Result r = advance(c, o, 0);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (r != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return r;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    catch (const std::bad_alloc &)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OUT_OF_MEMORY;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Render movieclip. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
extern "C" FVM_Result fvm_movieclip_render(
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Context *c, FVM_Object *o,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_RasterSurface *surface)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!c || !o || o->owner != c || !surface)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
        FVM_NativeCallGuard guard(c);
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        ColorAffine root_color;
        FVM_Result r = ancestor_color(c, o, root_color);
        return r == FVM_OK ? render(c, o, surface, root_color, 0) : r;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    catch (const std::bad_alloc &)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OUT_OF_MEMORY;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Present through a host view while retaining ordinary root visibility, pose and color.
 * [GC:] Copy host matrix before rendering getters; no script-visible transform is modified. */
extern "C" FVM_Result fvm_movieclip_render_view(FVM_Context *c, FVM_Object *clip,
                                                FVM_RasterSurface *surface, const FVM_Affine *view)
{
    if (!c || !clip || clip->owner != c || !surface || !view || c->destroying ||
        c->gc_collecting || c->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    const double values[FVM_MOVIECLIP_VIEW_COMPONENTS] = {view->xx, view->yx, view->xy,
                                                         view->yy, view->x0, view->y0};
    for (unsigned int i = 0; i < FVM_MOVIECLIP_VIEW_COMPONENTS; ++i)
        if (values[i] != values[i] || values[i] > DBL_MAX || values[i] < -DBL_MAX)
            return FVM_INVALID_ARGUMENT;
    FVM_Affine viewport = *view;
    try
    {
        FVM_NativeCallGuard guard(c);
        ColorAffine root_color;
        FVM_Result result = ancestor_color(c, clip, root_color);
        return result == FVM_OK ? render(c, clip, surface, root_color, 0, 0, 0, &viewport) : result;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* [GC:] Render source-library geometry while ignoring the root clip's on-stage pose/color. */
FVM_Result fvm_movieclip_render_bitmap(FVM_Context *c, FVM_Object *clip, FVM_RasterSurface *surface,
                                       const FVM_Affine &matrix, const double *multipliers,
                                       const double *offsets, FVM_RasterSurface *coverage)
{
    if (!c || !clip || clip->owner != c || !surface || !multipliers || !offsets)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] The caller retains source and destination while script getters inspect child poses. */
    FVM_NativeCallGuard guard(c);
    ColorAffine color;
    for (unsigned i = 0; i < 4; ++i)
    {
        color.multiply[i] = multipliers[i];
        color.add[i] = offsets[i];
    }
    return render(c, clip, surface, color, 0, &matrix, coverage);
}

/* Build constructor and prototype for movieclip. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
fvm_movieclip_stage(FVM_Context *c,
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *klass = 0, *proto = 0;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result r = fvm_object_create(c, &klass);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = make(c, "", FVM_MOVIECLIP_DEFAULT_FRAMES, 0, &proto);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    /* [GC:] The marker belongs to retained private state and survives graph transport. */
    FVM_Value prototype_marker;
    fvm_value_boolean(&prototype_marker, true);
    proto->resource_result.object->properties[FVM_MOVIECLIP_CLASS_PROTOTYPE_KEY] = prototype_marker;
    klass->resource_kind = CLIP_CLASS;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Value v = object(klass);

    /* Quality accessors share context state, including after graph transport. */
    FVM_AccessorEntry quality, highquality;
    quality.getter = fvm_movieclip_quality_get;
    quality.setter = fvm_movieclip_quality_set;
    highquality.getter = fvm_movieclip_highquality_get;
    highquality.setter = fvm_movieclip_highquality_set;
    proto->accessors["_quality"] = quality;
    proto->accessors["_highquality"] = highquality;
    proto->attributes["_quality"] = FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE;
    proto->attributes["_highquality"] = FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE;

    /* _lockroot is a nonenumerable, nondeletable instance property, independent of _parent. */
    FVM_AccessorEntry lockroot;
    lockroot.getter = fvm_movieclip_lockroot_get;
    lockroot.setter = fvm_movieclip_lockroot_set;
    proto->accessors["_lockroot"] = lockroot;
    proto->attributes["_lockroot"] = FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE;
    FVM_AccessorEntry mouse_x, mouse_y;
    mouse_x.getter = fvm_movieclip_xmouse_get;
    mouse_y.getter = fvm_movieclip_ymouse_get;
    proto->accessors["_xmouse"] = mouse_x;
    proto->accessors["_ymouse"] = mouse_y;
    proto->attributes["_xmouse"] = FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE;
    proto->attributes["_ymouse"] = FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    globals["MovieClip"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    proto->properties["constructor"] = v;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    klass->properties["prototype"] = object(proto);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeEntry e;
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        e.callback = methods[i].callback;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        proto->methods[methods[i].name] = e;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const char *keys[] = {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        "_parent", "_root", "_currentframe",
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        "_totalframes", "_framesloaded", "_target"};
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_NativeFunction getters[] = {parent_get, root_get, frame_get, total_get, total_get, target};
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (unsigned int i = 0; i < 6; ++i)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_AccessorEntry a;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        a.getter = getters[i];
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        proto->accessors[keys[i]] = a;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        proto->attributes[keys[i]] = FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    e.callback = constructor;
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    e.root = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    e.userdata = klass;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    constructors["MovieClip"] = e;
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Validate movieclip. */
FVM_Result fvm_movieclip_validate(FVM_Context *c)
{
    FVM_Value v;
    return fvm_get_global(c, "MovieClip", &v) == FVM_OK && v.type == FVM_VALUE_OBJECT &&
                   v.object->resource_kind == CLIP_CLASS
               ? FVM_OK
               : FVM_INVALID_ARGUMENT;
}

/* Loaded reads/writes use intrinsic paths; only unloaded host execution exposes these descriptors. */
bool fvm_movieclip_hidden_path_accessor(FVM_Context *c, const FVM_AccessorEntry &entry)
{
    return c->swf_loaded && (entry.getter == parent_get || entry.getter == root_get);
}

/* Read display provenance directly; parentless clips return the existing null sentinel. */
FVM_Result fvm_movieclip_preload_display(FVM_Context *c, FVM_Object *clip, bool want_parent,
                                       FVM_Value *out)
{
    if (!c || !clip || clip->owner != c || !out)
        return FVM_INVALID_ARGUMENT;
    return want_parent ? parent_get(c, clip, 0, 0, out, 0)
                       : root_get(c, clip, 0, 0, out, 0);
}

/* Map a native movieclip callback to its stable serialization identifier. */
unsigned long fvm_movieclip_callback_id(FVM_NativeFunction cb)
{
    if (cb == constructor)
        return FVM_MOVIECLIP_CALLBACK_CONSTRUCTOR;
    if (cb == parent_get)
        return FVM_MOVIECLIP_CALLBACK_PARENT;
    if (cb == root_get)
        return FVM_MOVIECLIP_CALLBACK_ROOT;
    if (cb == frame_get)
        return FVM_MOVIECLIP_CALLBACK_CURRENT_FRAME;
    if (cb == total_get)
        return FVM_MOVIECLIP_CALLBACK_TOTAL_FRAMES;
    if (cb == target)
        return FVM_MOVIECLIP_CALLBACK_TARGET_PATH;
    if (cb == fvm_movieclip_lockroot_get)
        return FVM_MOVIECLIP_CALLBACK_LOCKROOT_GET;
    if (cb == fvm_movieclip_lockroot_set)
        return FVM_MOVIECLIP_CALLBACK_LOCKROOT_SET;
    if (cb == fvm_movieclip_xmouse_get)
        return FVM_MOVIECLIP_CALLBACK_XMOUSE_GET;
    if (cb == fvm_movieclip_ymouse_get)
        return FVM_MOVIECLIP_CALLBACK_YMOUSE_GET;
    if (cb == fvm_movieclip_quality_get)
        return FVM_MOVIECLIP_CALLBACK_QUALITY_GET;
    if (cb == fvm_movieclip_quality_set)
        return FVM_MOVIECLIP_CALLBACK_QUALITY_SET;
    if (cb == fvm_movieclip_highquality_get)
        return FVM_MOVIECLIP_CALLBACK_HIGHQUALITY_GET;
    if (cb == fvm_movieclip_highquality_set)
        return FVM_MOVIECLIP_CALLBACK_HIGHQUALITY_SET;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
        if (cb == methods[i].callback)
            return FVM_MOVIECLIP_CALLBACK_METHOD_BASE + (unsigned long)i;
    return 0;
}

/* Restore a native movieclip callback from its stable serialization identifier. */
FVM_NativeFunction fvm_movieclip_callback_from_id(unsigned long id)
{
    if (id == FVM_MOVIECLIP_CALLBACK_CONSTRUCTOR)
        return constructor;
    if (id == FVM_MOVIECLIP_CALLBACK_PARENT)
        return parent_get;
    if (id == FVM_MOVIECLIP_CALLBACK_ROOT)
        return root_get;
    if (id == FVM_MOVIECLIP_CALLBACK_CURRENT_FRAME)
        return frame_get;
    if (id == FVM_MOVIECLIP_CALLBACK_TOTAL_FRAMES)
        return total_get;
    if (id == FVM_MOVIECLIP_CALLBACK_TARGET_PATH)
        return target;
    if (id == FVM_MOVIECLIP_CALLBACK_LOCKROOT_GET)
        return fvm_movieclip_lockroot_get;
    if (id == FVM_MOVIECLIP_CALLBACK_LOCKROOT_SET)
        return fvm_movieclip_lockroot_set;
    if (id == FVM_MOVIECLIP_CALLBACK_XMOUSE_GET)
        return fvm_movieclip_xmouse_get;
    if (id == FVM_MOVIECLIP_CALLBACK_YMOUSE_GET)
        return fvm_movieclip_ymouse_get;
    if (id == FVM_MOVIECLIP_CALLBACK_QUALITY_GET)
        return fvm_movieclip_quality_get;
    if (id == FVM_MOVIECLIP_CALLBACK_QUALITY_SET)
        return fvm_movieclip_quality_set;
    if (id == FVM_MOVIECLIP_CALLBACK_HIGHQUALITY_GET)
        return fvm_movieclip_highquality_get;
    if (id == FVM_MOVIECLIP_CALLBACK_HIGHQUALITY_SET)
        return fvm_movieclip_highquality_set;
    return id >= FVM_MOVIECLIP_CALLBACK_METHOD_BASE &&
                   id < FVM_MOVIECLIP_CALLBACK_METHOD_BASE + sizeof(methods) / sizeof(methods[0])
               ? methods[id - FVM_MOVIECLIP_CALLBACK_METHOD_BASE].callback
               : 0;
}
