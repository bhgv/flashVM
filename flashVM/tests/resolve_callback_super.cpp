/* Missing-property resolver callbacks receive depth-zero super regardless of storage. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_adapter_super.cpp"
#include "../src/core/call_scope_constants.h"


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
        for (unsigned kind = 0; kind < 5; ++kind)
        {
            for (unsigned chain = 0; chain < 4; ++chain)
            {
                for (unsigned location = 0; location <= chain; ++location)
                {
                    for (unsigned route = 0; route < 4; ++route)
                    {
                        if ((kind && version < 7) || (route >= 2 && !fvm_jit_available()))
                        {
                            continue;
                        }
                        FVM_Context *c = 0;
                        if (!(fvm_context_create(rt, &c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        version_load(c, version);
                        if (!(fvm_standard_install(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Object *nodes[4];
                        FVM_Value value, out;
                        for (unsigned i = 0; i < 4; ++i)
                        {
                            if (!(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK &&
                                  fvm_gc_pin(c, nodes[i]) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            fvm_value_number(&value, i);
                            if (!(fvm_object_set(c, nodes[i], "marker", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        value = class_value(adapter_super_function(c, kind));
                        if (!(fvm_object_set(c, nodes[location], "__resolve", &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        for (unsigned i = 0; i < 4; ++i)
                        {
                            if (!(fvm_object_set_prototype(c, nodes[i], i < chain ? nodes[i + 1] : 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        value = class_value(nodes[0]);
                        if (!(fvm_set_global(c, "resolveSuperTarget", &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!route)
                        {
                            if (!(fvm_object_get_bytes(c, nodes[0], "n\0m", 3, &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            fvm_value_string_bytes(&value, "n\0m", 3);
                            if (!(fvm_set_global(c, "resolveSuperKey", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            Program code;
                            code.variable("resolveSuperTarget");
                            code.variable("resolveSuperKey");
                            code.op(0x4e);
                            code.finish();
                            out = boxing_run(c, code, route - 1, FVM_OK);
                        }
                        if (kind == 2 || kind == 3)
                        {
                            if (!(out.type == FVM_VALUE_UNDEFINED))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(out.type == FVM_VALUE_OBJECT && out.object->resource_kind == FVM_RESOURCE_SUPER &&
                                  out.object->resource_result.object->properties["depth"].number == 0))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Object *facade = out.object;
                            if (!(fvm_gc_pin(c, facade) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            /* Avoid a second resolver call when checking an exhausted prototype chain. */
                            for (unsigned i = 0; i <= chain; ++i)
                            {
                                int removed = 0;
                                if (!(fvm_object_delete(c, nodes[i], "__resolve", &removed) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            if (!(fvm_object_get(c, facade, "marker", &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (chain)
                            {
                                if (!(out.type == FVM_VALUE_NUMBER && out.number == 1))
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
                        }
                        if (!(!c->resolve_depth && !c->gc_frames && !c->gc_native_depth && !c->calling_function &&
                              !c->calling_owner && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_context_destroy(c);
                    }
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
