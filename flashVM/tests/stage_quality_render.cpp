/* LOW affects solid/stroke/gradient/bitmap MovieClip edges; standalone paths retain AA. */
#define FVM_MOVIECLIP_GRADIENT_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_gradient.cpp"
#include "flashvm/stage.h"
#include "flashvm/transfer.h"
#include "../src/render/quality_internal.hpp"


static void triangle(FVM_Context *c, FVM_Object *clip)
{
    const double xy[] = {0.2, 0.3, 7.6, 1.4, 2.3, 7.7, 0.2, 0.3};
    FVM_Value a[2], out;
    for (unsigned i = 0; i < 8; i += 2)
    {
        fvm_value_number(a, xy[i]);
        fvm_value_number(a + 1, xy[i + 1]);
        if (!(fvm_object_call(c, clip, i ? "lineTo" : "moveTo", 2, a, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_object_call(c, clip, "endFill", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static unsigned partial(const unsigned char *p)
{
    unsigned count = 0;
    for (unsigned i = 3; i < 256; i += 4)
    {
        if (p[i] && p[i] != 255)
        {
            ++count;
        }
    }
    return count;
}


static bool visible(const unsigned char *p)
{
    for (unsigned i = 3; i < 256; i += 4)
    {
        if (p[i])
        {
            return true;
        }
    }
    return false;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Object *root;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "quality", 1, &root) == FVM_OK &&
          fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned style = 0; style < 4; ++style)
    {
        FVM_Object *clip = new_child(c, root, "paint", style + 1);
        FVM_Value a[8], out;
        if (style == 0)
        {
            fvm_value_number(a, 0xff0000);
            if (!(fvm_object_call(c, clip, "beginFill", 1, a, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (style == 1)
        {
            arguments(c, a, false);
            if (!(fvm_object_call(c, clip, "beginGradientFill", 8, a, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (style == 2)
        {
            FVM_Object *image;
            fvm_value_number(a, 1);
            fvm_value_number(a + 1, 1);
            fvm_value_boolean(a + 2, 0);
            fvm_value_number(a + 3, 0xff0000);
            if (!(fvm_construct(c, "flash.display.BitmapData", 4, a, &image) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ref(a, image);
            if (!(fvm_object_call(c, clip, "beginBitmapFill", 1, a, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            fvm_value_number(a, 1);
            fvm_value_number(a + 1, 0xff0000);
            if (!(fvm_object_call(c, clip, "lineStyle", 2, a, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        triangle(c, clip);
        unsigned char high[256] = {0}, low[256] = {0}, again[256] = {0};
        FVM_RasterSurface surface = {high, 8, 8, 32};
        if (!(fvm_stage_set_quality(c, FVM_STAGE_QUALITY_HIGH) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Result r = fvm_movieclip_render(c, clip, &surface);
        if (r == FVM_UNSUPPORTED)
        {
            continue;
        }
        if (!(r == FVM_OK && visible(high) && partial(high) > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned char toggle[] = {8, 0};
        if (!(fvm_avm1_execute(c, toggle, sizeof(toggle)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        surface.pixels = low;
        if (!(fvm_movieclip_render(c, clip, &surface) == FVM_OK && visible(low) && !partial(low)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(std::memcmp(high, low, sizeof(high)) != 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_stage_set_quality(c, FVM_STAGE_QUALITY_BEST) == FVM_OK && fvm_stage_toggle_quality(c) == FVM_OK &&
              fvm_stage_toggle_quality(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        surface.pixels = again;
        if (!(fvm_movieclip_render(c, clip, &surface) == FVM_OK && !std::memcmp(high, again, sizeof(high))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Transport clip geometry, never context-wide stage policy. */
        FVM_Context *other;
        FVM_Channel *channel;
        FVM_Value packet, restored;
        if (!(fvm_context_create(rt, &other) == FVM_OK && fvm_channel_create(&channel) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_stage_set_quality(c, FVM_STAGE_QUALITY_LOW) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ref(&packet, clip);
        if (!(fvm_channel_send_value(c, channel, &packet, 0) == FVM_OK &&
              fvm_channel_receive_value(other, channel, &restored, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_StageQuality imported_quality;
        if (!(fvm_stage_get_quality(other, &imported_quality) == FVM_OK && imported_quality == FVM_STAGE_QUALITY_HIGH))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        std::memset(again, 0, sizeof(again));
        surface.pixels = again;
        if (!(fvm_movieclip_render(other, restored.object, &surface) == FVM_OK && partial(again) > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(channel);
        fvm_context_destroy(other);
    }
    FVM_Path *path;
    if (!(fvm_path_create(&path) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_PathCommand command;
    std::memset(&command, 0, sizeof(command));
    command.verb = FVM_PATH_MOVE;
    command.x[0] = 0.2;
    command.y[0] = 0.3;
    if (!(fvm_path_add(path, &command) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    command.verb = FVM_PATH_LINE;
    command.x[0] = 7.6;
    command.y[0] = 1.4;
    if (!(fvm_path_add(path, &command) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    command.x[0] = 2.3;
    command.y[0] = 7.7;
    if (!(fvm_path_add(path, &command) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    command.verb = FVM_PATH_CLOSE;
    if (!(fvm_path_add(path, &command) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char pixels[256] = {0};
    FVM_RasterSurface surface = {pixels, 8, 8, 32};
    FVM_RasterColor red = {255, 0, 0, 255};
    if (!(fvm_stage_set_quality(c, FVM_STAGE_QUALITY_LOW) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Result r = fvm_path_draw(path, &surface, red, 0, 0);
    if (!(r == FVM_UNSUPPORTED || (r == FVM_OK && partial(pixels) > 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
