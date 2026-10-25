/* SetBackgroundColor is a replayable, graph-transported timeline control with stage-wide first win. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/movieclip.h"
#include "flashvm/stage.h"
#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"

typedef fvm_detail::vector<unsigned char> Bytes;


static void word(Bytes &b, unsigned n)
{
    b.push_back((unsigned char)n);
    b.push_back((unsigned char)(n >> 8));
}


static void tag(Bytes &b, unsigned code, const Bytes &p)
{
    if (!(p.size() < 63))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    word(b, (code << 6) | (unsigned)p.size());
    for (size_t i = 0; i < p.size(); ++i)
    {
        b.push_back(p[i]);
    }
}


static Bytes movie(const Bytes &tags)
{
    Bytes b;
    b.push_back('F');
    b.push_back('W');
    b.push_back('S');
    b.push_back(8);
    unsigned long n = (unsigned long)tags.size() + 14;
    for (unsigned i = 0; i < 4; ++i)
    {
        b.push_back((unsigned char)n);
        n >>= 8;
    }
    b.push_back(8);
    b.push_back(0);
    word(b, 12 * 256);
    word(b, 2);
    for (size_t i = 0; i < tags.size(); ++i)
    {
        b.push_back(tags[i]);
    }
    return b;
}

#include "flashvm/swf.h"
#include <cstring>


static Bytes document(unsigned long flags, const char *xml, bool duplicate = false)
{
    Bytes tags, attributes, metadata, empty;
    for (unsigned i = 0; i < 4; ++i)
    {
        attributes.push_back((unsigned char)flags);
        flags >>= 8;
    }
    tag(tags, 69, attributes);
    if (duplicate)
    {
        tag(tags, 69, attributes);
    }
    if (xml)
    {
        for (size_t i = 0; i <= std::strlen(xml); ++i)
        {
            metadata.push_back((unsigned char)xml[i]);
        }
        tag(tags, 77, metadata);
    }
    tag(tags, 1, empty);
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    return movie(tags);
}


static void inspect_metadata(FVM_Context *c, FVM_Object *clip, const char *xml)
{
    FVM_SWFDocumentInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_movieclip_swf_document_info(c, clip, &info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(info.file_attributes_present && info.file_attributes == 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(info.metadata.type == FVM_VALUE_STRING && fvm_value_string_size(&info.metadata) == std::strlen(xml)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::memcmp(info.metadata.string, xml, std::strlen(xml))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
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
    const char *xml = "<x>\xd1\x8f</x>";
    Bytes swf = document(16, xml);
    FVM_SWFInfo header;
    header.struct_size = sizeof(header);
    if (!(fvm_swf_inspect(swf.begin(), swf.size(), &header) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *clip = 0;
    if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inspect_metadata(c, clip, xml);
    FVM_Channel *channel = 0;
    if (!(fvm_channel_create(&channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = clip;
    if (!(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, channel, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(other, value.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(other, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inspect_metadata(other, value.object, xml);
    Bytes zero = document(0, 0);
    if (!(fvm_load_memory(c, zero.begin(), zero.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inspect_metadata(c, clip, xml);
    const unsigned long unsupported[] = {1, 8, 32, 64};
    for (unsigned i = 0; i < 4; ++i)
    {
        Bytes bad = document(unsupported[i], 0);
        FVM_Object *out = clip;
        if (!(fvm_swf_inspect(bad.begin(), bad.size(), &header) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_load_swf(c, bad.begin(), bad.size(), "bad", &out) == FVM_UNSUPPORTED && !out))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Bytes malformed[] = {
        document(16, 0), document(0, "<x/>"), document(0, 0, true), document(128, 0), document(16, "\xc0\x80")};
    for (unsigned i = 0; i < 5; ++i)
    {
        if (!(fvm_swf_inspect(malformed[i].begin(), malformed[i].size(), &header) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *out = clip;
        if (!(fvm_movieclip_load_swf(c, malformed[i].begin(), malformed[i].size(), "bad", &out) ==
                  FVM_INVALID_ARGUMENT &&
              !out))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Bytes duplicate_tags, attributes, metadata, empty;
    attributes.push_back(16);
    attributes.push_back(0);
    attributes.push_back(0);
    attributes.push_back(0);
    metadata.push_back('x');
    metadata.push_back(0);
    tag(duplicate_tags, 69, attributes);
    tag(duplicate_tags, 77, metadata);
    tag(duplicate_tags, 77, metadata);
    tag(duplicate_tags, 1, empty);
    tag(duplicate_tags, 1, empty);
    tag(duplicate_tags, 0, empty);
    Bytes duplicate_metadata = movie(duplicate_tags);
    if (!(fvm_swf_inspect(duplicate_metadata.begin(), duplicate_metadata.size(), &header) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes late_tags;
    tag(late_tags, 9, metadata);
    tag(late_tags, 69, attributes);
    tag(late_tags, 77, metadata);
    tag(late_tags, 1, empty);
    tag(late_tags, 1, empty);
    tag(late_tags, 0, empty);
    Bytes late = movie(late_tags);
    if (!(fvm_swf_inspect(late.begin(), late.size(), &header) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes empty_metadata = document(16, "");
    FVM_Object *empty_clip = 0;
    if (!(fvm_movieclip_load_swf(c, empty_metadata.begin(), empty_metadata.size(), "empty", &empty_clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inspect_metadata(c, empty_clip, "");
    fvm_channel_destroy(channel);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
