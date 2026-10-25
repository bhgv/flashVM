/* Loaded point final application rounds binary32 linear terms to even twips, retaining host doubles. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cmath>


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned profile = 0; profile < 6; ++profile)
    {
        for (unsigned native = 0; native < 2; ++native)
        {
            for (unsigned reverse = 0; reverse < 2; ++reverse)
            {
                FVM_Context *c = 0;
                FVM_Object *clip = 0, *point = 0;
                FVM_Value v, out;
                FVM_Affine matrix = {0.5, 0, 0, 0.5, 1, 2};
                unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                if (reverse)
                {
                    matrix.xx = matrix.yy = 2;
                    matrix.x0 = 2;
                    matrix.y0 = 4;
                }
                if (!(fvm_context_create(rt, &c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (profile)
                {
                    swf[3] = (unsigned char)(profile + 3);
                    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "clip", 1, &clip) == FVM_OK &&
                      fvm_gc_pin(c, clip) == FVM_OK && fvm_movieclip_set_matrix(c, clip, &matrix) == FVM_OK &&
                      fvm_object_create(c, &point) == FVM_OK && fvm_gc_pin(c, point) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const double input[] = {0.05, 0.15, -0.15}, rounded[] = {0, 0.1, -0.1};
                FVM_GCRootFrame *roots = c->gc_frames;
                for (unsigned i = 0; i < 3; ++i)
                {
                    fvm_value_number(&v, input[i]);
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
                    double delta = profile ? rounded[i] : input[i] * 0.5;
                    if (!(fvm_object_get(c, point, "x", &out) == FVM_OK &&
                          std::fabs(out.number - ((reverse ? -1 : 1) + delta)) < 1e-9 &&
                          fvm_object_get(c, point, "y", &out) == FVM_OK &&
                          std::fabs(out.number - ((reverse ? -2 : 2) + delta)) < 1e-9))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth))
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
    fvm_runtime_destroy(rt);
    return 0;
}
