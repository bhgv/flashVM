/* Standalone images share atomic clip publication and preserve identity/source under GC. */
#define FVM_PNG_GIF_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "image_codecs_swf_png_gif.cpp"
#include "../src/core/runtime.hpp"
#include "../src/standard/movieclip_storage.hpp"
#include "flashvm/execution.h"

static const unsigned char empty_movie[] = {70, 87, 83, 8, 25, 0,  0, 0, 128, 0, 0, 20, 0,
                                            0,  0,  20, 0, 0,  24, 1, 0, 64,  0, 0, 0};


static FVM_Object *make_child(FVM_Context *c, FVM_Object *parent, const char *name)
{
    FVM_Value a[2], out;
    fvm_value_string(a, name);
    fvm_value_number(a + 1, 1);
    if (!(fvm_object_call(c, parent, "createEmptyMovieClip", 2, a, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out.object;
}
#ifndef FVM_IMAGE_CLIP_FIXTURE_ONLY


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *root = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &c) == FVM_OK && fvm_context_create(runtime, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, root) == FVM_OK && fvm_movieclip_set_source(c, root, "/old/root.swf", 13) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *target = make_child(c, root, "target"), *old = make_child(c, target, "old");
    if (!(fvm_gc_pin(c, old) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    fvm_value_number(&value, 42);
    if (!(fvm_object_set(c, target, "kept", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes images[3] = {png_fixture(), gif_fixture(true), jpeg_fixture()};
    for (unsigned kind = 0; kind < 3; ++kind)
    {
        if (!(fvm_movieclip_replace_resource(
                  c, target, images[kind].begin(), images[kind].size(), "/images/photo.bin", 17) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, root, "target", &value) == FVM_OK && value.object == target))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, target, "kept", &value) == FVM_OK && value.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char pixels[8 * 8 * 4];
        std::memset(pixels, 0, sizeof(pixels));
        FVM_RasterSurface surface = {pixels, 8, 8, 32};
        if (!(fvm_movieclip_render(c, target, &surface) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (kind == 0)
        {
            if (!(pixels[3] == 0 && pixels[4] == 64 && pixels[5] == 32 && pixels[6] == 16 && pixels[7] == 128))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (kind == 1)
        {
            if (!(pixels[3] == 0 && pixels[4] == 255 && pixels[7] == 255))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (kind == 2)
        {
            if (!(pixels[0] >= 60 && pixels[0] <= 68 && pixels[3] == 255))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_Object *before = target->resource_result.object;
        if (!(fvm_movieclip_replace_image(c, target, images[kind].begin(), images[kind].size() / 2, "bad", 3) !=
              FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(target->resource_result.object == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_replace_image(other, target, images[kind].begin(), images[kind].size(), "bad", 3) ==
              FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    int attached = 1;
    if (!(fvm_movieclip_attached_to(c, old, root, &attached) == FVM_OK && !attached))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_source(c, old, &value) == FVM_OK && fvm_value_string_size(&value) == 13))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_source(c, target, &value) == FVM_OK && fvm_value_string_size(&value) == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes animated = gif_fixture(false, true);
    FVM_Object *before = target->resource_result.object;
    if (!(fvm_movieclip_replace_image(c, target, animated.begin(), animated.size(), "bad", 3) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(target->resource_result.object == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_replace_resource(c, target, "FWSx", 4, "bad", 3) != FVM_OK &&
          target->resource_result.object == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_replace_resource(c, target, empty_movie, sizeof(empty_movie), "/restored.swf", 13) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_unload(c, target) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Loaded SWF7 permits JPEG but rejects PNG/GIF before mutation. */
    unsigned char version7[sizeof(empty_movie)];
    std::memcpy(version7, empty_movie, sizeof(empty_movie));
    version7[3] = 7;
    if (!(fvm_load_memory(c, version7, sizeof(version7)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *legacy = 0;
    if (!(fvm_movieclip_load_swf(c, version7, sizeof(version7), "legacy", &legacy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, legacy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = legacy->resource_result.object;
    if (!(fvm_movieclip_replace_image(c, legacy, images[0].begin(), images[0].size(), "png", 3) == FVM_UNSUPPORTED &&
          legacy->resource_result.object == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_replace_image(c, legacy, images[2].begin(), images[2].size(), "jpeg", 4) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes progressive = jpeg_fixture(false, true);
    before = legacy->resource_result.object;
    if (!(fvm_movieclip_replace_image(c, legacy, progressive.begin(), progressive.size(), "jpeg", 4) ==
              FVM_UNSUPPORTED &&
          legacy->resource_result.object == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_replace_image(c, target, progressive.begin(), progressive.size(), "jpeg", 4) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}
#endif
