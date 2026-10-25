/* Loaded action dispatch and interface traversal distinguish interfaces from their prototype links. */
#define main existing_inheritance_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_inheritance.cpp"

#undef main
#include "flashvm/execution.h"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 7; version <= 8; ++version)
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
        FVM_Value sub = function(c, "Sub"), iface = function(c, "Iface"), parent = function(c, "Parent"),
                  other = function(c, "Other"), iface_proto, other_proto, value, type, out;
        implements(c, "Sub", "Iface");
        implements(c, "Iface", "Parent");
        implements(c, "Parent", "Iface");
        if (!(fvm_object_get(c, iface.object, "prototype", &iface_proto) == FVM_OK &&
              fvm_object_get(c, other.object, "prototype", &other_proto) == FVM_OK &&
              fvm_object_set_prototype(c, iface_proto.object, other_proto.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *object = 0;
        if (!(fvm_function_construct(c, sub.object, 0, 0, &object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = object;
        if (!(fvm_set_global(c, "instance", &value) == FVM_OK &&
              fvm_exec_instance_of(c, &value, &iface, &out) == FVM_OK && out.boolean &&
              fvm_exec_instance_of(c, &value, &parent, &out) == FVM_OK && out.boolean &&
              fvm_exec_instance_of(c, &value, &other, &out) == FVM_OK && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes code;
        text(code, "result");
        variable(code, "instance");
        variable(code, "Iface");
        code.push_back(0x54);
        code.push_back(0x1d);
        run(c, code);
        if (!(fvm_get_global(c, "result", &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        cast(c, "Iface", true);
        cast(c, "Other", false);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
