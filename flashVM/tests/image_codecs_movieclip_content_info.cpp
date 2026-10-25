/* Image content records describe original encoded/pixel payloads, not transformed drawing bounds.
 */
#define FVM_IMAGE_CLIP_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "image_codecs_movieclip_loading.cpp"
#include "flashvm/serialization.h"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *root = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *clip = make_child(c, root, "image");
    Bytes images[] = {png_fixture(), gif_fixture(true), jpeg_fixture()};
    FVM_MovieClipContentFormat formats[] = {
        FVM_MOVIECLIP_CONTENT_PNG, FVM_MOVIECLIP_CONTENT_GIF, FVM_MOVIECLIP_CONTENT_JPEG};
    for (unsigned kind = 0; kind < 3; ++kind)
    {
        if (!(fvm_movieclip_replace_image(c, clip, images[kind].begin(), images[kind].size(), "image.bin", 9) ==
              FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
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
        if (!(info.format == formats[kind] && info.encoded_bytes == images[kind].size() && !info.swf_version))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(info.pixel_width == 8 && info.pixel_height == 8 && info.decoded_bytes == 256))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value count;
        if (!(fvm_object_call(c, clip, "getBytesLoaded", 0, 0, &count) == FVM_OK &&
              count.number == images[kind].size()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, clip, "getBytesTotal", 0, 0, &count) == FVM_OK && count.number == images[kind].size()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, clip, "getSWFVersion", 0, 0, &count) == FVM_OK && count.number == -1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_MovieClipContentInfo before = info;
        FVM_Value value;
        fvm_value_number(&value, 16);
        if (!(fvm_object_set(c, clip, "_width", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_get_content_info(c, clip, &info) == FVM_OK && !std::memcmp(&info, &before, sizeof(info))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_replace_image(c, clip, images[kind].begin(), images[kind].size() / 2, "bad", 3) != FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_get_content_info(c, clip, &info) == FVM_OK && !std::memcmp(&info, &before, sizeof(info))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Duplicate content keeps immutable metadata, but unloading the duplicate cannot reset it.
         */
        FVM_Value args[2], out;
        fvm_value_string(args, "copy");
        fvm_value_number(args + 1, 2);
        if (!(fvm_object_call(c, clip, "duplicateMovieClip", 2, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *copy = out.object;
        if (!(fvm_movieclip_get_content_info(c, copy, &info) == FVM_OK && info.encoded_bytes == images[kind].size()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_unload(c, copy) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_get_content_info(c, clip, &info) == FVM_OK && info.format == formats[kind]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
