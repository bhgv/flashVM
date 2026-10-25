/* Explicit-base deletion shares display resolution but never reads the final member. */
#define main paths_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_paths.cpp"

#undef main
#include "flashvm/execution.h"


static void remove_path(FVM_Context *c, FVM_Object *root, const char *path, size_t size, bool expected)
{
    FVM_Value base, text, out;
    fvm_value_undefined(&base);
    base.type = FVM_VALUE_OBJECT;
    base.object = root;
    if (!(fvm_value_string_bytes(&text, path, size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_delete_path(c, &base, &text, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN &&
          !!out.boolean == expected))
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
        FVM_Object *a = child(c, root, "a", 1), *b = child(c, a, "x.y", 2);
        FVM_Value value;
        fvm_value_number(&value, 17);
        if (!(fvm_object_set(c, b, "victim", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        remove_path(c, root, "a/x.y:victim", 12, true);
        remove_path(c, root, "a/x.y:victim", 12, false);
        if (!(fvm_object_set(c, b, "victim", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        remove_path(c, root, "A/x.y:victim", 12, version < 7);
        if (version >= 7)
        {
            remove_path(c, root, "a/x.y:victim", 12, true);
        }
        if (!(fvm_object_set(c, a, "victim", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        remove_path(c, b, "../:victim", 10, true);
        if (!(fvm_object_set(c, b, "a/b", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        remove_path(c, root, "a/x.y:a/b", 9, true);
        if (!(fvm_object_set_bytes(c, b, "a\0b", 3, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        remove_path(c, root, "a/x.y:a\0b", 9, true);
        remove_path(c, root, "missing/path:key", 16, false);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
