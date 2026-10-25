/* Delete2 resolves its owner through versioned GetVariable grammar before final deletion. */
#define main paths_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_paths.cpp"

#undef main
#include "flashvm/execution.h"


static bool remove_name(FVM_Context *c, FVM_Object *receiver, const char *key)
{
    Bytes bytes;
    text(bytes, key);
    bytes.push_back(0x3b);
    bytes.push_back(0x3e);
    bytes.push_back(0);
    FVM_Object *fn = 0;
    FVM_Value out;
    if (!(fvm_function_create_avm1(c, bytes.begin(), bytes.size(), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_invoke(c, fn, receiver, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return !!out.boolean;
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
    for (unsigned version = 5; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *root = 0;
        if (!(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *a = child(c, root, "a", 1), *b = child(c, a, "b", 2);
        FVM_Value value;
        fvm_value_number(&value, 17);
        if (!(fvm_object_set(c, b, "victim", &value) == FVM_OK && remove_name(c, root, "a/b:victim")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!remove_name(c, root, "a/b:victim")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *dot = child(c, a, "x.y", 3);
        if (!(fvm_object_set(c, dot, "victim", &value) == FVM_OK && !remove_name(c, root, "a/x.y:victim")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, b, "victim", &value) == FVM_OK && remove_name(c, b, "/a/b:victim")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, a, "victim", &value) == FVM_OK && remove_name(c, b, "_parent/:victim")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, b, "slash/name", &value) == FVM_OK && remove_name(c, root, "a/b:slash/name")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version >= 5)
        {
            if (!(fvm_object_set(c, b, "victim", &value) == FVM_OK && remove_name(c, root, "this/a/b:victim")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_set(c, b, "victim", &value) == FVM_OK && remove_name(c, b, "_root/a/b:victim")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
