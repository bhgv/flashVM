/* Point conversion failures permit only the documented ordered prefix of coordinate writes. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "../src/core/runtime.hpp"


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
            const char *xkey = profile && profile < 4 ? "X" : "x", *ykey = profile && profile < 4 ? "Y" : "y";
            FVM_Context *c = 0;
            FVM_Object *clip = 0, *point = 0;
            FVM_Value v, out;
            FVM_Affine matrix = {2, 0, 0, 3, 10, 20};
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (profile)
            {
                unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
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
            bool success = false;
            unsigned failures = 0;
            FVM_GCRootFrame *roots = c->gc_frames;
            for (long failure = 0; failure < 2048 && !success; ++failure)
            {
                fvm_value_number(&v, 1);
                if (!(fvm_object_set(c, point, xkey, &v) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_number(&v, 2);
                if (!(fvm_object_set(c, point, ykey, &v) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_undefined(&v);
                v.type = FVM_VALUE_OBJECT;
                v.object = point;
                allowance = failure;
                FVM_Result r = native ? fvm_object_call(c, clip, "localToGlobal", 1, &v, &out)
                                      : fvm_exec_clip_point(c, clip, 0, &v);
                allowance = -1;
                if (!(r == FVM_OK || r == FVM_OUT_OF_MEMORY))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                success = r == FVM_OK;
                if (!success)
                {
                    ++failures;
                }
                FVM_Value x, y;
                if (!(fvm_object_get(c, point, xkey, &x) == FVM_OK && fvm_object_get(c, point, ykey, &y) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!((x.number == 1 && y.number == 2) || (x.number == 12 && (y.number == 2 || y.number == 26))))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (success)
                {
                    if (!(x.number == 12 && y.number == 26))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth &&
                      fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(success && failures))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
