#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/serialization.h"
#include "flashvm/gc.h"
#include "../src/details/vector.hpp"
#include "zlib.h"
#include <cassert>
#include <cstring>
#include <cstdio>
typedef fvm_detail::vector<unsigned char> Bytes;
static void u16(Bytes &b, unsigned int n) { b.push_back((unsigned char)n); b.push_back((unsigned char)(n >> 8)); }
static void u32(Bytes &b, unsigned long n) { for (int i = 0; i < 4; ++i) { b.push_back((unsigned char)n); n >>= 8; } }
static void text(Bytes &b, const char *s) { b.push_back(0x96); u16(b, (unsigned int)std::strlen(s) + 2); b.push_back(0); do { b.push_back((unsigned char)*s); } while (*s++); }
static void tag(Bytes &b, unsigned int code, const Bytes &data)
{
    if (data.size() < 63) u16(b, (code << 6) | (unsigned int)data.size());
    else { u16(b, (code << 6) | 63); u32(b, (unsigned long)data.size()); }
    for (size_t i = 0; i < data.size(); ++i) b.push_back(data[i]);
}
static Bytes action(unsigned int n)
{
    Bytes b; text(b, "this"); b.push_back(0x1c); text(b, "seen");
    b.push_back(0x96); u16(b, 5); b.push_back(7); u32(b, n); b.push_back(0x4f); b.push_back(0); return b;
}
static Bytes movie(const Bytes &tags, unsigned int frames)
{
    Bytes b; b.push_back('F'); b.push_back('W'); b.push_back('S'); b.push_back(8);
    u32(b, (unsigned long)tags.size() + 14); b.push_back(8); b.push_back(0);
    u16(b, 12 * 256); u16(b, frames);
    for (size_t i = 0; i < tags.size(); ++i) b.push_back(tags[i]);
    return b;
}
static void number(FVM_Context *c, FVM_Object *clip, const char *key, double expected)
{
    FVM_Value v; assert(fvm_object_get(c, clip, key, &v) == FVM_OK);
    if (v.type != FVM_VALUE_NUMBER || v.number != expected) std::fprintf(stderr, "%s: type %d value %g expected %g\n", key, (int)v.type, v.number, expected);
    assert(v.type == FVM_VALUE_NUMBER && v.number == expected);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0, *other = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    Bytes tags, empty, label; const char *name = "second";
    do { label.push_back((unsigned char)*name); } while (*name++);
    tag(tags, 12, action(1)); tag(tags, 12, action(2)); tag(tags, 1, empty);
    tag(tags, 43, label); tag(tags, 12, action(3)); tag(tags, 1, empty); tag(tags, 0, empty);
    Bytes swf = movie(tags, 2); FVM_Object *clip = 0;
    assert(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &clip) == FVM_OK);
    assert(fvm_gc_pin(c, clip) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
    FVM_Value v; assert(fvm_object_get(c, clip, "seen", &v) == FVM_OK && v.type == FVM_VALUE_UNDEFINED);
    assert(fvm_movieclip_advance(c, clip, 1) == FVM_OK); number(c, clip, "seen", 2); number(c, clip, "_currentframe", 1);
    assert(fvm_movieclip_advance(c, clip, 1) == FVM_OK); number(c, clip, "seen", 3); number(c, clip, "_currentframe", 2);
    assert(fvm_movieclip_advance(c, clip, 1) == FVM_OK); number(c, clip, "seen", 2);
    fvm_value_string(&v, "second"); FVM_Value out;
    assert(fvm_object_call(c, clip, "gotoAndStop", 1, &v, &out) == FVM_OK);
    assert(fvm_movieclip_advance(c, clip, 2) == FVM_OK); number(c, clip, "seen", 3);
    FVM_Channel *ch = 0; assert(fvm_channel_create(&ch) == FVM_OK);
    fvm_value_undefined(&v); v.type = FVM_VALUE_OBJECT; v.object = clip;
    assert(fvm_channel_send_value(c, ch, &v, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, ch, &out, 0) == FVM_OK);
    FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK);
    assert(fvm_gc_collect(other, 0) == FVM_OK);
    assert(fvm_object_call(other, copy, "play", 0, 0, &out) == FVM_OK);
    assert(fvm_movieclip_advance(other, copy, 1) == FVM_OK); number(other, copy, "seen", 2);
    unsigned char *snapshot = 0; size_t snapshot_size = 0;
    assert(fvm_value_serialize(c, &v, &snapshot, &snapshot_size) == FVM_OK);
    assert(fvm_value_deserialize(other, snapshot, snapshot_size, &out) == FVM_OK);
    fvm_serialized_free(snapshot); FVM_Object *restored = out.object;
    assert(fvm_object_call(other, restored, "play", 0, 0, &out) == FVM_OK);
    assert(fvm_movieclip_advance(other, restored, 1) == FVM_OK); number(other, restored, "seen", 2);
    FVM_Channel *file = 0;
    FVM_Result file_result = fvm_channel_open_file("flashvm-timeline-test.bin", "w", &file);
    if (file_result == FVM_OK) {
        assert(fvm_channel_send_value(c, file, &v, 0) == FVM_OK); fvm_channel_destroy(file);
        assert(fvm_channel_open_file("flashvm-timeline-test.bin", "r", &file) == FVM_OK);
        assert(fvm_channel_receive_value(other, file, &out, 0) == FVM_OK);
        fvm_channel_destroy(file); assert(std::remove("flashvm-timeline-test.bin") == 0);
        restored = out.object;
        assert(fvm_object_call(other, restored, "play", 0, 0, &out) == FVM_OK);
        assert(fvm_movieclip_advance(other, restored, 1) == FVM_OK); number(other, restored, "seen", 2);
    } else assert(file_result == FVM_UNSUPPORTED);
    Bytes cws; for (size_t i = 0; i < 8; ++i) cws.push_back(swf[i]); cws[0] = 'C';
    uLongf bound = compressBound((uLong)swf.size() - 8); size_t header = cws.size(); cws.resize(header + bound);
    assert(compress2(cws.begin() + header, &bound, swf.begin() + 8, (uLong)swf.size() - 8, 9) == Z_OK);
    cws.resize(header + bound); FVM_Object *compressed = 0;
    assert(fvm_movieclip_load_swf(c, cws.begin(), cws.size(), "compressed", &compressed) == FVM_OK);
    assert(fvm_movieclip_advance(c, compressed, 1) == FVM_OK); number(c, compressed, "seen", 2);
    FVM_Object *bad = clip;
    assert(fvm_movieclip_load_swf(c, swf.begin(), swf.size() - 1, "bad", &bad) == FVM_INVALID_ARGUMENT && !bad);
    Bytes wrong = movie(tags, 3);
    assert(fvm_movieclip_load_swf(c, wrong.begin(), wrong.size(), "bad", &bad) == FVM_INVALID_ARGUMENT && !bad);
    Bytes scene; tag(scene, 83, empty); tag(scene, 1, empty); tag(scene, 0, empty); wrong = movie(scene, 1);
    assert(fvm_movieclip_load_swf(c, wrong.begin(), wrong.size(), "bad", &bad) == FVM_UNSUPPORTED && !bad);
    Bytes malformed, missing_label; missing_label.push_back('x');
    tag(malformed, 43, missing_label); tag(malformed, 1, empty); tag(malformed, 0, empty); wrong = movie(malformed, 1);
    assert(fvm_movieclip_load_swf(c, wrong.begin(), wrong.size(), "bad", &bad) == FVM_INVALID_ARGUMENT && !bad);
    Bytes duplicate; tag(duplicate, 43, label); tag(duplicate, 43, label); tag(duplicate, 1, empty); tag(duplicate, 0, empty); wrong = movie(duplicate, 1);
    assert(fvm_movieclip_load_swf(c, wrong.begin(), wrong.size(), "bad", &bad) == FVM_INVALID_ARGUMENT && !bad);
    Bytes truncated_action, invalid; truncated_action.push_back(0x96); truncated_action.push_back(1);
    tag(invalid, 12, truncated_action); tag(invalid, 1, empty); tag(invalid, 0, empty); wrong = movie(invalid, 1);
    assert(fvm_movieclip_load_swf(c, wrong.begin(), wrong.size(), "bad", &bad) == FVM_INVALID_ARGUMENT && !bad);
    Bytes trailing; tag(trailing, 1, empty); tag(trailing, 12, action(5)); tag(trailing, 0, empty); wrong = movie(trailing, 1);
    assert(fvm_movieclip_load_swf(c, wrong.begin(), wrong.size(), "bad", &bad) == FVM_INVALID_ARGUMENT && !bad);
    Bytes isolated, push; push.push_back(0x96); u16(push, 5); push.push_back(7); u32(push, 42); push.push_back(0);
    tag(isolated, 12, push); Bytes pop; pop.push_back(0x17); pop.push_back(0);
    tag(isolated, 12, pop); tag(isolated, 1, empty); tag(isolated, 0, empty); wrong = movie(isolated, 1);
    assert(fvm_movieclip_load_swf(c, wrong.begin(), wrong.size(), "isolated", &bad) == FVM_OK);
    assert(fvm_movieclip_advance(c, bad, 1) == FVM_ERROR);
    fvm_channel_destroy(ch); fvm_context_destroy(other); fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
