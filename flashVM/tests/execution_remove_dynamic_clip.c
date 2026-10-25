/* Dynamic policy excludes negative depths, parentless roots and detached ancestry. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"


static FVM_Object *child(FVM_Context *c, FVM_Object *parent, const char *name, double depth)
{
    FVM_Value args[2], out;
    if (!(fvm_value_string(args, name) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, depth);
    if (!(fvm_object_call(c, parent, "createEmptyMovieClip", 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out.object;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned profile;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (profile = 0; profile < 6; ++profile)
    {
        FVM_Context *c;
        FVM_Object *root, *clip, *nested;
        FVM_Value out, v;
        unsigned i;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        const double depths[] = {-16384, -1, 0, 1048575};
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
              fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_remove_dynamic_clip(c, root) == FVM_OK && fvm_exec_clip_detached(c, root, &out) == FVM_OK &&
              !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < 4; ++i)
        {
            clip = child(c, root, "child", depths[i]);
            if (!(fvm_gc_pin(c, clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, 99);
            if (!(fvm_object_set(c, clip, "removeMovieClip", &v) == FVM_OK &&
                  fvm_object_set(c, clip, "getDepth", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exec_remove_dynamic_clip(c, clip) == FVM_OK && fvm_exec_clip_detached(c, clip, &out) == FVM_OK &&
                  out.boolean == (depths[i] >= 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exec_remove_dynamic_clip(c, clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        clip = child(c, root, "branch", 2);
        nested = child(c, clip, "nested", 0);
        if (!(fvm_gc_pin(c, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_detach_clip(c, clip) == FVM_OK && fvm_exec_remove_dynamic_clip(c, nested) == FVM_OK &&
              fvm_exec_clip_detached(c, nested, &out) == FVM_OK && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
