/* Loaded root-relative bounds/shape hits restore native/root scopes at every allocation failure. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "../src/core/runtime.hpp"


static void coordinate(FVM_Context *c, FVM_Object *clip, const char *method, double x, double y)
{
    FVM_Value a[2], out;
    fvm_value_number(a, x);
    fvm_value_number(a + 1, y);
    if (!(fvm_object_call(c, clip, method, 2, a, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
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
            FVM_Context *c = 0;
            FVM_Object *root = 0;
            FVM_Value a[2], out;
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
                  fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
                  fvm_gc_pin(c, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(a, "target");
            fvm_value_number(a + 1, 0);
            if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, a, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *clip = out.object;
            bool success = false;
            unsigned failures = 0;
            for (long boundary = 0; boundary < 2000 && !success; ++boundary)
            {
                fvm_value_string(a, "target");
                FVM_GCRootFrame *roots = c->gc_frames;
                allowance = boundary;
                FVM_Result r =
                    native ? fvm_object_call(c, root, "hitTest", 1, a, &out) : fvm_exec_resolve_clip(c, root, a, 0, a);
                allowance = -1;
                if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (r == FVM_OK)
                {
                    if (native)
                    {
                        if (!(out.type == FVM_VALUE_BOOLEAN && !out.boolean))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(a->type == FVM_VALUE_OBJECT && a->object == clip))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    success = true;
                }
                else
                {
                    if (!(r == FVM_OUT_OF_MEMORY && a->type == FVM_VALUE_STRING))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    ++failures;
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
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
