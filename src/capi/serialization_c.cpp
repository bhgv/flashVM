#include "../standard/movieclip_state.hpp"
#include "../standard/function_state.hpp"
#include "../standard/date_state.hpp"
#include "../standard/base_state.hpp"
#include "flashvm/utf8.h"
#include "../standard/string_state.hpp"
#include "../standard/number_state.hpp"
#include "../standard/error_state.hpp"
#include "../core/runtime.hpp"
#include "../core/transfer_resource.hpp"
#include "../core/capability_internal.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/serialization.h"
#include "../standard/text_state.hpp"
#include "zlib.h"
#include <cstring>
#include <cstdlib>
#include <climits>
#include <cmath>
#include <limits>
#include <new>

namespace
{
const size_t none = (size_t)-1;
const size_t max_bytes = 64UL * 1024 * 1024;
const size_t max_objects = 4096, max_entries = 65536;
struct Failure
{
    FVM_Result result;
    explicit Failure(FVM_Result r) : result(r)
    {
    }
};
void require(bool condition, FVM_Result result = FVM_INVALID_ARGUMENT)
{
    if (!condition)
        throw Failure(result);
}
struct Value
{
    FVM_ValueType type;
    double number;
    int boolean;
    fvm_detail::string string;
    size_t object, metatable;
    Value() : type(FVM_VALUE_UNDEFINED), number(0), boolean(0), object(none), metatable(none)
    {
    }
};
struct Callback
{
    unsigned long builtin;
    fvm_detail::string name;
    size_t root;
    bool root_userdata;
    Callback() : builtin(0), root(none), root_userdata(false)
    {
    }
};
struct Accessor
{
    Callback getter, setter;
};
struct Object
{
    unsigned long kind;
    Value state;
    FVM_TextFieldState text;
    size_t prototype, metatable;
    fvm_detail::map<fvm_detail::string, Value> properties;
    fvm_detail::map<unsigned long, Value> indexed;
    fvm_detail::map<fvm_detail::string, Callback> methods, metamethods;
    fvm_detail::map<fvm_detail::string, unsigned int> attributes;
    Callback invocable, constructable;
    fvm_detail::map<fvm_detail::string, Value> method_values, watchers;
    fvm_detail::map<fvm_detail::string, Accessor> accessors;
    Object() : kind(0), prototype(none), metatable(none)
    {
    }
};
struct Graph
{
    Value value;
    fvm_detail::vector<Object> objects;
};
bool known_kind(unsigned long kind)
{
    return kind == 4001 || kind == 4008 || kind == 3001 || kind == 3008 || kind == 0 ||
           kind == 10 || kind == 20 || kind == 21 || kind == 22 || kind == 23 || kind == 24 ||
           kind == 25 || kind == 26 || kind == 27 || kind == 28 || kind == 29 || kind == 30 ||
           kind == 31 || kind == 34 || kind == 35 || kind == 32 || kind == 33 || kind == 36 ||
           kind == 40 || kind == 41 || kind == 1001 || kind == 1003 || kind == 1004 || kind == 1002;
}
size_t identify(FVM_Context *c, FVM_Object *object, fvm_detail::vector<FVM_Object *> &seen)
{
    if (!object)
        return none;
    require(c && object->owner == c);
    for (size_t i = 0; i < seen.size(); ++i)
        if (seen[i] == object)
            return i;
    require(seen.size() < max_objects, FVM_UNSUPPORTED);
    seen.push_back(object);
    return seen.size() - 1;
}
void charge(size_t &budget, size_t bytes)
{
    require(bytes <= max_bytes - budget, FVM_UNSUPPORTED);
    budget += bytes;
}
Value capture_value(FVM_Context *c, const FVM_Value &src, fvm_detail::vector<FVM_Object *> &seen,
                    size_t &budget)
{
    bool valid = false;
    for (int i = FVM_VALUE_UNDEFINED; i <= FVM_VALUE_OBJECT; ++i)
    {
        FVM_ValueType candidate = (FVM_ValueType)i;
        if (!std::memcmp(&candidate, &src.type, sizeof(candidate)))
            valid = true;
    }
    require(valid);
    charge(budget, 13);
    Value dst;
    dst.type = src.type;
    require(!src.metatable || src.type != FVM_VALUE_OBJECT);
    dst.metatable = identify(c, src.metatable, seen);
    if (src.type == FVM_VALUE_NUMBER)
        dst.number = src.number;
    if (src.type == FVM_VALUE_BOOLEAN)
        dst.boolean = src.boolean != 0;
    if (src.type == FVM_VALUE_STRING)
    {
        require(src.string != 0);
        charge(budget, std::strlen(src.string));
        dst.string = src.string;
    }
    if (src.type == FVM_VALUE_OBJECT)
    {
        require(src.object != 0);
        dst.object = identify(c, src.object, seen);
    }
    return dst;
}
Callback capture_callback(FVM_Context *c, FVM_NativeFunction fn, void *userdata, FVM_Object *root,
                          fvm_detail::vector<FVM_Object *> &seen, size_t &budget)
{
    charge(budget, 13);
    Callback dst;
    if (!fn)
    {
        require(!userdata && !root);
        return dst;
    }
    const char *selector = fvm_text_callback_selector(fn, userdata);
    require(!userdata || selector || (root && userdata == root), FVM_UNSUPPORTED);
    dst.root = identify(c, root, seen);
    dst.root_userdata = userdata != 0 && !selector;
    dst.builtin = fvm_numeric_callback_id(fn);
    if (!dst.builtin)
        dst.builtin = fvm_text_callback_id(fn);
    if (!dst.builtin)
        dst.builtin = fvm_error_callback_id(fn);
    if (!dst.builtin)
        dst.builtin = fvm_number_callback_id(fn);
    if (!dst.builtin)
        dst.builtin = fvm_string_callback_id(fn);
    if (!dst.builtin)
        dst.builtin = fvm_base_callback_id(fn);
    if (!dst.builtin)
        dst.builtin = fvm_date_callback_id(fn);
    if (!dst.builtin)
        dst.builtin = fvm_function_callback_id(fn);
    if (!dst.builtin)
        dst.builtin = fvm_movieclip_callback_id(fn);
    if (selector)
        dst.name = selector;
    if (!dst.builtin)
    {
        for (fvm_detail::map<fvm_detail::string, FVM_NativeFunction>::iterator i =
                 c->serialization_callbacks.begin();
             i != c->serialization_callbacks.end(); ++i)
            if (i->second == fn)
            {
                charge(budget, i->first.size());
                dst.name = i->first;
                break;
            }
        require(!dst.name.empty(), FVM_UNSUPPORTED);
    }
    return dst;
}
Graph capture(FVM_Context *c, const FVM_Value &value)
{
    Graph graph;
    fvm_detail::vector<FVM_Object *> seen;
    size_t entries = 0, budget = 32;
    graph.value = capture_value(c, value, seen, budget);
    for (size_t i = 0; i < seen.size(); ++i)
    {
        FVM_Object *src = seen[i];
        charge(budget, 40);
        Object dst;
        if (src->resource_kind == 1 || src->resource_kind == 8)
        {
            require(src->watchers.empty(), FVM_UNSUPPORTED);
            FVM_Result result;
            if (c->transfer_resources)
            {
                unsigned long token = 0;
                result = fvm_transfer_capture_resource(c, src, &token);
                dst.kind = 3000 + (unsigned long)src->resource_kind;
                dst.state.type = FVM_VALUE_NUMBER;
                dst.state.number = (double)token;
            }
            else
            {
                result = fvm_capability_export(c, src, dst.state.string);
                dst.kind = 4000 + (unsigned long)src->resource_kind;
                dst.state.type = FVM_VALUE_STRING;
            }
            require(result == FVM_OK, result);
            graph.objects.push_back(dst);
            continue;
        }
        require(src->resource_kind >= 0 && known_kind((unsigned long)src->resource_kind) &&
                    (!src->resource || src->resource_kind == 1002) &&
                    (!src->release_resource || src->resource_kind == 1002) &&
                    !src->callable.callback,
                FVM_UNSUPPORTED);
        dst.kind = (unsigned long)src->resource_kind;
        dst.invocable = capture_callback(c, src->invocable.callback, src->invocable.userdata,
                                         src->invocable.root, seen, budget);
        dst.constructable =
            capture_callback(c, src->constructable.callback, src->constructable.userdata,
                             src->constructable.root, seen, budget);
        for (fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator method =
                 src->method_values.begin();
             method != src->method_values.end(); ++method)
        {
            FVM_Value v;
            fvm_value_undefined(&v);
            v.type = FVM_VALUE_OBJECT;
            v.object = method->second;
            dst.method_values[method->first] = capture_value(c, v, seen, budget);
            charge(budget, method->first.size() + 12);
            ++entries;
            require(entries <= max_entries, FVM_UNSUPPORTED);
        }
        for (fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator watcher =
                 src->watchers.begin();
             watcher != src->watchers.end(); ++watcher)
        {
            FVM_Value descriptor;
            fvm_value_undefined(&descriptor);
            descriptor.type = FVM_VALUE_OBJECT;
            descriptor.object = watcher->second;
            dst.watchers[watcher->first] = capture_value(c, descriptor, seen, budget);
            charge(budget, watcher->first.size() + 12);
            require(++entries <= max_entries, FVM_UNSUPPORTED);
        }
        dst.attributes = src->attributes;
        for (fvm_detail::map<fvm_detail::string, unsigned int>::iterator flag =
                 dst.attributes.begin();
             flag != dst.attributes.end(); ++flag)
        {
            require(flag->second <= 7);
            charge(budget, flag->first.size() + 12);
            ++entries;
            require(entries <= max_entries, FVM_UNSUPPORTED);
        }
        if (dst.kind == 1002)
        {
            charge(budget, fvm_textfield_snapshot_size(src));
            FVM_Result result = fvm_textfield_snapshot(src, dst.text);
            require(result == FVM_OK, result);
        }
        if (dst.kind == 20 || dst.kind == 24 || dst.kind == 26 || dst.kind == 28 ||
            dst.kind == 34 || dst.kind == 32 || dst.kind == 40)
        {
            require(src->resource_result_valid &&
                        src->resource_result.type == (dst.kind == 20   ? FVM_VALUE_BOOLEAN
                                                      : dst.kind == 26 ? FVM_VALUE_STRING
                                                      : (dst.kind == 32 || dst.kind == 40)
                                                          ? FVM_VALUE_OBJECT
                                                          : FVM_VALUE_NUMBER),
                    FVM_UNSUPPORTED);
            dst.state = capture_value(c, src->resource_result, seen, budget);
        }
        else if (dst.kind == 10)
        {
            dst.state.type = FVM_VALUE_NUMBER;
            dst.state.number = c->random_state;
        }
        else
            require(!src->resource_result_valid, FVM_UNSUPPORTED);
        dst.prototype = identify(c, src->prototype, seen);
        dst.metatable = identify(c, src->metatable, seen);
        size_t count = src->properties.size() + src->indexed.size() + src->methods.size() +
                       src->metamethods.size() + src->accessors.size();
        require(count <= max_entries - entries, FVM_UNSUPPORTED);
        entries += count;
        charge(budget, count * 4);
        for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator p = src->properties.begin();
             p != src->properties.end(); ++p)
            charge(budget, p->first.size());
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator p =
                 src->methods.begin();
             p != src->methods.end(); ++p)
            charge(budget, p->first.size());
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator p =
                 src->metamethods.begin();
             p != src->metamethods.end(); ++p)
            charge(budget, p->first.size());
        for (fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator p =
                 src->accessors.begin();
             p != src->accessors.end(); ++p)
            charge(budget, p->first.size());
        for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator p = src->properties.begin();
             p != src->properties.end(); ++p)
            dst.properties[p->first] = capture_value(c, p->second, seen, budget);
        for (fvm_detail::map<unsigned long, FVM_Value>::iterator p = src->indexed.begin();
             p != src->indexed.end(); ++p)
        {
            require(p->first <= 0xffffffffUL, FVM_UNSUPPORTED);
            dst.indexed[p->first] = capture_value(c, p->second, seen, budget);
        }
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator p =
                 src->methods.begin();
             p != src->methods.end(); ++p)
        {
            require(!p->second.collector_entry ||
                        fvm_function_collector_callback(p->second.callback),
                    FVM_UNSUPPORTED);
            dst.methods[p->first] = capture_callback(c, p->second.callback, p->second.userdata,
                                                     p->second.root, seen, budget);
        }
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator p =
                 src->metamethods.begin();
             p != src->metamethods.end(); ++p)
        {
            require(!p->second.collector_entry ||
                        fvm_function_collector_callback(p->second.callback),
                    FVM_UNSUPPORTED);
            dst.metamethods[p->first] = capture_callback(c, p->second.callback, p->second.userdata,
                                                         p->second.root, seen, budget);
        }
        for (fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator p =
                 src->accessors.begin();
             p != src->accessors.end(); ++p)
        {
            Accessor a;
            a.getter =
                capture_callback(c, p->second.getter, p->second.getter ? p->second.userdata : 0,
                                 p->second.getter ? p->second.root : 0, seen, budget);
            a.setter =
                capture_callback(c, p->second.setter, p->second.setter ? p->second.userdata : 0,
                                 p->second.setter ? p->second.root : 0, seen, budget);
            dst.accessors[p->first] = a;
        }
        graph.objects.push_back(dst);
    }
    return graph;
}
bool little_double()
{
    require(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559, FVM_UNSUPPORTED);
    const unsigned char little[] = {0, 0, 0, 0, 0, 0, 0xf0, 0x3f};
    const unsigned char big[] = {0x3f, 0xf0, 0, 0, 0, 0, 0, 0};
    unsigned char bytes[8];
    double one = 1;
    std::memcpy(bytes, &one, 8);
    require(!std::memcmp(bytes, little, 8) || !std::memcmp(bytes, big, 8), FVM_UNSUPPORTED);
    return !std::memcmp(bytes, little, 8);
}
struct Writer
{
    fvm_detail::vector<unsigned char> bytes;
    void byte(unsigned long v)
    {
        require(bytes.size() < max_bytes, FVM_UNSUPPORTED);
        bytes.push_back((unsigned char)v);
    }
    void u32(unsigned long v)
    {
        require(v <= 0xffffffffUL, FVM_UNSUPPORTED);
        for (int i = 24; i >= 0; i -= 8)
            byte(v >> i);
    }
    void ref(size_t id)
    {
        u32(id == none ? 0xffffffffUL : (unsigned long)id);
    }
    void string(const fvm_detail::string &s)
    {
        u32((unsigned long)s.size());
        for (size_t i = 0; i < s.size(); ++i)
            byte((unsigned char)s[i]);
    }
    void value(const Value &v)
    {
        byte(v.type);
        if (v.type == FVM_VALUE_BOOLEAN)
            byte(v.boolean ? 1 : 0);
        if (v.type == FVM_VALUE_STRING)
            string(v.string);
        if (v.type == FVM_VALUE_OBJECT)
            ref(v.object);
        if (v.type == FVM_VALUE_NUMBER)
        {
            unsigned char raw[8];
            bool little = little_double();
            std::memcpy(raw, &v.number, 8);
            for (size_t i = 0; i < 8; ++i)
                byte(raw[little ? 7 - i : i]);
        }
        ref(v.metatable);
    }
    void number(double d)
    {
        Value v;
        v.type = FVM_VALUE_NUMBER;
        v.number = d;
        value(v);
    }
    void text_style(const FVM_TextStyleState &s)
    {
        string(s.font);
        number(s.size);
        number(s.leading);
        u32(s.color);
        u32(s.alignment);
        require(s.source.index >= (-2147483647L - 1L) && s.source.index <= 2147483647L,
                FVM_UNSUPPORTED);
        u32(s.source.index < 0 ? (unsigned long)(s.source.index + 2147483647L) + 1UL
                               : 0x80000000UL + (unsigned long)s.source.index);
        u32((unsigned long)s.source.bytes.size());
        for (size_t i = 0; i < s.source.bytes.size(); ++i)
            byte(s.source.bytes[i]);
    }
    void text_field(const FVM_TextFieldState &s)
    {
        text_style(s.current);
        text_style(s.future);
        string(s.text);
        number(s.width);
        number(s.height);
        byte(s.multiline ? 1 : 0);
        byte(s.wrap ? 1 : 0);
    }
    void callback(const Callback &v)
    {
        u32(v.builtin);
        string(v.name);
        ref(v.root);
        byte(v.root_userdata ? 1 : 0);
    }
};
void write_graph(const Graph &g, Writer &w)
{
    w.value(g.value);
    w.u32((unsigned long)g.objects.size());
    for (size_t i = 0; i < g.objects.size(); ++i)
    {
        const Object &o = g.objects[i];
        w.u32(o.kind);
        w.value(o.state);
        if (o.kind == 1002)
            w.text_field(o.text);
        w.ref(o.prototype);
        w.ref(o.metatable);
        w.u32((unsigned long)o.properties.size());
        for (fvm_detail::map<fvm_detail::string, Value>::iterator p = o.properties.begin();
             p != o.properties.end(); ++p)
        {
            w.string(p->first);
            w.value(p->second);
        }
        w.u32((unsigned long)o.indexed.size());
        for (fvm_detail::map<unsigned long, Value>::iterator p = o.indexed.begin();
             p != o.indexed.end(); ++p)
        {
            w.u32(p->first);
            w.value(p->second);
        }
        w.u32((unsigned long)o.methods.size());
        for (fvm_detail::map<fvm_detail::string, Callback>::iterator p = o.methods.begin();
             p != o.methods.end(); ++p)
        {
            w.string(p->first);
            w.callback(p->second);
        }
        w.u32((unsigned long)o.metamethods.size());
        for (fvm_detail::map<fvm_detail::string, Callback>::iterator p = o.metamethods.begin();
             p != o.metamethods.end(); ++p)
        {
            w.string(p->first);
            w.callback(p->second);
        }
        w.u32((unsigned long)o.accessors.size());
        for (fvm_detail::map<fvm_detail::string, Accessor>::iterator p = o.accessors.begin();
             p != o.accessors.end(); ++p)
        {
            w.string(p->first);
            w.callback(p->second.getter);
            w.callback(p->second.setter);
        }
        w.u32((unsigned long)o.attributes.size());
        for (fvm_detail::map<fvm_detail::string, unsigned int>::iterator p = o.attributes.begin();
             p != o.attributes.end(); ++p)
        {
            w.string(p->first);
            w.u32(p->second);
        }
        w.callback(o.invocable);
        w.callback(o.constructable);
        w.u32((unsigned long)o.method_values.size());
        for (fvm_detail::map<fvm_detail::string, Value>::iterator p = o.method_values.begin();
             p != o.method_values.end(); ++p)
        {
            w.string(p->first);
            w.value(p->second);
        }
        w.u32((unsigned long)o.watchers.size());
        for (fvm_detail::map<fvm_detail::string, Value>::iterator p = o.watchers.begin();
             p != o.watchers.end(); ++p)
        {
            w.string(p->first);
            w.value(p->second);
        }
    }
}
struct Reader
{
    const unsigned char *data;
    size_t size, offset, entries;
    Reader(const void *p, size_t n) : data((const unsigned char *)p), size(n), offset(0), entries(0)
    {
    }
    unsigned long byte()
    {
        require(offset < size);
        return data[offset++];
    }
    unsigned long u32()
    {
        unsigned long v = 0;
        for (int i = 0; i < 4; ++i)
            v = (v << 8) | byte();
        return v;
    }
    size_t ref()
    {
        unsigned long v = u32();
        return v == 0xffffffffUL ? none : (size_t)v;
    }
    size_t count()
    {
        size_t n = (size_t)u32();
        require(n <= max_entries - entries && n <= size - offset);
        entries += n;
        return n;
    }
    fvm_detail::string string()
    {
        size_t n = (size_t)u32();
        require(n <= size - offset);
        require(!n || !std::memchr(data + offset, 0, n));
        fvm_detail::string s((const char *)data + offset, n);
        offset += n;
        return s;
    }
    Value value()
    {
        Value v;
        unsigned long type = byte();
        require(type <= FVM_VALUE_OBJECT);
        v.type = (FVM_ValueType)type;
        if (v.type == FVM_VALUE_BOOLEAN)
        {
            unsigned long b = byte();
            require(b <= 1);
            v.boolean = (int)b;
        }
        if (v.type == FVM_VALUE_STRING)
            v.string = string();
        if (v.type == FVM_VALUE_OBJECT)
            v.object = ref();
        if (v.type == FVM_VALUE_NUMBER)
        {
            unsigned char raw[8];
            bool little = little_double();
            for (size_t i = 0; i < 8; ++i)
                raw[little ? 7 - i : i] = (unsigned char)byte();
            std::memcpy(&v.number, raw, 8);
        }
        v.metatable = ref();
        require(v.type != FVM_VALUE_OBJECT || v.metatable == none);
        return v;
    }
    double number()
    {
        Value v = value();
        require(v.type == FVM_VALUE_NUMBER && v.metatable == none);
        return v.number;
    }
    FVM_TextStyleState text_style()
    {
        FVM_TextStyleState s;
        s.font = string();
        s.size = number();
        s.leading = number();
        s.color = u32();
        s.alignment = u32();
        unsigned long index = u32();
        s.source.index =
            index >= 0x80000000UL ? (long)(index - 0x80000000UL) : (long)index - 2147483647L - 1L;
        size_t length = (size_t)u32();
        require(length <= size - offset);
        s.source.bytes.resize(length);
        if (length)
            std::memcpy(s.source.bytes.begin(), data + offset, length);
        offset += length;
        return s;
    }
    FVM_TextFieldState text_field()
    {
        FVM_TextFieldState s;
        s.current = text_style();
        s.future = text_style();
        s.text = string();
        s.width = number();
        s.height = number();
        unsigned long multiline = byte(), wrap = byte();
        require(multiline <= 1 && wrap <= 1);
        s.multiline = multiline != 0;
        s.wrap = wrap != 0;
        return s;
    }
    Callback callback()
    {
        Callback v;
        v.builtin = u32();
        v.name = string();
        v.root = ref();
        unsigned long mode = byte();
        require(mode <= 1);
        v.root_userdata = mode != 0;
        require(!v.builtin || v.name.empty() ||
                fvm_text_callback_userdata(v.builtin, v.name.c_str()));
        require(v.builtin < 201 || v.builtin > 203 || !v.name.empty());
        require(!v.root_userdata || v.root != none);
        require(v.builtin || !v.name.empty() || (v.root == none && !v.root_userdata));
        return v;
    }
};
void validate_ref(size_t ref, size_t count)
{
    require(ref == none || ref < count);
}
void validate_value(const Value &v, size_t count)
{
    validate_ref(v.metatable, count);
    if (v.type == FVM_VALUE_OBJECT)
        require(v.object < count);
}
void validate_callback(const Callback &v, size_t count)
{
    validate_ref(v.root, count);
}
Graph read_graph(Reader &r, unsigned int version)
{
    Graph g;
    g.value = r.value();
    size_t n = (size_t)r.u32();
    require(n <= max_objects && n <= r.size - r.offset);
    for (size_t i = 0; i < n; ++i)
    {
        Object o;
        o.kind = r.u32();
        require(known_kind(o.kind), FVM_UNSUPPORTED);
        o.state = r.value();
        require(o.state.metatable == none);
        if (o.kind == 1002)
            o.text = r.text_field();
        o.prototype = r.ref();
        o.metatable = r.ref();
        if (o.kind == 4001 || o.kind == 4008)
            require(o.state.type == FVM_VALUE_STRING && o.state.string.size() <= 512);
        else if (o.kind == 20 || o.kind == 24 || o.kind == 26 || o.kind == 28 || o.kind == 32 ||
                 o.kind == 34 || o.kind == 40)
            require(o.state.type == (o.kind == 20                     ? FVM_VALUE_BOOLEAN
                                     : o.kind == 26                   ? FVM_VALUE_STRING
                                     : (o.kind == 32 || o.kind == 40) ? FVM_VALUE_OBJECT
                                                                      : FVM_VALUE_NUMBER));
        else if (o.kind == 10 || o.kind == 3001 || o.kind == 3008)
            require(o.state.type == FVM_VALUE_NUMBER && o.state.number >= 0 &&
                    o.state.number <= 4294967295.0 &&
                    o.state.number == (unsigned long)o.state.number);
        else
            require(o.state.type == FVM_VALUE_UNDEFINED);
        if (o.kind == 26)
            require(std::strlen(o.state.string.c_str()) == o.state.string.size() &&
                    fvm_utf8_validate(o.state.string.c_str(), o.state.string.size(), 0) ==
                        FVM_UTF8_OK);
        size_t count = r.count();
        for (size_t j = 0; j < count; ++j)
        {
            fvm_detail::string name = r.string();
            require(o.properties.find(name) == o.properties.end());
            o.properties[name] = r.value();
        }
        count = r.count();
        for (size_t j = 0; j < count; ++j)
        {
            unsigned long key = r.u32();
            require(o.indexed.find(key) == o.indexed.end());
            o.indexed[key] = r.value();
        }
        count = r.count();
        for (size_t j = 0; j < count; ++j)
        {
            fvm_detail::string name = r.string();
            require(o.methods.find(name) == o.methods.end());
            Callback cb = r.callback();
            require(cb.builtin || !cb.name.empty());
            o.methods[name] = cb;
        }
        count = r.count();
        for (size_t j = 0; j < count; ++j)
        {
            fvm_detail::string name = r.string();
            require(o.metamethods.find(name) == o.metamethods.end());
            Callback cb = r.callback();
            require(cb.builtin || !cb.name.empty());
            o.metamethods[name] = cb;
        }
        count = r.count();
        for (size_t j = 0; j < count; ++j)
        {
            fvm_detail::string name = r.string();
            require(o.accessors.find(name) == o.accessors.end());
            Accessor a;
            a.getter = r.callback();
            a.setter = r.callback();
            require(a.getter.builtin || !a.getter.name.empty() || a.setter.builtin ||
                    !a.setter.name.empty());
            require(a.getter.root == none || a.setter.root == none ||
                    a.getter.root == a.setter.root);
            o.accessors[name] = a;
        }
        if (version >= 2)
        {
            count = r.count();
            for (size_t j = 0; j < count; ++j)
            {
                fvm_detail::string name = r.string();
                unsigned long flags = r.u32();
                require(flags <= 7 && o.attributes.find(name) == o.attributes.end());
                o.attributes[name] = (unsigned int)flags;
            }
        }
        if (version >= 2)
        {
            o.invocable = r.callback();
            o.constructable = r.callback();
            count = r.count();
            for (size_t j = 0; j < count; ++j)
            {
                fvm_detail::string name = r.string();
                Value method = r.value();
                require(method.type == FVM_VALUE_OBJECT &&
                        o.method_values.find(name) == o.method_values.end());
                o.method_values[name] = method;
            }
        }
        if (version >= 3)
        {
            count = r.count();
            for (size_t j = 0; j < count; ++j)
            {
                fvm_detail::string name = r.string();
                Value descriptor = r.value();
                require(!name.empty() && descriptor.type == FVM_VALUE_OBJECT &&
                        o.watchers.find(name) == o.watchers.end());
                o.watchers[name] = descriptor;
            }
        }
        if (o.kind == 34)
            require(o.state.number != o.state.number ||
                    (std::fabs(o.state.number) <= 8640000000000000.0 &&
                     o.state.number == std::floor(o.state.number)));
        if (o.kind == 28)
        {
            require(o.state.number >= 0 && o.state.number <= 4294967295.0 &&
                    o.state.number == std::floor(o.state.number));
            for (fvm_detail::map<unsigned long, Value>::iterator slot = o.indexed.begin();
                 slot != o.indexed.end(); ++slot)
                require(slot->first < o.state.number);
        }
        g.objects.push_back(o);
    }
    require(r.offset == r.size);
    validate_value(g.value, n);
    for (size_t i = 0; i < n; ++i)
    {
        const Object &o = g.objects[i];
        validate_ref(o.prototype, n);
        validate_ref(o.metatable, n);
        validate_value(o.state, n);
        validate_callback(o.invocable, n);
        validate_callback(o.constructable, n);
        for (fvm_detail::map<fvm_detail::string, Value>::iterator p = o.method_values.begin();
             p != o.method_values.end(); ++p)
            validate_value(p->second, n);
        for (fvm_detail::map<fvm_detail::string, Value>::iterator p = o.watchers.begin();
             p != o.watchers.end(); ++p)
            validate_value(p->second, n);
        for (fvm_detail::map<fvm_detail::string, Value>::iterator p = o.properties.begin();
             p != o.properties.end(); ++p)
            validate_value(p->second, n);
        for (fvm_detail::map<unsigned long, Value>::iterator p = o.indexed.begin();
             p != o.indexed.end(); ++p)
            validate_value(p->second, n);
        for (fvm_detail::map<fvm_detail::string, Callback>::iterator p = o.methods.begin();
             p != o.methods.end(); ++p)
            validate_callback(p->second, n);
        for (fvm_detail::map<fvm_detail::string, Callback>::iterator p = o.metamethods.begin();
             p != o.metamethods.end(); ++p)
            validate_callback(p->second, n);
        for (fvm_detail::map<fvm_detail::string, Accessor>::iterator p = o.accessors.begin();
             p != o.accessors.end(); ++p)
        {
            validate_callback(p->second.getter, n);
            validate_callback(p->second.setter, n);
        }
    }
    /* Watch descriptors are private data records, never opaque host pointers. */
    for (size_t i = 0; i < n; ++i)
        for (fvm_detail::map<fvm_detail::string, Value>::iterator watcher =
                 g.objects[i].watchers.begin();
             watcher != g.objects[i].watchers.end(); ++watcher)
        {
            const Object &descriptor = g.objects[watcher->second.object];
            require(descriptor.kind == 0 && descriptor.prototype == none &&
                    descriptor.metatable == none);
            fvm_detail::map<fvm_detail::string, Value>::iterator handler =
                descriptor.properties.find("handler");
            require(handler != descriptor.properties.end() &&
                    handler->second.type == FVM_VALUE_OBJECT);
            const Object &function = g.objects[handler->second.object];
            require(function.invocable.builtin || !function.invocable.name.empty());
            require(descriptor.properties.find("userData") != descriptor.properties.end());
        }
    /* Validate every edge before following any prototype chain. */
    for (size_t i = 0; i < n; ++i)
    {
        size_t cursor = i, depth = 0;
        while (cursor != none && depth++ <= n)
            cursor = g.objects[cursor].prototype;
        require(cursor == none);
    }
    return g;
}
const unsigned char magic[8] = {'F', 'V', 'M', 'G', 0, 0, 0, 3};
Graph decode(const void *data, size_t size)
{
    require(data && size >= 16 && size <= max_bytes);
    require(!std::memcmp(data, magic, 7));
    unsigned int version = ((const unsigned char *)data)[7];
    require(version >= 1 && version <= 3);
    Reader header(data, size);
    header.offset = 8;
    size_t body = (size_t)header.u32();
    unsigned long crc = header.u32();
    require(body == size - 16);
    const unsigned char *bytes = (const unsigned char *)data + 16;
    require(crc == (unsigned long)crc32(0, bytes, (uInt)body));
    Reader reader(bytes, body);
    return read_graph(reader, version);
}
void encode(FVM_Context *c, const FVM_Value &value, unsigned char **data, size_t *size)
{
    Graph graph = capture(c, value);
    Writer body;
    write_graph(graph, body);
    require(body.bytes.size() <= max_bytes - 16, FVM_UNSUPPORTED);
    Writer header;
    for (size_t i = 0; i < 8; ++i)
        header.byte(magic[i]);
    header.u32((unsigned long)body.bytes.size());
    header.u32((unsigned long)crc32(0, body.bytes.begin(), (uInt)body.bytes.size()));
    unsigned char *p = (unsigned char *)std::malloc(body.bytes.size() + 16);
    require(p != 0, FVM_OUT_OF_MEMORY);
    std::memcpy(p, header.bytes.begin(), 16);
    std::memcpy(p + 16, body.bytes.begin(), body.bytes.size());
    *data = p;
    *size = body.bytes.size() + 16;
}
FVM_Value restore_value(FVM_Context *c, const Value &src,
                        const fvm_detail::vector<FVM_Object *> &objects)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = src.type;
    if (src.metatable != none)
        v.metatable = objects[src.metatable];
    if (src.type == FVM_VALUE_NUMBER)
        v.number = src.number;
    if (src.type == FVM_VALUE_BOOLEAN)
        v.boolean = src.boolean;
    if (src.type == FVM_VALUE_OBJECT)
        v.object = objects[src.object];
    if (src.type == FVM_VALUE_STRING)
    {
        char *p = new char[src.string.size() + 1];
        std::memcpy(p, src.string.c_str(), src.string.size() + 1);
        try
        {
            c->strings.push_back(p);
        }
        catch (...)
        {
            delete[] p;
            throw;
        }
        v.string = p;
    }
    return v;
}
FVM_NativeEntry restore_callback(FVM_Context *c, const Callback &src,
                                 const fvm_detail::vector<FVM_Object *> &objects)
{
    FVM_NativeEntry dst;
    if (src.builtin)
    {
        dst.callback = fvm_numeric_callback_from_id(src.builtin);
        if (!dst.callback)
            dst.callback = fvm_text_callback_from_id(src.builtin);
        if (!dst.callback)
            dst.callback = fvm_error_callback_from_id(src.builtin);
        if (!dst.callback)
            dst.callback = fvm_number_callback_from_id(src.builtin);
        if (!dst.callback)
            dst.callback = fvm_string_callback_from_id(src.builtin);
        if (!dst.callback)
            dst.callback = fvm_base_callback_from_id(src.builtin);
        if (!dst.callback)
            dst.callback = fvm_date_callback_from_id(src.builtin);
        if (!dst.callback)
            dst.callback = fvm_function_callback_from_id(src.builtin);
        if (!dst.callback)
            dst.callback = fvm_movieclip_callback_from_id(src.builtin);
        require(dst.callback != 0, FVM_UNSUPPORTED);
    }
    else if (!src.name.empty())
    {
        fvm_detail::map<fvm_detail::string, FVM_NativeFunction>::iterator p =
            c->serialization_callbacks.find(src.name);
        require(p != c->serialization_callbacks.end(), FVM_UNSUPPORTED);
        dst.callback = p->second;
    }
    if (src.builtin >= 201 && src.builtin <= 203)
    {
        dst.userdata = fvm_text_callback_userdata(src.builtin, src.name.c_str());
        require(dst.userdata && src.root == none && !src.root_userdata);
    }
    if (src.root != none)
        dst.root = objects[src.root];
    if (src.root_userdata)
        dst.userdata = dst.root;
    dst.collector_entry = fvm_function_collector_callback(dst.callback);
    return dst;
}
void restore(FVM_Context *c, const Graph &g, FVM_Value *out)
{
    const size_t old_objects = c->objects.size(), old_strings = c->strings.size();
    const unsigned long old_allocations = c->gc_allocations;
    fvm_detail::map<fvm_detail::string, FVM_FontSource *> old_fonts = c->fonts;
    fvm_detail::string old_default_font = c->default_font;
    fvm_detail::vector<FVM_Object *> objects;
    try
    {
        for (size_t i = 0; i < g.objects.size(); ++i)
        {
            FVM_Object *o = 0;
            const Object &src = g.objects[i];
            FVM_Result r;
            if (src.kind == 3001 || src.kind == 3008 || src.kind == 4001 || src.kind == 4008)
            {
                require(src.properties.empty() && src.indexed.empty() && src.methods.empty() &&
                        src.metamethods.empty() && src.accessors.empty() && src.prototype == none &&
                        src.metatable == none);
                r = src.kind >= 4000
                        ? fvm_capability_import(c, src.state.string.c_str(), src.kind - 4000, &o)
                        : fvm_transfer_restore_resource(c, src.kind - 3000,
                                                        (unsigned long)src.state.number, &o);
            }
            else
                r = fvm_object_create(c, &o);
            require(r == FVM_OK, r);
            if (src.kind == 4001 || src.kind == 4008)
                o->resource_path = c->resource_receive_path;
            objects.push_back(o);
        }
        bool random = false;
        unsigned long random_state = 0;
        for (size_t i = 0; i < objects.size(); ++i)
        {
            const Object &src = g.objects[i];
            FVM_Object *dst = objects[i];
            if (src.kind == 3001 || src.kind == 3008 || src.kind == 4001 || src.kind == 4008)
                continue;
            dst->resource_kind = (int)src.kind;
            if (src.kind == 1002)
            {
                FVM_Result result = fvm_textfield_restore(c, dst, src.text);
                require(result == FVM_OK, result);
            }
            if (src.kind == 20 || src.kind == 24 || src.kind == 26 || src.kind == 28 ||
                src.kind == 34 || src.kind == 32 || src.kind == 40)
            {
                dst->resource_result_valid = true;
                dst->resource_result = restore_value(c, src.state, objects);
            }
            if (src.kind == 10)
            {
                unsigned long state = (unsigned long)src.state.number;
                require(!random || state == random_state);
                random = true;
                random_state = state;
            }
            dst->invocable = restore_callback(c, src.invocable, objects);
            dst->constructable = restore_callback(c, src.constructable, objects);
            dst->invocable.collector_entry =
                fvm_function_collector_callback(dst->invocable.callback);
            for (fvm_detail::map<fvm_detail::string, Value>::iterator p = src.method_values.begin();
                 p != src.method_values.end(); ++p)
                dst->method_values[p->first] = restore_value(c, p->second, objects).object;
            for (fvm_detail::map<fvm_detail::string, Value>::iterator p = src.watchers.begin();
                 p != src.watchers.end(); ++p)
                dst->watchers[p->first] = restore_value(c, p->second, objects).object;
            dst->attributes = src.attributes;
            if (src.prototype != none)
                dst->prototype = objects[src.prototype];
            if (src.metatable != none)
                dst->metatable = objects[src.metatable];
            for (fvm_detail::map<fvm_detail::string, Value>::iterator p = src.properties.begin();
                 p != src.properties.end(); ++p)
                dst->properties[p->first] = restore_value(c, p->second, objects);
            for (fvm_detail::map<unsigned long, Value>::iterator p = src.indexed.begin();
                 p != src.indexed.end(); ++p)
                dst->indexed[p->first] = restore_value(c, p->second, objects);
            for (fvm_detail::map<fvm_detail::string, Callback>::iterator p = src.methods.begin();
                 p != src.methods.end(); ++p)
                dst->methods[p->first] = restore_callback(c, p->second, objects);
            for (fvm_detail::map<fvm_detail::string, Callback>::iterator p =
                     src.metamethods.begin();
                 p != src.metamethods.end(); ++p)
                dst->metamethods[p->first] = restore_callback(c, p->second, objects);
            for (fvm_detail::map<fvm_detail::string, Accessor>::iterator p = src.accessors.begin();
                 p != src.accessors.end(); ++p)
            {
                FVM_NativeEntry get = restore_callback(c, p->second.getter, objects),
                                set = restore_callback(c, p->second.setter, objects);
                require(!get.callback || !set.callback || get.userdata == set.userdata);
                FVM_AccessorEntry a;
                a.getter = get.callback;
                a.setter = set.callback;
                a.root = get.root ? get.root : set.root;
                a.userdata = get.callback ? get.userdata : set.userdata;
                dst->accessors[p->first] = a;
            }
        }
        FVM_Value result = restore_value(c, g.value, objects);
        if (random)
            c->random_state = random_state;
        *out = result;
    }
    catch (...)
    {
        /* No graph was published and no user finalizer may run on failed input. */
        for (size_t i = old_objects; i < c->objects.size(); ++i)
        {
            if (c->objects[i]->release_resource)
                c->objects[i]->release_resource(c->objects[i]->resource);
            delete c->objects[i];
        }
        for (fvm_detail::map<fvm_detail::string, FVM_FontSource *>::iterator i = c->fonts.begin();
             i != c->fonts.end(); ++i)
            if (old_fonts.find(i->first) == old_fonts.end())
                delete i->second;
        c->fonts.swap(old_fonts);
        c->default_font.swap(old_default_font);
        c->objects.resize(old_objects);
        for (size_t i = old_strings; i < c->strings.size(); ++i)
            delete[] c->strings[i];
        c->strings.resize(old_strings);
        c->gc_allocations = old_allocations;
        throw;
    }
}
} // namespace
extern "C" FVM_Result fvm_serialization_register_callback(FVM_Context *c, const char *name,
                                                          FVM_NativeFunction fn)
{
    if (!c || !name || !*name || std::strlen(name) > 255 || !fn || c->destroying ||
        c->gc_collecting || c->gc_releasing || !std::strncmp(name, "flashvm.", 8))
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::map<fvm_detail::string, FVM_NativeFunction>::iterator p =
            c->serialization_callbacks.find(name);
        if (p != c->serialization_callbacks.end() && p->second != fn)
            return FVM_INVALID_ARGUMENT;
        c->serialization_callbacks[name] = fn;
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" FVM_Result fvm_value_serialize(FVM_Context *c, const FVM_Value *v, unsigned char **data,
                                          size_t *size)
{
    if (!data || !size)
        return FVM_INVALID_ARGUMENT;
    *data = 0;
    *size = 0;
    if (!c || !v || c->destroying || c->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_NativeCallGuard guard(c);
        encode(c, *v, data, size);
        return FVM_OK;
    }
    catch (const Failure &f)
    {
        return f.result;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" FVM_Result fvm_value_deserialize(FVM_Context *c, const void *data, size_t size,
                                            FVM_Value *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    if (!c || c->destroying || c->gc_collecting || c->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_NativeCallGuard guard(c);
        Graph graph = decode(data, size);
        restore(c, graph, out);
        return FVM_OK;
    }
    catch (const Failure &f)
    {
        return f.result;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" void fvm_serialized_free(void *data)
{
    std::free(data);
}
/* Primitive entry points let the C89 file transport use the identical format. */
extern "C" FVM_Result fvm_serialization_encode_primitive(const FVM_Value *v, unsigned char **data,
                                                         size_t *size)
{
    if (!v || !data || !size)
        return FVM_INVALID_ARGUMENT;
    *data = 0;
    *size = 0;
    try
    {
        Graph g = capture(0, *v);
        require(g.objects.empty());
        encode(0, *v, data, size);
        return FVM_OK;
    }
    catch (const Failure &f)
    {
        return f.result;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" FVM_Result fvm_serialization_decode_primitive(const void *data, size_t size,
                                                         FVM_Value *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    try
    {
        Graph g = decode(data, size);
        require(g.objects.empty(), FVM_UNSUPPORTED);
        Value &v = g.value;
        out->type = v.type;
        out->number = v.number;
        out->boolean = v.boolean;
        if (v.type == FVM_VALUE_STRING)
        {
            char *p = (char *)std::malloc(v.string.size() + 1);
            require(p != 0, FVM_OUT_OF_MEMORY);
            std::memcpy(p, v.string.c_str(), v.string.size() + 1);
            out->string = p;
        }
        return FVM_OK;
    }
    catch (const Failure &f)
    {
        fvm_value_undefined(out);
        return f.result;
    }
    catch (...)
    {
        fvm_value_undefined(out);
        return FVM_OUT_OF_MEMORY;
    }
}
