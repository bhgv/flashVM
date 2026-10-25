/* C89 intrinsic root-lock access is versioned, alias-safe and independent of activation state. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/property.h"


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
        FVM_Object *clip, *foreign, *plain;
        FVM_Value value, out, ctor, proto;
        unsigned flags;
        int removed;
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
        if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &clip) == FVM_OK &&
              fvm_gc_pin(c, clip) == FVM_OK && fvm_object_create(c, &plain) == FVM_OK &&
              fvm_object_create(other, &foreign) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_clip_lock_root(c, 0, clip, 0, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&out, 73);
        fvm_value_undefined(&value);
        if (!(fvm_exec_clip_lock_root(0, 0, clip, 0, &out) == FVM_INVALID_ARGUMENT &&
              fvm_exec_clip_lock_root(c, 0, plain, 0, &out) == FVM_INVALID_ARGUMENT &&
              fvm_exec_clip_lock_root(c, 0, foreign, 0, &out) == FVM_INVALID_ARGUMENT &&
              fvm_exec_clip_lock_root(c, 1, clip, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 73))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string(&value, "0") == FVM_OK &&
              fvm_exec_clip_lock_root(c, 1, clip, &value, &value) == FVM_OK && value.type == FVM_VALUE_BOOLEAN &&
              !!value.boolean == (!profile || profile + 3 >= 7)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, clip, "_lockroot", &out) == FVM_OK && out.boolean == value.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_boolean(&value, 1);
        if (!(fvm_object_set(c, clip, "_lockroot", &value) == FVM_OK &&
              fvm_exec_clip_lock_root(c, 0, clip, 0, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "MovieClip", &ctor) == FVM_OK &&
              fvm_object_get(c, ctor.object, "prototype", &proto) == FVM_OK &&
              fvm_object_get(c, proto.object, "_lockroot", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_property_flags(c, proto.object, "_lockroot", &flags) == FVM_OK &&
              (flags & (FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE)) ==
                  (FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_delete(c, proto.object, "_lockroot", &removed) == FVM_OK && !removed))
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
