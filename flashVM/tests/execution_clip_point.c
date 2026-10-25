/* Strict C89 explicit point conversion agrees with both native methods in host/SWF4-8. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <math.h>


static void number(FVM_Context *c, FVM_Object *o, const char *name, double n)
{
    FVM_Value v;
    fvm_value_number(&v, n);
    if (!(fvm_object_set(c, o, name, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void check(FVM_Context *c, FVM_Object *o, const char *name, double n)
{
    FVM_Value v;
    if (!(fvm_object_get(c, o, name, &v) == FVM_OK && fabs(v.number - n) < 1e-8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned profile, method;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (profile = 0; profile < 6; ++profile)
    {
        for (method = 0; method < 2; ++method)
        {
            FVM_Context *c;
            FVM_Object *root, *child, *point;
            FVM_Value args[2], out, v;
            FVM_Affine parent = {2, 0, 0, 3, 20, 30}, local = {0, 1, -1, 0, 5, 7}, singular = {0, 0, 0, 0, 0, 0};
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
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
            if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
                  fvm_gc_pin(c, root) == FVM_OK && fvm_movieclip_set_matrix(c, root, &parent) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(args, "child");
            fvm_value_number(args + 1, 1);
            if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            child = out.object;
            if (!(fvm_movieclip_set_matrix(c, child, &local) == FVM_OK && fvm_object_create(c, &point) == FVM_OK &&
                  fvm_gc_pin(c, point) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&v);
            v.type = FVM_VALUE_OBJECT;
            v.object = point;
            number(c, point, "x", 4);
            number(c, point, "y", 6);
            if (!((method ? fvm_object_call(c, child, "localToGlobal", 1, &v, &out)
                          : fvm_exec_clip_point(c, child, 0, &v)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            check(c, point, "x", 18);
            check(c, point, "y", 63);
            if (!((method ? fvm_object_call(c, child, "globalToLocal", 1, &v, &out)
                          : fvm_exec_clip_point(c, child, 1, &v)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            check(c, point, "x", 4);
            check(c, point, "y", 6);
            if (!(fvm_movieclip_set_matrix(c, child, &singular) == FVM_OK &&
                  fvm_exec_clip_point(c, child, 1, &v) == (profile ? FVM_OK : FVM_INVALID_ARGUMENT)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            check(c, point, "x", 4);
            check(c, point, "y", 6);
            if (!(fvm_exec_clip_point(c, child, 0, 0) == FVM_INVALID_ARGUMENT &&
                  fvm_exec_clip_point(c, point, 0, &v) == FVM_INVALID_ARGUMENT && fvm_gc_collect(c, 0) == FVM_OK))
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
