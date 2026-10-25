/* Native removal uses complete counted names and retains unrelated aliases. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"


static FVM_Object *child(FVM_Context *c, FVM_Object *root, const char *name, unsigned depth)
{
    FVM_Value args[2], out;
    if (!(fvm_value_string(args, name) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, depth);
    if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
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
    unsigned mode;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (mode = 0; mode < 6; ++mode)
    {
        FVM_Context *c;
        FVM_Object *root, *clip;
        FVM_Value value, out, arg;
        int own, deleted;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (mode)
        {
            swf[3] = (unsigned char)(mode + 3);
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clip = child(c, root, "original", 2);
        if (!(fvm_gc_pin(c, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = clip;
        if (!(fvm_object_set(c, root, "prefix", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_bytes(c, root, "prefix\0tail", 11, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string_bytes(&value, "prefix\0tail", 11) == FVM_OK &&
              fvm_object_set(c, clip, "_name", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, clip, "removeMovieClip", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_has_own_bytes(c, root, "prefix\0tail", 11, &own) == FVM_OK && !own))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, root, "prefix", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT && out.object == clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&arg, 2);
        if (!(fvm_object_call(c, root, "getInstanceAtDepth", 1, &arg, &out) == FVM_OK &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, clip, "removeMovieClip", 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clip = child(c, root, "missing", 3);
        if (!(fvm_object_delete(c, clip, "_name", &deleted) == FVM_OK && deleted))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, clip, "removeMovieClip", 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&arg, 3);
        if (!(fvm_object_call(c, root, "getInstanceAtDepth", 1, &arg, &out) == FVM_OK &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clip = child(c, root, "replaced", 4);
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = clip;
        if (!(fvm_object_set(c, root, "prefix", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_bytes(c, root, "prefix\0tail", 11, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string_bytes(&value, "prefix\0tail", 11) == FVM_OK &&
              fvm_object_set(c, clip, "_name", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        child(c, root, "replacement", 4);
        if (!(fvm_object_has_own_bytes(c, root, "prefix\0tail", 11, &own) == FVM_OK && !own))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, root, "prefix", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT && out.object == clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clip = child(c, root, "nameless", 5);
        if (!(fvm_object_delete(c, clip, "_name", &deleted) == FVM_OK && deleted))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        child(c, root, "replacement2", 5);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
