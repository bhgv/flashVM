/* Transported Function/Function2 values retain Undefined actual this and destination caller cells. */
#define FVM_UNDEFINED_THIS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "anonymous_undefined_this.cpp"


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
        for (unsigned kind = 0; kind < 11; ++kind)
        {
            for (unsigned route = 0; route < 4; ++route)
            {
                if (kind && kind != 9 && version < 7)
                {
                    continue;
                }
                if (route >= 2 && !fvm_jit_available())
                {
                    continue;
                }
                FVM_Context *c = 0, *other = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                version_load(c, version);
                version_load(other, version);
                if (!(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value value = class_value(undefined_this_function(c, kind)), out, name;
                unsigned char *wire = 0;
                size_t size = 0;
                if (!(fvm_value_serialize(c, &value, &wire, &size) == FVM_OK &&
                      fvm_value_deserialize(other, wire, size, &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_serialized_free(wire);
                if (!(fvm_set_global(other, "anonymousFunction", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *parent = 0;
                if (!(fvm_construct(other, "Object", 0, 0, &parent) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_number(&out, 81);
                if (!(fvm_object_set(other, parent, "marker", &out) == FVM_OK &&
                      fvm_object_set_prototype(other, other->global, parent) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!route)
                {
                    int removed = 0;
                    if (!(fvm_object_delete(other, other->global, "anonymousFunction", &removed) == FVM_OK && removed))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_undefined(&name);
                    if (!(fvm_exec_call_method(other, 0, &value, &name, other->global, 0, 0, &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    Program code;
                    code.number(0);
                    code.variable("anonymousFunction");
                    code.variable("this");
                    code.text("anonymousFunction");
                    code.op(0x3a);
                    code.op(0x17);
                    code.text("");
                    code.op(0x52);
                    code.finish();
                    out = boxing_run(other, code, route - 1, FVM_OK);
                }
                if (kind == 3 || kind == 5 || kind == 10)
                {
                    if (!(out.type == FVM_VALUE_OBJECT && out.object == other->global))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(out.type == FVM_VALUE_UNDEFINED))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_gc_collect(other, 0) == FVM_OK &&
                      fvm_object_get(other, other->global, "marker", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
                      out.number == 81))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!other->calling_this_undefined && !other->script_this_undefined && !other->script_this &&
                      !other->gc_frames && !other->gc_native_depth && !other->calling_function &&
                      !other->calling_owner))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(other);
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
