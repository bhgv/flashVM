/* C89 callers resolve removal targets without an interpreter activation. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c, *other;
    FVM_Object *root, *foreign, *clip;
    FVM_Value args[2], out, input;
    unsigned direct;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK &&
          fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
          fvm_gc_pin(c, root) == FVM_OK && fvm_object_create(other, &foreign) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&input);
    if (!(fvm_exec_remove_target(0, root, &input) == FVM_INVALID_ARGUMENT &&
          fvm_exec_remove_target(c, 0, &input) == FVM_INVALID_ARGUMENT &&
          fvm_exec_remove_target(c, root, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    input.type = FVM_VALUE_OBJECT;
    input.object = foreign;
    if (!(fvm_exec_remove_target(c, root, &input) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (direct = 0; direct < 2; ++direct)
    {
        if (!(fvm_value_string(args, "victim") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args + 1, 0);
        if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clip = out.object;
        if (!(fvm_gc_pin(c, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (direct)
        {
            fvm_value_undefined(&input);
            input.type = FVM_VALUE_OBJECT;
            input.object = clip;
        }
        else
        {
            if (!(fvm_value_string(&input, "victim") == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_exec_remove_target(c, root, &input) == FVM_OK && fvm_exec_clip_detached(c, clip, &out) == FVM_OK &&
              out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_value_string(&input, "absent") == FVM_OK && fvm_exec_remove_target(c, root, &input) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
