/* FlashVM: counted AVM1 display paths with child priority, prototype fallback and scoped GC roots. */
#include "movieclip_state.hpp"
#include "movieclip_path_constants.h"
#include "string_order.hpp"
#include "../core/property_key.hpp"
#include "../avm1/scope_keywords.hpp"
#include "flashvm/property.h"
#include <new>
namespace
{
/* Read validated private MovieClip state without invoking script accessors. */
FVM_Result state(FVM_Object *clip, FVM_Object *&out)
{
    if (!clip || clip->resource_kind != FVM_RESOURCE_MOVIECLIP || !clip->resource_result_valid ||
        clip->resource_result.type != FVM_VALUE_OBJECT || !clip->resource_result.object)
        return FVM_INVALID_ARGUMENT;
    out = clip->resource_result.object;
    return FVM_OK;
}
/* Read a private object edge, accepting the null parent at a display root. */
FVM_Result edge(FVM_Object *object, const char *key, FVM_Object *&out)
{
    out = 0;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator entry = object->properties.find(key);
    if (entry == object->properties.end())
        return FVM_INVALID_ARGUMENT;
    if (entry->second.type == FVM_VALUE_NULL)
        return FVM_OK;
    if (entry->second.type != FVM_VALUE_OBJECT || !entry->second.object)
        return FVM_INVALID_ARGUMENT;
    out = entry->second.object;
    return FVM_OK;
}
/* Read the display parent, requiring consistent native state. */
FVM_Result parent(FVM_Object *clip, FVM_Object *&out)
{
    out = 0;
    if (clip && clip->resource_kind != FVM_RESOURCE_MOVIECLIP)
        return FVM_OK;
    FVM_Object *data = 0;
    FVM_Result status = state(clip, data);
    return status == FVM_OK ? edge(data, "parent", out) : status;
}
/* Fold only ASCII for the verified SWF 4–6 path-name subset. */
unsigned char lower(unsigned char value)
{
    return value >= 'A' && value <= 'Z' ? (unsigned char)(value + ('a' - 'A')) : value;
}
/* Compare complete counted ASCII names independently from the host locale. */
bool ascii_equal(const fvm_detail::string &a, const fvm_detail::string &b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (lower((unsigned char)a[i]) != lower((unsigned char)b[i]))
            return false;
    return true;
}
/* Find the existing property spelling so accessor/method dispatch retains its owner contract. */
template <class T>
bool key(fvm_detail::map<fvm_detail::string, T> &entries, const fvm_detail::string &name,
         bool sensitive, fvm_detail::string &out)
{
    if (sensitive)
    {
        if (entries.find(name) == entries.end())
            return false;
        out = name;
        return true;
    }
    for (typename fvm_detail::map<fvm_detail::string, T>::iterator i = entries.begin();
         i != entries.end(); ++i)
        if (ascii_equal(i->first, name))
        {
            out = i->first;
            return true;
        }
    return false;
}
/* Read a fallback property through its actual receiver, including inherited accessors. */
FVM_Result property(FVM_Context *c, FVM_Object *receiver, const fvm_detail::string &name,
                    bool sensitive, FVM_Value &out, bool *present = 0)
{
    if (present)
        *present = false;
    FVM_Object *cursor = receiver;
    fvm_detail::string spelling;
    unsigned int visited = 0;
    while (cursor)
    {
        if (++visited > FVM_MOVIECLIP_PATH_MAX_VISITS)
            return FVM_ERROR;
        if (key(cursor->properties, name, sensitive, spelling) ||
            key(cursor->methods, name, sensitive, spelling) ||
            key(cursor->accessors, name, sensitive, spelling))
        {
            if (present)
                *present = true;
            return fvm_object_get_bytes(c, receiver, spelling.data(), spelling.size(), &out);
        }
        int intrinsic = 0;
        FVM_Result status = fvm_avm1_movieclip_path_property(c, cursor, name, &out, &intrinsic);
        if (status != FVM_OK)
            return status;
        if (intrinsic)
        {
            if (present)
                *present = true;
            return FVM_OK;
        }
        cursor = cursor->prototype;
    }
    /* Preserve indexed/virtual standard-object behavior even without a stored named entry. */
    FVM_Result status = fvm_object_get_bytes(c, receiver, name.data(), name.size(), &out);
    if (status == FVM_OK && present && out.type != FVM_VALUE_UNDEFINED)
        *present = true;
    return status;
}
/* Resolve named display children before dynamic properties with the same name. */
FVM_Result child(FVM_Object *receiver, const fvm_detail::string &name, bool sensitive,
                 FVM_Object *&out)
{
    out = 0;
    if (receiver->resource_kind != FVM_RESOURCE_MOVIECLIP)
        return FVM_OK;
    FVM_Object *data = 0, *children = 0;
    FVM_Result status = state(receiver, data);
    if (status == FVM_OK)
        status = edge(data, "children", children);
    if (status != FVM_OK || !children)
        return FVM_INVALID_ARGUMENT;
    FVM_Value token;
    status = fvm_value_string_bytes(&token, name.data(), name.size());
    if (status != FVM_OK)
        return status;
    for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = children->indexed.begin();
         i != children->indexed.end(); ++i)
    {
        if (i->second.type != FVM_VALUE_OBJECT || !i->second.object)
            return FVM_INVALID_ARGUMENT;
        FVM_Object *candidate = i->second.object;
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator stored =
            candidate->properties.find("_name");
        if (stored == candidate->properties.end())
            continue;
        if (stored->second.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
        bool match = false;
        if (sensitive)
        {
            int order = 0;
            status = fvm_string_order(stored->second, token, order);
            if (status != FVM_OK)
                return status;
            match = order == 0;
        }
        else
            match = ascii_equal(fvm_detail::string(stored->second.string,
                                                   fvm_string_size(stored->second)), name);
        if (match)
        {
            FVM_Object *child_data = 0;
            status = state(candidate, child_data);
            if (status != FVM_OK)
                return status;
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator art =
                child_data->properties.find("static_art");
            out = art != child_data->properties.end() && art->second.type == FVM_VALUE_NUMBER &&
                          art->second.number == FVM_MOVIECLIP_PATH_STATIC_ART ? receiver : candidate;
            return FVM_OK;
        }
    }
    return FVM_OK;
}
/* [GC:] Keep original/root/current objects and the input string live during accessor callbacks. */
struct PathScope
{
    FVM_Context *context; /* Context owning the temporary root chain. */
    FVM_GCRootFrame frame; /* [GC:] Root frame linked only for the resolver lifetime. */
    fvm_detail::vector<FVM_Value> values; /* [GC:] Start, root, path, current and explicit this snapshots. */
    /* [GC:] Initialize and link complete root slots before any script property lookup. */
    PathScope(FVM_Context *c, FVM_Object *start, const FVM_Value &path, FVM_Object *path_this)
        : context(c), values(FVM_MOVIECLIP_PATH_ROOT_COUNT)
    {
        values[FVM_MOVIECLIP_PATH_START_ROOT].type = FVM_VALUE_OBJECT;
        values[FVM_MOVIECLIP_PATH_START_ROOT].object = start;
        values[FVM_MOVIECLIP_PATH_TEXT_ROOT] = path;
        if (path_this)
        {
            values[FVM_MOVIECLIP_PATH_THIS_ROOT].type = FVM_VALUE_OBJECT;
            values[FVM_MOVIECLIP_PATH_THIS_ROOT].object = path_this;
        }
        frame.previous = c->gc_frames;
        frame.stack = &values;
        frame.locals = 0;
        frame.receiver = start;
        c->gc_frames = &frame;
    }
    /* [GC:] Unlink temporary snapshots before their vector is destroyed. */
    ~PathScope()
    {
        context->gc_frames = frame.previous;
    }
  private:
    /* Keep root-frame ownership unique. */
    PathScope(const PathScope &);
    /* Keep root-frame ownership unique. */
    PathScope &operator=(const PathScope &);
};
}
/* Resolve a counted display path; absent/nonobject components return OK with a null result. */
FVM_Result fvm_movieclip_resolve_path(FVM_Context *c, FVM_Object *start, const FVM_Value &path,
                                    FVM_Object **out, FVM_Object *display_root, FVM_Object *path_this)
{
    if (out)
        *out = 0;
    if (!c || !start || start->owner != c || !out || path.type != FVM_VALUE_STRING ||
        !path.string || (display_root && display_root->owner != c) ||
        (path_this && path_this->owner != c) || c->destroying ||
        c->gc_collecting)
        return FVM_INVALID_ARGUMENT;
    try
    {
        PathScope scope(c, start, path, path_this);
        FVM_Object *root = display_root ? display_root : start, *next = 0;
        unsigned int visited = 0;
        while (!display_root)
        {
            if (++visited > FVM_MOVIECLIP_PATH_MAX_VISITS)
                return FVM_ERROR;
            FVM_Result status = parent(root, next);
            if (status != FVM_OK)
                return status;
            if (!next)
                break;
            root = next;
        }
        scope.values[FVM_MOVIECLIP_PATH_DISPLAY_ROOT].type = FVM_VALUE_OBJECT;
        scope.values[FVM_MOVIECLIP_PATH_DISPLAY_ROOT].object = root;
        fvm_detail::string text(path.string, fvm_string_size(path));
        bool modern = !c->swf_loaded || c->swf_info.version >= FVM_SWF_VERSION_5;
        bool sensitive = !c->swf_loaded || c->swf_info.version >= FVM_SWF_VERSION_7;
        /* Historical non-ASCII case tables are a separate unresolved compatibility boundary. */
        if (!sensitive)
            for (size_t i = 0; i < text.size(); ++i)
                if ((unsigned char)text[i] > FVM_MOVIECLIP_PATH_ASCII_MAX)
                    return FVM_UNSUPPORTED;
        size_t at = 0;
        bool first = true, slash = false;
        FVM_Object *cursor = start;
        if (!text.empty() && text[0] == '/')
        {
            cursor = root;
            at = 1;
            first = false;
            slash = true;
        }
        visited = 0;
        while (at < text.size())
        {
            if (++visited > FVM_MOVIECLIP_PATH_MAX_VISITS)
                return FVM_ERROR;
            scope.values[FVM_MOVIECLIP_PATH_CURRENT_ROOT].type = FVM_VALUE_OBJECT;
            scope.values[FVM_MOVIECLIP_PATH_CURRENT_ROOT].object = cursor;
            if (modern)
                while (at < text.size() && text[at] == ':')
                    ++at;
            if (text.size() - at >= 2 && text[at] == '.' && text[at + 1] == '.' &&
                (text.size() - at == 2 || text[at + 2] == '/' ||
                 (modern && text[at + 2] == ':')))
            {
                if (text.size() - at > 2 && text[at + 2] == '/')
                    slash = true;
                at += text.size() - at > 2 ? 3 : 2;
                FVM_Result status = parent(cursor, next);
                if (status != FVM_OK)
                    return status;
                if (!next)
                    return FVM_OK;
                cursor = next;
            }
            else
            {
                size_t begin = at;
                while (at < text.size())
                {
                    char ch = text[at];
                    if ((modern && ch == ':') || (modern && ch == '.' && !slash))
                        break;
                    if (ch == '/')
                    {
                        slash = true;
                        break;
                    }
                    ++at;
                }
                fvm_detail::string name;
                FVM_Result status = fvm_property_key(text.data() + begin, at - begin, name);
                if (status != FVM_OK)
                    return status;
                if (at < text.size())
                    ++at;
                FVM_Value value;
                fvm_value_undefined(&value);
                if (modern && path_this && name == fvm_detail::string("this"))
                {
                    value.type = FVM_VALUE_OBJECT;
                    value.object = path_this;
                }
                else if (first && modern && cursor == c->global && fvm_avm1_global_keyword(c, name))
                {
                    bool present = false;
                    if (c->swf_loaded)
                        status = property(c, cursor, name, sensitive, value, &present);
                    if (status == FVM_OK && !present)
                    {
                        value.type = FVM_VALUE_OBJECT;
                        value.object = c->global;
                    }
                }
                else if (first && modern &&
                         (sensitive ? name == fvm_detail::string("_root")
                                    : ascii_equal(name, "_root")))
                {
                    value.type = FVM_VALUE_OBJECT;
                    value.object = root;
                }
                else if (first && (sensitive ? name == fvm_detail::string("_level0")
                                             : ascii_equal(name, "_level0")))
                {
                    status = fvm_get_global(c, "_level0", &value);
                    if (status == FVM_OK && value.type == FVM_VALUE_UNDEFINED)
                    {
                        value.type = FVM_VALUE_OBJECT;
                        value.object = root;
                    }
                }
                else
                {
                    FVM_Object *found = 0;
                    status = child(cursor, name, sensitive, found);
                    if (status == FVM_OK && found)
                    {
                        value.type = FVM_VALUE_OBJECT;
                        value.object = found;
                    }
                    else if (status == FVM_OK)
                        status = property(c, cursor, name, sensitive, value);
                }
                if (status != FVM_OK)
                    return status;
                if (value.type != FVM_VALUE_OBJECT || !value.object)
                    return FVM_OK;
                cursor = value.object;
            }
            first = false;
        }
        *out = cursor;
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
