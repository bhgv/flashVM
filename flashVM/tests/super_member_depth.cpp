/* Explicit-super methods capture facade-relative lookup depth before getters execute. */
#define FVM_PROPERTY_CALLBACK_SUPER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "property_callback_super.cpp"
#include "../src/core/call_scope.hpp"
#include "../src/standard/super.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *keys[] = {"slot", "5", "n\0m"};
    const size_t sizes[] = {4, 1, 3};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned kind = 0; kind < 5; ++kind)
        {
            for (unsigned base = 0; base < 3; ++base)
            {
                for (unsigned location = 0; location < 3; ++location)
                {
                    for (unsigned virtual_slot = 0; virtual_slot < 2; ++virtual_slot)
                    {
                        for (unsigned key = 0; key < 3; ++key)
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
                                FVM_Object *nodes[7];
                                FVM_Value value, out;
                                for (unsigned i = 0; i < 7; ++i)
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
                                FVM_Object *method = adapter_super_function(c, kind);
                                value = class_value(method);
                                if (!virtual_slot)
                                {
                                    if (!(fvm_object_set_bytes(
                                              c, nodes[base + location + 1], keys[key], sizes[key], &value) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                else
                                {
                                    if (!(fvm_object_set(c, nodes[0], "returned", &value) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                    Program body;
                                    body.variable("this");
                                    body.text("returned");
                                    body.op(0x4e);
                                    body.finish();
                                    FVM_Object *getter = 0;
                                    if (!(fvm_function_create_avm1(
                                              c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &getter) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                    callback_super_property(c,
                                                            nodes[base + location + 1],
                                                            keys[key],
                                                            sizes[key],
                                                            getter,
                                                            version_function(c, 0));
                                }
                                for (unsigned i = 0; i < 7; ++i)
                                {
                                    if (!(fvm_object_set_prototype(c, nodes[i], i < 6 ? nodes[i + 1] : 0) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                {


                                    FVM_CallScope scope(c, 0, method, 0, base);
                                    if (!(fvm_super_create(c, method, nodes[0], &value) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                FVM_Object *target = value.object;
                                if (!(fvm_set_global(c, "memberDepthTarget", &value) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                fvm_value_string_bytes(&value, keys[key], sizes[key]);
                                if (!(fvm_set_global(c, "memberDepthKey", &value) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                if (!route)
                                {
                                    if (!(fvm_object_call_bytes(c, target, keys[key], sizes[key], 0, 0, &out) ==
                                          FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                else
                                {
                                    Program code;
                                    code.number(0);
                                    code.variable("memberDepthTarget");
                                    code.variable("memberDepthKey");
                                    code.op(0x52);
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
                                    const unsigned depth = base + location + 1;
                                    if (!(out.type == FVM_VALUE_OBJECT &&
                                          out.object->resource_kind == FVM_RESOURCE_SUPER &&
                                          out.object->resource_result.object->properties["depth"].number == depth))
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
                                    if (!(fvm_object_get(c, facade, "marker", &out) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                    if (!(out.type == FVM_VALUE_NUMBER && out.number == depth + 1))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_owner &&
                                      !c->calling_function &&
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
    }
    fvm_runtime_destroy(rt);
}
