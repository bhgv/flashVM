/* Stroke modes preserve stage AA selection, affine placement and inherited alpha after sampling. */
#include <stdio.h>
#include <stdlib.h>
#include "line_gradient_fixture.hpp"
#include "flashvm/stage.h"
#include "../src/render/quality_internal.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const double xy[] = {1.2, 2.3, 13.6, 4.4, 5.3, 12.7, 1.2, 2.3};
    for (unsigned transformed = 0; transformed < 2; ++transformed)
    {
        for (unsigned mode = 0; mode < 3; ++mode)
        {
            for (unsigned radial = 0; radial < 2; ++radial)
            {
                FVM_Object *clip = 0;
                if (!(fvm_movieclip_create(c, "quality", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                line_gradient_style(c, clip, mode, radial != 0);
                FVM_Value args[2], out;
                for (unsigned i = 0; i < 8; i += 2)
                {
                    fvm_value_number(args, xy[i]);
                    fvm_value_number(args + 1, xy[i + 1]);
                    if (!(fvm_object_call(c, clip, i ? "lineTo" : "moveTo", 2, args, &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                set(c, clip, "_alpha", 50);
                if (transformed)
                {
                    set(c, clip, "_x", 1.25);
                    set(c, clip, "_y", -0.5);
                    set(c, clip, "_xscale", 75);
                    set(c, clip, "_yscale", 125);
                }
                const double sx = transformed ? 0.75 : 1, sy = transformed ? 1.25 : 1, tx = transformed ? 1.25 : 0,
                             ty = transformed ? -0.5 : 0;
                FVM_Path *path = 0;
                FVM_PathCommand command;
                if (!(fvm_path_create(&path) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                std::memset(&command, 0, sizeof(command));
                for (unsigned i = 0; i < 8; i += 2)
                {
                    command.verb = i ? FVM_PATH_LINE : FVM_PATH_MOVE;
                    command.x[0] = sx * xy[i] + tx;
                    command.y[0] = sy * xy[i + 1] + ty;
                    if (!(fvm_path_add(path, &command) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                FVM_Gradient gradient;
                FVM_GradientStop stops[2];
                if (!(fvm_gradient_init(&gradient) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                stops[0].ratio = 0;
                stops[0].color.r = stops[0].color.a = 255;
                stops[0].color.g = stops[0].color.b = 0;
                stops[1].ratio = 1;
                stops[1].color.r = stops[1].color.g = 0;
                stops[1].color.b = stops[1].color.a = 255;
                gradient.stops = stops;
                gradient.count = 2;
                gradient.type = radial ? FVM_GRADIENT_RADIAL : FVM_GRADIENT_LINEAR;
                gradient.matrix.xx = 2 * sx;
                gradient.matrix.yy = 2 * sy;
                gradient.matrix.x0 = 4.5 * sx + tx;
                gradient.matrix.y0 = 8.5 * sy + ty;
                gradient.multiply[3] = 0.5;
                for (unsigned quality = 0; quality < 4; ++quality)
                {
                    unsigned char actual[1024] = {0}, expected[1024] = {0};
                    FVM_RasterSurface dst = {actual, 16, 16, 64}, oracle = {expected, 16, 16, 64};
                    if (!(fvm_stage_set_quality(c, (FVM_StageQuality)quality) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Result result = fvm_movieclip_render(c, clip, &dst);
                    if (result == FVM_UNSUPPORTED)
                    {
                        continue;
                    }
                    if (!(result == FVM_OK &&
                          fvm_path_stroke_gradient_quality(
                              path, &oracle, &gradient, 4 * sx, mode, quality != FVM_STAGE_QUALITY_LOW) == FVM_OK &&
                          !std::memcmp(actual, expected, sizeof(actual))))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    unsigned partial = 0, visible = 0;
                    for (unsigned i = 3; i < sizeof(actual); i += 4)
                    {
                        if (actual[i])
                        {
                            ++visible;
                        }
                        if (actual[i] && actual[i] != 128)
                        {
                            ++partial;
                        }
                    }
                    if (!(visible && (quality == FVM_STAGE_QUALITY_LOW ? !partial : partial > 0)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                fvm_path_destroy(path);
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
