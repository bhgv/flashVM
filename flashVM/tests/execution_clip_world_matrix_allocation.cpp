/* Standalone world queries preserve complete output and execution state at every allocation failure. */
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
    FVM_Context *c = 0;
    FVM_Object *root = 0;
    FVM_Value a[2], value;
    FVM_Affine half = {0.5, 0, 0, 0.5, 0, 0}, local = {1, 0, 0, 1, 0.15, -0.15};
    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
          fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK &&
          fvm_movieclip_set_matrix(c, root, &half) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(a, "mid");
    fvm_value_number(a + 1, 1);
    if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, a, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *mid = value.object;
    if (!(fvm_movieclip_set_matrix(c, mid, &half) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(a, "leaf");
    if (!(fvm_object_call(c, mid, "createEmptyMovieClip", 2, a, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *leaf = value.object;
    if (!(fvm_movieclip_set_matrix(c, leaf, &local) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_GCRootFrame *roots = c->gc_frames;
    bool success = false;
    unsigned failures = 0;
    for (long failure = 0; failure < 2048 && !success; ++failure)
    {
        FVM_TwipMatrix out = {7, 8, 9, 10, 77, 88};
        allowance = failure;
        FVM_Result r = fvm_exec_clip_world_matrix(c, leaf, &out);
        allowance = -1;
        if (!(r == FVM_OK || r == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        success = r == FVM_OK;
        if (success)
        {
            if (!(out.a == 0.25f && out.d == 0.25f && out.tx == 1 && out.ty == -1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            ++failures;
            if (!(out.a == 7 && out.b == 8 && out.c == 9 && out.d == 10 && out.tx == 77 && out.ty == 88))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth && fvm_gc_collect(c, 0) == FVM_OK))
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
    fvm_runtime_destroy(rt);
    return 0;
}
