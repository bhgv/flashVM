/* Imported-content snapshots remain atomic, counted and independent of ambient SWF/host buffers. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/serialization.h"
#include "../src/core/runtime.hpp"
#include "../src/standard/movieclip_content.hpp"
#include "zlib.h"
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
    if (!(fvm_movieclip_content_info_init(&info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_content_info(c, clip, &info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return info;
}
#ifndef FVM_CONTENT_INFO_FIXTURE_ONLY


int main()
{
    if (!(fvm_movieclip_content_info_init(0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *root = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_MovieClipContentInfo info = sentinel(), before = info;
    if (!(fvm_movieclip_get_content_info(c, root, &info) == FVM_NOT_FOUND &&
          !std::memcmp(&info, &before, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    --info.struct_size;
    before = info;
    if (!(fvm_movieclip_get_content_info(c, root, &info) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&info, &before, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info = sentinel();
    before = info;
    if (!(fvm_movieclip_get_content_info(other, root, &info) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&info, &before, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char input[sizeof(movie)];
    std::memcpy(input, movie, sizeof(movie));
    FVM_Object *clip = 0;
    if (!(fvm_movieclip_load_swf(c, input, sizeof(input), "loaded", &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::memset(input, 0, sizeof(input)); /* Import retains scalar provenance, not caller bytes. */
    info = content(c, clip);
    if (!(info.format == FVM_MOVIECLIP_CONTENT_SWF && info.swf_version == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(info.encoded_bytes == sizeof(movie) && info.decoded_bytes == sizeof(movie) && !info.pixel_width &&
          !info.pixel_height))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = info;
    if (!(fvm_movieclip_replace_swf(c, clip, "FWSbad", 6, "bad", 3) != FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info = content(c, clip);
    if (!(!std::memcmp(&info, &before, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* CWS transport count differs from the canonical uncompressed movie length. */
    unsigned char compressed[128];
    uLongf length = sizeof(compressed) - 8;
    std::memcpy(compressed, movie, 8);
    compressed[0] = 'C';
    if (!(compress2(compressed + 8, &length, movie + 8, sizeof(movie) - 8, Z_BEST_COMPRESSION) == Z_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t wire_size = (size_t)length + 8;
    if (!(wire_size != sizeof(movie)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_replace_swf(c, clip, compressed, wire_size, "compressed.swf", 14) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info = content(c, clip);
    if (!(info.encoded_bytes == wire_size && info.decoded_bytes == sizeof(movie)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Ordinary script properties cannot spoof private snapshot fields. */
    FVM_Value value;
    fvm_value_number(&value, 999);
    if (!(fvm_object_set(c, clip, "content_info", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(content(c, clip).encoded_bytes == wire_size))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value cv;
    fvm_value_undefined(&cv);
    cv.type = FVM_VALUE_OBJECT;
    cv.object = clip;
    unsigned char *serialized = 0;
    size_t serialized_size = 0;
    if (!(fvm_value_serialize(c, &cv, &serialized, &serialized_size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_deserialize(other, serialized, serialized_size, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(serialized);
    FVM_Object *copy = value.object;
    if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(content(other, copy).encoded_bytes == wire_size && content(other, copy).swf_version == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_unload(other, copy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info = content(other, copy);
    if (!(info.format == FVM_MOVIECLIP_CONTENT_EMPTY && !info.encoded_bytes && !info.decoded_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(content(c, clip).format == FVM_MOVIECLIP_CONTENT_SWF))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Imported private graphs are validated, including NaN and foreign record edges. */
    FVM_Object *data = clip->resource_result.object;
    FVM_Object *record = data->properties[FVM_MOVIECLIP_CONTENT_RECORD_NAME].object;
    FVM_Value saved = record->properties["encoded"];
    fvm_value_number(&record->properties["encoded"], std::numeric_limits<double>::quiet_NaN());
    info = sentinel();
    before = info;
    if (!(fvm_movieclip_get_content_info(c, clip, &info) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&info, &before, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    record->properties["encoded"] = saved;
    FVM_Value saved_edge = data->properties[FVM_MOVIECLIP_CONTENT_RECORD_NAME];
    FVM_Object *foreign = 0;
    if (!(fvm_object_create(other, &foreign) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value foreign_edge;
    fvm_value_undefined(&foreign_edge);
    foreign_edge.type = FVM_VALUE_OBJECT;
    foreign_edge.object = foreign;
    data->properties[FVM_MOVIECLIP_CONTENT_RECORD_NAME] = foreign_edge;
    if (!(fvm_movieclip_get_content_info(c, clip, &info) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&info, &before, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    data->properties[FVM_MOVIECLIP_CONTENT_RECORD_NAME] = saved_edge;
    c->gc_collecting = true;
    if (!(fvm_movieclip_get_content_info(c, clip, &info) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&info, &before, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->gc_collecting = false;
    if (!(fvm_gc_collect(c, 0) == FVM_OK && content(c, clip).encoded_bytes == wire_size))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_unload(c, clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info = content(c, clip);
    if (!(info.format == FVM_MOVIECLIP_CONTENT_EMPTY && !info.swf_version))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
