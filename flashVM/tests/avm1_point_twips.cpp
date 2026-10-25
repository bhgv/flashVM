/* Independent decimal/raw-double cases for loaded point input truncation and signed saturation. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cmath>
#include <limits>


int main()
{
    const double inputs[] = {0.049,
                             0.051,
                             -0.049,
                             -0.051,
                             40.018,
                             -40.018,
                             std::numeric_limits<double>::quiet_NaN(),
                             std::numeric_limits<double>::infinity(),
                             -std::numeric_limits<double>::infinity(),
                             std::numeric_limits<double>::max(),
                             -std::numeric_limits<double>::max(),
                             107374182.35,
                             -107374182.4};
    const double expected[] = {0,
                               0.05,
                               0,
                               -0.05,
                               40,
                               -40,
                               0,
                               -2147483648.0 / 20,
                               -2147483648.0 / 20,
                               -2147483648.0 / 20,
                               -2147483648.0 / 20,
                               -2147483648.0 / 20,
                               -2147483648.0 / 20};
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned native = 0; native < 2; ++native)
        {
            for (unsigned reverse = 0; reverse < 2; ++reverse)
            {
                FVM_Context *c = 0;
                FVM_Object *clip = 0, *point = 0;
                FVM_Value v, out;
                unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                swf[3] = (unsigned char)version;
                if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
                      fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "clip", 1, &clip) == FVM_OK &&
                      fvm_gc_pin(c, clip) == FVM_OK && fvm_object_create(c, &point) == FVM_OK &&
                      fvm_gc_pin(c, point) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (unsigned i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i)
                {
                    fvm_value_number(&v, inputs[i]);
                    if (!(fvm_object_set(c, point, "x", &v) == FVM_OK && fvm_object_set(c, point, "y", &v) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_undefined(&v);
                    v.type = FVM_VALUE_OBJECT;
                    v.object = point;
                    if (!((native ? fvm_object_call(c, clip, reverse ? "globalToLocal" : "localToGlobal", 1, &v, &out)
                                  : fvm_exec_clip_point(c, clip, reverse, &v)) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_get(c, point, "x", &out) == FVM_OK && out.number == expected[i] &&
                          fvm_object_get(c, point, "y", &out) == FVM_OK && out.number == expected[i]))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (reverse)
                {
                    FVM_Affine singular = {0, 0, 0, 0, 0, 0};
                    if (!(fvm_movieclip_set_matrix(c, clip, &singular) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_number(&v, std::numeric_limits<double>::infinity());
                    if (!(fvm_object_set(c, point, "x", &v) == FVM_OK && fvm_object_set(c, point, "y", &v) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_undefined(&v);
                    v.type = FVM_VALUE_OBJECT;
                    v.object = point;
                    if (!((native ? fvm_object_call(c, clip, "globalToLocal", 1, &v, &out)
                                  : fvm_exec_clip_point(c, clip, 1, &v)) == FVM_OK &&
                          fvm_object_get(c, point, "x", &out) == FVM_OK && out.number == 2147483647.0 / 20 &&
                          fvm_object_get(c, point, "y", &out) == FVM_OK && out.number == 2147483647.0 / 20))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    /* Unloaded C-host contexts retain precise input doubles and reject nonfinite values. */
    {
        FVM_Context *c = 0;
        FVM_Object *clip = 0, *point = 0;
        FVM_Value v, out;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
              fvm_movieclip_create(c, "clip", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK &&
              fvm_object_create(c, &point) == FVM_OK && fvm_gc_pin(c, point) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 0.051);
        if (!(fvm_object_set(c, point, "x", &v) == FVM_OK && fvm_object_set(c, point, "y", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_OBJECT;
        v.object = point;
        if (!(fvm_exec_clip_point(c, clip, 0, &v) == FVM_OK && fvm_object_get(c, point, "x", &out) == FVM_OK &&
              out.number == 0.051))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&out, std::numeric_limits<double>::infinity());
        if (!(fvm_object_set(c, point, "x", &out) == FVM_OK &&
              fvm_exec_clip_point(c, clip, 0, &v) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
