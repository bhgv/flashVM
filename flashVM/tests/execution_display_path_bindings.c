/* Loaded root/parent paths are virtual, versioned and shadowable; host descriptors stay compatible. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/property.h"
#include <string.h>


static FVM_Result custom(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *a, FVM_Value *out, void *raw)
{
    (void)c;
    (void)self;
    (void)n;
    (void)a;
    (void)raw;
    fvm_value_number(out, 73);
    return FVM_OK;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned version;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (version = 0; version <= 8; ++version)
    {
        FVM_Context *c;
        FVM_Object *root, *child, *owner;
        FVM_Value args[2], value, key, out;
        int own, found;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (version && version < 4)
        {
            continue;
        }
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            swf[3] = (unsigned char)version;
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
        fvm_value_string(args, "child");
        fvm_value_number(args + 1, 1);
        if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        child = out.object;
        if (!(fvm_object_get(c, child, "_root", &out) == FVM_OK &&
              out.type == (version == 4 ? FVM_VALUE_UNDEFINED : FVM_VALUE_OBJECT)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version != 4)
        {
            if (!(out.object == root))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_object_get(c, child, "_parent", &out) == FVM_OK &&
              out.type == (version == 4 ? FVM_VALUE_UNDEFINED : FVM_VALUE_OBJECT)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version != 4)
        {
            if (!(out.object == root))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (version >= 5)
        {
            if (!(fvm_object_get(c, root, "_parent", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_string(&key, "this");
        if (!(fvm_exec_variable_lookup(c, 0, root, root, 0, 0, &key, &out, &owner, &found) == FVM_OK &&
              found == (version != 4)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version != 4)
        {
            if (!(out.object == root))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            fvm_value_string(&value, "assigned");
            if (!(fvm_exec_variable_set(c, 0, root, root, &key, &value) == FVM_OK &&
                  fvm_exec_get_variable(c, 0, root, root, 0, &key, &out) == FVM_OK && out.type == FVM_VALUE_STRING &&
                  !strcmp(out.string, "assigned")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (version)
        {
            if (!(fvm_object_has_own(c, child, "_root", &own) == FVM_OK && !own &&
                  fvm_object_has_own(c, child, "_parent", &own) == FVM_OK && !own))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(&key, "_root");
            fvm_value_string(&value, "root-binding");
            if (!(fvm_exec_variable_set(c, 0, child, child, &key, &value) == FVM_OK &&
                  fvm_object_get(c, child, "_root", &out) == FVM_OK && out.type == FVM_VALUE_STRING &&
                  !strcmp(out.string, "root-binding")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(&key, "_parent");
            fvm_value_string(&value, "parent-binding");
            if (!(fvm_exec_variable_set(c, 0, child, child, &key, &value) == FVM_OK &&
                  fvm_object_get(c, child, "_parent", &out) == FVM_OK && out.type == FVM_VALUE_STRING &&
                  !strcmp(out.string, "parent-binding")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_object_define_accessor_rooted(c, root, "_root", custom, 0, 0, 0) == FVM_OK &&
              fvm_object_get(c, root, "_root", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 73))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
