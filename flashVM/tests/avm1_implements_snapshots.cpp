/* ImplementsOp freezes prototype identities once; object counts never call valueOf. */
#define main existing_inheritance_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_inheritance.cpp"

#undef main
#include "flashvm/execution.h"


static FVM_Result forbidden_count(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    if (!(0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_ERROR;
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
        FVM_Value sub = function(c, "Sub"), iface = function(c, "Iface"), other = function(c, "Other"), old, new_value,
                  alias, value, out;
        if (!(fvm_object_get(c, iface.object, "prototype", &old) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *count = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &count) == FVM_OK &&
              fvm_object_define_method_rooted(c, count, "valueOf", forbidden_count, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = count;
        if (!(fvm_set_global(c, "Count", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes noop;
        variable(noop, "Count");
        variable(noop, "Sub");
        noop.push_back(0x2c);
        run(c, noop);
        implements(c, "Sub", "Iface");
        FVM_Object *instance = 0, *replacement = 0, *old_alias = 0;
        if (!(fvm_function_construct(c, sub.object, 0, 0, &instance) == FVM_OK &&
              fvm_object_create(c, &replacement) == FVM_OK && fvm_object_create(c, &old_alias) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = instance;
        if (!(fvm_set_global(c, "instance", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        new_value = value;
        new_value.object = replacement;
        if (!(fvm_object_set(c, iface.object, "prototype", &new_value) == FVM_OK &&
              fvm_object_set(c, old_alias, "prototype", &old) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        alias = value;
        alias.object = old_alias;
        if (!(fvm_exec_instance_of(c, &value, &alias, &out) == FVM_OK && out.boolean == (version >= 7) &&
              fvm_exec_instance_of(c, &value, &iface, &out) == FVM_OK && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        implements(c, "Sub", "Other");
        if (!(fvm_exec_instance_of(c, &value, &other, &out) == FVM_OK && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
