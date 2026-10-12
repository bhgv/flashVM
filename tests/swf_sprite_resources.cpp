#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/avm1.h"
#include "flashvm/transfer.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "flashvm/serialization.h"
#include "flashvm/exception.h"
#include "../src/details/vector.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
typedef fvm_detail::vector<unsigned char> Bytes;
static void u16(Bytes &b, unsigned int n) { b.push_back((unsigned char)n); b.push_back((unsigned char)(n >> 8)); }
static void u32(Bytes &b, unsigned long n) { for (int i = 0; i < 4; ++i) { b.push_back((unsigned char)n); n >>= 8; } }
static void name(Bytes &b, const char *s) { do { b.push_back((unsigned char)*s); } while (*s++); }
static void text(Bytes &b, const char *s) { b.push_back(0x96); u16(b, (unsigned int)std::strlen(s) + 2); b.push_back(0); name(b, s); }
static void variable(Bytes &b, const char *s) { text(b, s); b.push_back(0x1c); }
static void integer(Bytes &b, unsigned int n) { b.push_back(0x96); u16(b, 5); b.push_back(7); u32(b, n); }
static void tag(Bytes &b, unsigned int code, const Bytes &data)
{
    if (data.size() < 63) u16(b, (code << 6) | (unsigned int)data.size());
    else { u16(b, (code << 6) | 63); u32(b, (unsigned long)data.size()); }
    for (size_t i = 0; i < data.size(); ++i) b.push_back(data[i]);
}
static Bytes action(unsigned int n)
{
    Bytes b; variable(b, "this"); text(b, "seen"); integer(b, n); b.push_back(0x4f); b.push_back(0); return b;
}
static Bytes sprite(unsigned int id, unsigned int frames, const Bytes &controls)
{
    Bytes b; u16(b, id); u16(b, frames);
    for (size_t i = 0; i < controls.size(); ++i) b.push_back(controls[i]);
    return b;
}
static void export_asset(Bytes &tags, unsigned int id, const char *linkage)
{
    Bytes b; u16(b, 1); u16(b, id); name(b, linkage); tag(tags, 56, b);
}
static Bytes movie(const Bytes &tags, unsigned int frames = 2)
{
    Bytes b; b.push_back('F'); b.push_back('W'); b.push_back('S'); b.push_back(8);
    u32(b, (unsigned long)tags.size() + 14); b.push_back(8); b.push_back(0); u16(b, 12 * 256); u16(b, frames);
    for (size_t i = 0; i < tags.size(); ++i) b.push_back(tags[i]);
    return b;
}
static FVM_Object *attach(FVM_Context *c, FVM_Object *root, const char *linkage, const char *instance,
                          unsigned int depth, FVM_Object *initial = 0)
{
    FVM_Value a[4], out; fvm_value_string(a, linkage); fvm_value_string(a + 1, instance); fvm_value_number(a + 2, depth);
    fvm_value_undefined(a + 3); if (initial) { a[3].type = FVM_VALUE_OBJECT; a[3].object = initial; }
    assert(fvm_object_call(c, root, "attachMovie", initial ? 4 : 3, a, &out) == FVM_OK);
    return out.type == FVM_VALUE_OBJECT ? out.object : 0;
}
static FVM_Value get(FVM_Context *c, FVM_Object *o, const char *key)
{
    FVM_Value v; assert(fvm_object_get(c, o, key, &v) == FVM_OK); return v;
}
static void number(FVM_Context *c, FVM_Object *o, const char *key, double n)
{
    FVM_Value v = get(c, o, key); assert(v.type == FVM_VALUE_NUMBER && v.number == n);
}
static FVM_Value registered_constructor(FVM_Context *c, bool throwing = false)
{
    Bytes body;
    variable(body, "this"); text(body, "observed"); variable(body, "this"); text(body, "seed"); body.push_back(0x4e); body.push_back(0x4f);
    variable(body, "this"); text(body, "createdParent"); variable(body, "this"); text(body, "_parent"); body.push_back(0x4e); body.push_back(0x4f); body.push_back(0);
    if (throwing) { body.clear(); text(body, "attachment failed"); body.push_back(0x2a); body.push_back(0); }
    Bytes header; name(header, "HeroClass"); u16(header, 0); header.push_back(0); u16(header, 0); u16(header, (unsigned int)body.size());
    Bytes code; code.push_back(0x8e); u16(code, (unsigned int)header.size());
    for (size_t i = 0; i < header.size(); ++i) code.push_back(header[i]);
    for (size_t i = 0; i < body.size(); ++i) code.push_back(body[i]);
    code.push_back(0);
    assert(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK);
    FVM_Value ctor; assert(fvm_get_global(c, "HeroClass", &ctor) == FVM_OK);
    FVM_Value proto = get(c, ctor.object, "prototype"), klass; assert(fvm_get_global(c, "MovieClip", &klass) == FVM_OK);
    FVM_Value clip_proto = get(c, klass.object, "prototype");
    assert(fvm_object_set_prototype(c, proto.object, clip_proto.object) == FVM_OK);
    return ctor;
}
static void rejects(FVM_Context *c, const Bytes &tags, FVM_Result expected)
{
    Bytes swf = movie(tags, 1); FVM_Object *o = 0;
    assert(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "bad", &o) == expected && !o);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0, *other = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    Bytes controls, empty, label; name(label, "second"); tag(controls, 12, action(11)); tag(controls, 1, empty);
    tag(controls, 43, label); tag(controls, 12, action(22)); tag(controls, 1, empty); tag(controls, 0, empty);
    Bytes tags; tag(tags, 39, sprite(7, 2, controls)); export_asset(tags, 7, "OldHero"); export_asset(tags, 7, "Hero");
    tag(tags, 1, empty); tag(tags, 1, empty); tag(tags, 0, empty); Bytes swf = movie(tags);
    FVM_Object *root = 0; assert(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root) == FVM_OK);
    assert(fvm_gc_pin(c, root) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(!attach(c, root, "OldHero", "old", 9)); assert(!attach(c, root, "missing", "missing", 9));
    for (int i = 0; i < 5; ++i) {
        FVM_Value ignored_args[4], ignored_out;
        fvm_value_string(ignored_args, "Hero"); fvm_value_string(ignored_args + 1, "ignoredInitial"); fvm_value_number(ignored_args + 2, 20);
        fvm_value_undefined(ignored_args + 3);
        if (i == 1) ignored_args[3].type = FVM_VALUE_NULL;
        if (i == 2) fvm_value_number(ignored_args + 3, 31);
        if (i == 3) fvm_value_boolean(ignored_args + 3, 1);
        if (i == 4) fvm_value_string(ignored_args + 3, "not an object");
        assert(fvm_object_call(c, root, "attachMovie", 4, ignored_args, &ignored_out) == FVM_OK && ignored_out.type == FVM_VALUE_OBJECT);
        assert(get(c, ignored_out.object, "seed").type == FVM_VALUE_UNDEFINED);
    }
    FVM_Object *a = attach(c, root, "Hero", "a", 2), *b = attach(c, root, "Hero", "b", 3); assert(a && b && a != b);
    assert(get(c, a, "_parent").object == root); number(c, a, "_totalframes", 2);
    assert(get(c, a, "seen").type == FVM_VALUE_UNDEFINED);
    assert(fvm_movieclip_advance(c, root, 1) == FVM_OK); number(c, a, "seen", 11); number(c, b, "seen", 11);
    FVM_Value out; assert(fvm_object_call(c, a, "stop", 0, 0, &out) == FVM_OK);
    assert(fvm_movieclip_advance(c, root, 1) == FVM_OK); number(c, a, "seen", 11); number(c, b, "seen", 22);
    Bytes changed = action(99); assert(fvm_movieclip_define_frame(c, a, 2, "second", changed.begin(), changed.size()) == FVM_OK);
    FVM_Value frame; fvm_value_string(&frame, "second"); assert(fvm_object_call(c, a, "gotoAndStop", 1, &frame, &out) == FVM_OK);
    assert(fvm_movieclip_advance(c, a, 1) == FVM_OK); number(c, a, "seen", 99);
    assert(fvm_movieclip_advance(c, b, 2) == FVM_OK); number(c, b, "seen", 22);
    FVM_Object *replacement = attach(c, root, "Hero", "replacement", 2); assert(replacement);
    assert(get(c, a, "_parent").type != FVM_VALUE_OBJECT);
    assert(get(c, root, "a").type == FVM_VALUE_UNDEFINED); assert(get(c, root, "replacement").object == replacement);
    assert(fvm_movieclip_advance(c, replacement, 1) == FVM_OK); number(c, replacement, "seen", 11);
    FVM_Value child_args[2]; fvm_value_string(child_args, "empty"); fvm_value_number(child_args + 1, 8);
    assert(fvm_object_call(c, root, "createEmptyMovieClip", 2, child_args, &out) == FVM_OK); FVM_Object *holder = out.object;
    FVM_Object *nested = attach(c, holder, "Hero", "nested", 1); assert(nested); assert(get(c, nested, "_root").object == root);
    FVM_Object *initial = 0, *inherited = 0; assert(fvm_object_create(c, &initial) == FVM_OK); assert(fvm_object_create(c, &inherited) == FVM_OK);
    FVM_Value value; fvm_value_number(&value, 31); assert(fvm_object_set(c, initial, "seed", &value) == FVM_OK);
    fvm_value_number(&value, 12); assert(fvm_object_set(c, inherited, "inherited", &value) == FVM_OK);
    assert(fvm_object_set_prototype(c, initial, inherited) == FVM_OK);
    assert(fvm_object_set(c, initial, "hidden", &value) == FVM_OK);
    assert(fvm_object_set_property_flags(c, initial, "hidden", FVM_PROPERTY_DONT_ENUM, 0) == FVM_OK);
    FVM_Value ctor = registered_constructor(c), klass, registration[2]; assert(fvm_get_global(c, "Object", &klass) == FVM_OK);
    fvm_value_string(registration, "Hero"); registration[1] = ctor;
    assert(fvm_object_call(c, klass.object, "registerClass", 2, registration, &out) == FVM_OK && out.boolean);
    FVM_Object *custom = attach(c, root, "Hero", "custom", 4, initial); assert(custom);
    number(c, custom, "seed", 31); number(c, custom, "observed", 31); number(c, custom, "inherited", 12);
    assert(get(c, custom, "createdParent").object == root); assert(get(c, custom, "hidden").type == FVM_VALUE_UNDEFINED);
    assert(get(c, custom, "seen").type == FVM_VALUE_UNDEFINED);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    FVM_Channel *ch = 0; assert(fvm_channel_create(&ch) == FVM_OK); fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = root;
    assert(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK); assert(fvm_channel_receive_value(other, ch, &out, 0) == FVM_OK);
    FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK);
    FVM_Object *copy_init = 0; assert(fvm_object_create(other, &copy_init) == FVM_OK); fvm_value_number(&value, 52);
    assert(fvm_object_set(other, copy_init, "seed", &value) == FVM_OK);
    FVM_Object *copy_child = attach(other, copy, "Hero", "newRemote", 10, copy_init); assert(copy_child);
    number(other, copy_child, "observed", 52); assert(get(other, copy_child, "createdParent").object == copy);
    assert(fvm_movieclip_advance(other, copy_child, 1) == FVM_OK); number(other, copy_child, "seen", 11);
    fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = root;
    unsigned char *snapshot = 0; size_t snapshot_size = 0;
    assert(fvm_value_serialize(c, &value, &snapshot, &snapshot_size) == FVM_OK);
    assert(fvm_value_deserialize(other, snapshot, snapshot_size, &out) == FVM_OK);
    fvm_serialized_free(snapshot); FVM_Object *restored = out.object;
    assert(attach(other, restored, "Hero", "fromSnapshot", 14, copy_init));
    FVM_Channel *file = 0;
    FVM_Result file_result = fvm_channel_open_file("flashvm-sprite-test.bin", "w", &file);
    if (file_result == FVM_OK) {
        assert(fvm_channel_send_value(c, file, &value, 0) == FVM_OK); fvm_channel_destroy(file);
        assert(fvm_channel_open_file("flashvm-sprite-test.bin", "r", &file) == FVM_OK);
        assert(fvm_channel_receive_value(other, file, &out, 0) == FVM_OK); fvm_channel_destroy(file);
        assert(std::remove("flashvm-sprite-test.bin") == 0); restored = out.object;
        FVM_Object *from_file = attach(other, restored, "Hero", "fromFile", 14, copy_init); assert(from_file);
        number(other, from_file, "observed", 52);
        assert(fvm_movieclip_advance(other, from_file, 1) == FVM_OK); number(other, from_file, "seen", 11);
    } else assert(file_result == FVM_UNSUPPORTED);
    FVM_Value throwing_ctor = registered_constructor(c, true); registration[1] = throwing_ctor;
    assert(fvm_object_call(c, klass.object, "registerClass", 2, registration, &out) == FVM_OK);
    FVM_Value failure_args[3]; fvm_value_string(failure_args, "Hero"); fvm_value_string(failure_args + 1, "failedChild"); fvm_value_number(failure_args + 2, 17);
    assert(fvm_object_call(c, root, "attachMovie", 3, failure_args, &out) == FVM_EXCEPTION);
    assert(fvm_exception_get(c, &out) == FVM_OK && out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "attachment failed"));
    assert(fvm_exception_clear(c) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(get(c, root, "failedChild").type == FVM_VALUE_OBJECT);
    registration[1] = ctor; assert(fvm_object_call(c, klass.object, "registerClass", 2, registration, &out) == FVM_OK);
    FVM_Object *after_throw = attach(c, root, "Hero", "afterThrow", 18); assert(after_throw);
    assert(get(c, after_throw, "createdParent").object == root);
    fvm_value_undefined(registration + 1); assert(fvm_object_call(c, klass.object, "registerClass", 2, registration, &out) == FVM_OK && out.boolean);
    FVM_Object *plain = attach(c, root, "Hero", "plain", 11); assert(plain); assert(get(c, plain, "observed").type == FVM_VALUE_UNDEFINED);
    Bytes bad, truncated; truncated.push_back(1); tag(bad, 39, sprite(1, 1, truncated)); tag(bad, 1, empty); tag(bad, 0, empty); rejects(c, bad, FVM_INVALID_ARGUMENT);
    Bytes one; tag(one, 1, empty); tag(one, 0, empty); bad.clear(); tag(bad, 39, sprite(1, 2, one)); tag(bad, 1, empty); tag(bad, 0, empty); rejects(c, bad, FVM_INVALID_ARGUMENT);
    bad.clear(); tag(bad, 39, sprite(1, 1, one)); tag(bad, 39, sprite(1, 1, one)); tag(bad, 1, empty); tag(bad, 0, empty); rejects(c, bad, FVM_INVALID_ARGUMENT);
    bad.clear(); export_asset(bad, 9, "unknown"); tag(bad, 1, empty); tag(bad, 0, empty); rejects(c, bad, FVM_INVALID_ARGUMENT);
    Bytes nested_definition; tag(nested_definition, 39, sprite(2, 1, one)); tag(nested_definition, 1, empty); tag(nested_definition, 0, empty);
    bad.clear(); tag(bad, 39, sprite(1, 1, nested_definition)); tag(bad, 1, empty); tag(bad, 0, empty); rejects(c, bad, FVM_INVALID_ARGUMENT);
    fvm_channel_destroy(ch); fvm_context_destroy(other); fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
