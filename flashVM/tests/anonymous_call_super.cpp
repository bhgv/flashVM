/* Loaded anonymous CallMethod invokes Functions at depth zero independently of storage. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_adapter_super.cpp"
#include "../src/core/call_scope.hpp"


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
                for (unsigned storage = 0; storage < 5; ++storage)
                {
                    for (unsigned undefined_name = 0; undefined_name < 2; ++undefined_name)
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
                            nodes[0] = c->global;
                            FVM_Value value, out;
                            for (unsigned i = 1; i < 4; ++i)
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
                            FVM_Object *function = adapter_super_function(c, kind);
                            value = class_value(function);
                            if (!(fvm_set_global(c, "anonymousFunction", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (kind)
                            {
                                int removed = 0;
                                if (!(fvm_object_delete(c, c->global, "adapterFunction2", &removed) == FVM_OK &&
                                      removed))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            if (storage)
                            {
                                if (!(fvm_object_set(c, nodes[storage - 1], "storedFunction", &value) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            for (unsigned i = 0; i < 4; ++i)
                            {
                                if (!(fvm_object_set_prototype(c, nodes[i], i < chain ? nodes[i + 1] : 0) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            if (!route)
                            {
                                int removed = 0;
                                if (!(fvm_object_delete(c, c->global, "anonymousFunction", &removed) == FVM_OK &&
                                      removed))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                FVM_Value name;
                                if (undefined_name)
                                {
                                    fvm_value_undefined(&name);
                                }
                                else
                                {
                                    fvm_value_string(&name, "");
                                }
                                if (!(fvm_exec_call_method(c, 0, &value, &name, c->global, 0, 0, &out) == FVM_OK))
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
                                if (undefined_name)
                                {
                                    code.variable("undefined");
                                }
                                else
                                {
                                    code.text("");
                                }
                                code.op(0x52);
                                code.finish();
                                out = boxing_run(c, code, route - 1, FVM_OK);
                            }
                            if (!(out.type == FVM_VALUE_UNDEFINED))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_gc_collect(c, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(!c->calling_this_undefined && !c->script_this_undefined && !c->script_this &&
                                  !c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_owner &&
                                  c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH))
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
    }
    fvm_runtime_destroy(rt);
}
