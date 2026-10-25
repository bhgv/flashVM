/* C89 explicit duplication preserves aliases/errors and bypasses overridden methods. */
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
    unsigned profile;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (profile = 0; profile < 6; ++profile)
    {
        FVM_Context *c, *other;
        FVM_Object *root, *clip, *initial, *foreign;
        FVM_Value a[3], out, v;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
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
              fvm_gc_pin(c, root) == FVM_OK && fvm_object_create(c, &initial) == FVM_OK &&
              fvm_gc_pin(c, initial) == FVM_OK && fvm_object_create(other, &foreign) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(a, "source");
        fvm_value_number(a + 1, 1);
        if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, a, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clip = out.object;
        fvm_value_boolean(&v, 0);
        if (!(fvm_object_set(c, clip, "duplicateMovieClip", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 73);
        if (!(fvm_object_set(c, initial, "seed", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(a, "copy");
        fvm_value_number(a + 1, 2);
        fvm_value_undefined(a + 2);
        a[2].type = FVM_VALUE_OBJECT;
        a[2].object = initial;
        if (!(fvm_exec_duplicate_clip(c, clip, a, a + 1, a + 2, a) == FVM_OK && a->type == FVM_VALUE_OBJECT &&
              a->object != clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, a->object, "seed", &out) == FVM_OK && out.number == 73))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(a, "rootcopy");
        if (!(fvm_exec_duplicate_clip(c, root, a, a + 1, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&out, 73);
        if (!(fvm_exec_duplicate_clip(c, foreign, a, a + 1, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 73))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_duplicate_clip(c, clip, 0, a + 1, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 73))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(a + 1, -20000);
        if (!(fvm_exec_duplicate_clip(c, clip, a, a + 1, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 73))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
