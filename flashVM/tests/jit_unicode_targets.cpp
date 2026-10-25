/* Unicode display names select identical interpreter and explicit/automatic native targets. */
#define FVM_TARGET_FIXTURE_ONLY
#define FVM_TARGET_COMPARE_FIXTURE
#include <stdio.h>
#include <stdlib.h>
#include "jit_targets.cpp"


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
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
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *root = target_setup(c), *selected = target_child(c, root, "\xc3\x84", 1);
        FVM_Value value;
        fvm_value_string(&value, "\xc3\xa4");
        if (!(fvm_set_global(c, "path", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Program dynamic;
        dynamic.variable("path");
        dynamic.op(0x20);
        target_finish(dynamic);
        target_compare(c, root, dynamic, version < 7 ? selected : 0);
        if (version >= 6)
        {
            Program literal;
            target_literal(literal, "\xc3\xa4");
            target_finish(literal);
            target_compare(c, root, literal, version < 7 ? selected : 0);
        }
        fvm_value_string(&value, "\xc3\x84");
        if (!(fvm_set_global(c, "path", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        target_compare(c, root, dynamic, selected);
        target_throws = true;
        target_compare(c, root, dynamic, selected, FVM_EXCEPTION);
        target_throws = false;
        fvm_value_string(&value, "\xc4\xb0x");
        if (!(fvm_object_set(c, selected, "_name", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&value, "iX");
        if (!(fvm_set_global(c, "path", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        target_compare(c, root, dynamic, version < 7 ? selected : 0);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
