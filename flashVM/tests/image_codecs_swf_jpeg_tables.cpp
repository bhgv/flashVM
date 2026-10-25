#define FVM_JPEG_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "image_codecs_swf_jpeg.cpp"


static inline void split_jpeg(Bytes &tables, Bytes &image)
{
    Bytes full = jpeg_fixture();
    tables.push_back(255);
    tables.push_back(216);
    image.push_back(255);
    image.push_back(216);
    size_t at = 2;
    bool scan = false;
    while (at < full.size())
    {
        if (!(full[at] == 255 && at + 4 <= full.size()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned marker = full[at + 1];
        if (marker == 218)
        {
            for (size_t i = at; i < full.size(); ++i)
            {
                image.push_back(full[i]);
            }
            scan = true;
            break;
        }
        size_t length = ((size_t)full[at + 2] << 8) | full[at + 3];
        if (!(length >= 2 && length + 2 <= full.size() - at))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes &dest = marker == 219 || marker == 196 ? tables : image;
        for (size_t i = at; i < at + length + 2; ++i)
        {
            dest.push_back(full[i]);
        }
        at += length + 2;
    }
    if (!(scan))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tables.push_back(255);
    tables.push_back(217);
}


static inline Bytes table_movie(unsigned version, bool duplicate = false, bool missing = false)
{
    Bytes tables, image;
    split_jpeg(tables, image);
    Bytes tags, resource, placement, empty;
    u16(resource, 7);
    append(resource, image);
    if (!missing)
    {
        tag(tags, 8, tables);
    }
    if (duplicate)
    {
        tag(tags, 8, tables);
    }
    tag(tags, 6, resource);
    u16(placement, 7);
    u16(placement, 1);
    append(placement, matrix(0, 0));
    tag(tags, 4, placement);
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    Bytes result = movie(tags, 1);
    result[3] = (unsigned char)version;
    return result;
}
#ifndef FVM_JPEG_TABLES_FIXTURE_ONLY


int main()
{
    Bytes tables, image, normalized;
    split_jpeg(tables, image);
    if (!(fvm_swf_read_jpeg_tables(tables.begin(), tables.size(), 1, normalized) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(normalized.size() == tables.size()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes resource;
    u16(resource, 7);
    append(resource, image);
    FVM_SWFBitmap decoded;
    if (!(fvm_swf_read_legacy_jpeg(resource.begin(), resource.size(), normalized, 1, 1024, decoded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(decoded.width == 8 && decoded.height == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (size_t n = 0; n < tables.size(); ++n)
    {
        FVM_Result r = fvm_swf_read_jpeg_tables(tables.begin(), n, 1, normalized);
        if (!(r == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(normalized.size() == tables.size()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (size_t n = 0; n < resource.size(); ++n)
    {
        if (!(fvm_swf_read_legacy_jpeg(resource.begin(), n, normalized, 1, 1024, decoded) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(decoded.id == 7 && decoded.pixels.size() == 256))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Bytes duplicate = tables;
    duplicate.push_back(0);
    if (!(fvm_swf_read_jpeg_tables(duplicate.begin(), duplicate.size(), 1, normalized) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes bad = tables;
    bad[3] = 218;
    if (!(fvm_swf_read_jpeg_tables(bad.begin(), bad.size(), 1, normalized) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes legacy;
    legacy.push_back(255);
    legacy.push_back(217);
    legacy.push_back(255);
    legacy.push_back(216);
    append(legacy, tables);
    if (!(fvm_swf_read_jpeg_tables(legacy.begin(), legacy.size(), 7, normalized) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_read_jpeg_tables(legacy.begin(), legacy.size(), 8, normalized) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes old_image;
    u16(old_image, 7);
    old_image.push_back(255);
    old_image.push_back(217);
    old_image.push_back(255);
    old_image.push_back(216);
    append(old_image, image);
    if (!(fvm_swf_read_legacy_jpeg(old_image.begin(), old_image.size(), normalized, 7, 1024, decoded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_read_legacy_jpeg(old_image.begin(), old_image.size(), normalized, 8, 1024, decoded) ==
          FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 1; version <= 8; ++version)
    {
        Bytes swf = table_movie(version);
        FVM_Object *root = 0;
        if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick(c, root);
        jpeg_pixels(c, root, false);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        jpeg_pixels(c, root, false);
        FVM_Value value, copy;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = root;
        FVM_Channel *ch = 0;
        if (!(fvm_channel_create(&ch) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(other, copy.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        jpeg_pixels(other, copy.object, false);
        fvm_channel_destroy(ch);
    }
    for (unsigned malformed = 0; malformed < 2; ++malformed)
    {
        Bytes swf = table_movie(8, malformed == 0, malformed == 1);
        FVM_Object *root = 0;
        if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "bad", &root) == FVM_INVALID_ARGUMENT && !root))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
