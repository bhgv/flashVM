/* Invalidating lazy method caches releases hidden captures but preserves exported Functions. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"


static bool live_capture(FVM_Context *c, FVM_Object *root)
{
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        if (c->objects[i] == root)
        {
            return true;
        }
    }
    return false;
}


static FVM_Result captured(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *data)
{
    return fvm_object_get(c, (FVM_Object *)data, "value", out);
}


static FVM_Result replacement(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_number(out, 2);
    return FVM_OK;
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
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (unsigned operation = 0; operation < 4; ++operation)
        {
            for (unsigned keep = 0; keep < 2; ++keep)
            {
                if (version && version < 4)
                {
                    continue;
                }
                FVM_Context *c = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (version)
                {
                    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                    swf[3] = (unsigned char)version;
                    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_standard_install(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *object = 0, *root = 0;
                FVM_Value value, old, same, out;
                if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK &&
                      fvm_object_create(c, &root) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_number(&value, 1);
                if (!(fvm_object_set(c, root, "value", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_define_method_rooted_bytes(c, object, "MiXeD\0Name", 10, captured, root, root) ==
                      FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get_bytes(c, object, "MiXeD\0Name", 10, &old) == FVM_OK &&
                      old.type == FVM_VALUE_OBJECT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (keep)
                {
                    if (!(fvm_gc_pin(c, old.object) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                /* An identical definition preserves the lazy Function identity. */
                if (!(fvm_object_define_method_rooted_bytes(c, object, "MiXeD\0Name", 10, captured, root, root) ==
                      FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get_bytes(c, object, "MiXeD\0Name", 10, &same) == FVM_OK && same.object == old.object))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const char *query = version && version <= 6 ? "mixed\0name" : "MiXeD\0Name";
                if (operation == 0)
                {
                    if (!(fvm_object_define_method_rooted_bytes(c, object, query, 10, replacement, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (operation == 1)
                {
                    int deleted = 0;
                    if (!(fvm_object_delete_bytes(c, object, query, 10, &deleted) == FVM_OK && deleted))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (operation == 2)
                {
                    fvm_value_number(&value, 23);
                    if (!(fvm_object_set_bytes(c, object, query, 10, &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(fvm_object_define_accessor_rooted_bytes(c, object, query, 10, replacement, 0, 0, 0) ==
                          FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK && live_capture(c, root) == (keep != 0)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (keep)
                {
                    if (!(fvm_function_invoke(c, old.object, object, 0, 0, &out) == FVM_OK && out.number == 1))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_gc_unpin(c, old.object) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK &&
                          !live_capture(c, root)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(!c->gc_frames && !c->gc_native_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
