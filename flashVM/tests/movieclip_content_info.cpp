/* Imported-content snapshots remain atomic, counted and independent of ambient SWF/host buffers. */
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/serialization.h"
#include "../src/core/runtime.hpp"
#include "../src/standard/movieclip_content.hpp"
#include "zlib.h"
#include <cassert>
#include <cstring>
#include <limits>
static const unsigned char movie[] = {70, 87, 83, 6, 25, 0,  0, 0, 128, 0, 0, 20, 0,
                                      0,  0,  20, 0, 0,  24, 1, 0, 64,  0, 0, 0};
/* Prepare output sentinels so failure checks include the full ABI descriptor. */
static FVM_MovieClipContentInfo sentinel()
{
    FVM_MovieClipContentInfo info;
    std::memset(&info, 0xa5, sizeof(info));
    info.struct_size = sizeof(info);
    return info;
}
/* Read a complete record using only public C interfaces. */
static FVM_MovieClipContentInfo content(FVM_Context *c, FVM_Object *clip)
{
    FVM_MovieClipContentInfo info;
    assert(fvm_movieclip_content_info_init(&info) == FVM_OK);
    assert(fvm_movieclip_get_content_info(c, clip, &info) == FVM_OK);
    return info;
}
#ifndef FVM_CONTENT_INFO_FIXTURE_ONLY
int main()
{
    assert(fvm_movieclip_content_info_init(0) == FVM_INVALID_ARGUMENT);
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *root = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK);
    assert(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK);
    FVM_MovieClipContentInfo info = sentinel(), before = info;
    assert(fvm_movieclip_get_content_info(c, root, &info) == FVM_NOT_FOUND &&
           !std::memcmp(&info, &before, sizeof(info)));
    --info.struct_size;
    before = info;
    assert(fvm_movieclip_get_content_info(c, root, &info) == FVM_INVALID_ARGUMENT &&
           !std::memcmp(&info, &before, sizeof(info)));
    info = sentinel();
    before = info;
    assert(fvm_movieclip_get_content_info(other, root, &info) == FVM_INVALID_ARGUMENT &&
           !std::memcmp(&info, &before, sizeof(info)));
    unsigned char input[sizeof(movie)];
    std::memcpy(input, movie, sizeof(movie));
    FVM_Object *clip = 0;
    assert(fvm_movieclip_load_swf(c, input, sizeof(input), "loaded", &clip) == FVM_OK &&
           fvm_gc_pin(c, clip) == FVM_OK);
    std::memset(input, 0, sizeof(input)); /* Import retains scalar provenance, not caller bytes. */
    info = content(c, clip);
    assert(info.format == FVM_MOVIECLIP_CONTENT_SWF && info.swf_version == 6);
    assert(info.encoded_bytes == sizeof(movie) && info.decoded_bytes == sizeof(movie) &&
           !info.pixel_width && !info.pixel_height);
    before = info;
    assert(fvm_movieclip_replace_swf(c, clip, "FWSbad", 6, "bad", 3) != FVM_OK);
    info = content(c, clip);
    assert(!std::memcmp(&info, &before, sizeof(info)));
    /* CWS transport count differs from the canonical uncompressed movie length. */
    unsigned char compressed[128];
    uLongf length = sizeof(compressed) - 8;
    std::memcpy(compressed, movie, 8);
    compressed[0] = 'C';
    assert(compress2(compressed + 8, &length, movie + 8, sizeof(movie) - 8, Z_BEST_COMPRESSION) ==
           Z_OK);
    size_t wire_size = (size_t)length + 8;
    assert(wire_size != sizeof(movie));
    assert(fvm_movieclip_replace_swf(c, clip, compressed, wire_size, "compressed.swf", 14) ==
           FVM_OK);
    info = content(c, clip);
    assert(info.encoded_bytes == wire_size && info.decoded_bytes == sizeof(movie));
    /* Ordinary script properties cannot spoof private snapshot fields. */
    FVM_Value value;
    fvm_value_number(&value, 999);
    assert(fvm_object_set(c, clip, "content_info", &value) == FVM_OK);
    assert(content(c, clip).encoded_bytes == wire_size);
    FVM_Value cv;
    fvm_value_undefined(&cv);
    cv.type = FVM_VALUE_OBJECT;
    cv.object = clip;
    unsigned char *serialized = 0;
    size_t serialized_size = 0;
    assert(fvm_value_serialize(c, &cv, &serialized, &serialized_size) == FVM_OK);
    assert(fvm_value_deserialize(other, serialized, serialized_size, &value) == FVM_OK);
    fvm_serialized_free(serialized);
    FVM_Object *copy = value.object;
    assert(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
    assert(content(other, copy).encoded_bytes == wire_size &&
           content(other, copy).swf_version == 6);
    assert(fvm_movieclip_unload(other, copy) == FVM_OK);
    info = content(other, copy);
    assert(info.format == FVM_MOVIECLIP_CONTENT_EMPTY && !info.encoded_bytes &&
           !info.decoded_bytes);
    assert(content(c, clip).format == FVM_MOVIECLIP_CONTENT_SWF);
    /* Imported private graphs are validated, including NaN and foreign record edges. */
    FVM_Object *data = clip->resource_result.object;
    FVM_Object *record = data->properties[FVM_MOVIECLIP_CONTENT_RECORD_NAME].object;
    FVM_Value saved = record->properties["encoded"];
    fvm_value_number(&record->properties["encoded"], std::numeric_limits<double>::quiet_NaN());
    info = sentinel();
    before = info;
    assert(fvm_movieclip_get_content_info(c, clip, &info) == FVM_INVALID_ARGUMENT &&
           !std::memcmp(&info, &before, sizeof(info)));
    record->properties["encoded"] = saved;
    c->gc_collecting = true;
    assert(fvm_movieclip_get_content_info(c, clip, &info) == FVM_INVALID_ARGUMENT &&
           !std::memcmp(&info, &before, sizeof(info)));
    c->gc_collecting = false;
    assert(fvm_gc_collect(c, 0) == FVM_OK && content(c, clip).encoded_bytes == wire_size);
    assert(fvm_movieclip_unload(c, clip) == FVM_OK);
    info = content(c, clip);
    assert(info.format == FVM_MOVIECLIP_CONTENT_EMPTY && !info.swf_version);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
