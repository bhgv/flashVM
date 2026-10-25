/* __resolve and primitive projection skip version-hidden local callbacks without invoking getters. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"


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
        for (unsigned kind = 0; kind < 3; ++kind)
        {
            for (unsigned route = 0; route < 4; ++route)
            {
                if (route >= 2 && !fvm_jit_available())
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
                FVM_Object *child = 0, *parent = 0;
                if (!(fvm_construct(c, "Object", 0, 0, &child) == FVM_OK &&
                      fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK &&
                      fvm_object_set_prototype(c, child, parent) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const char *name = kind == 2 ? "valueOf" : "__resolve";
                FVM_Value value = class_value(version_function(c, 91)), out;
                if (!(fvm_object_set(c, parent, name, &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                value = class_value(version_function(c, 22));
                if (!(fvm_object_set(c, child, name, &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (kind == 1)
                {
                    FVM_Value args[3];
                    fvm_value_string(args, name);
                    args[1] = class_value(version_function(c, 55));
                    args[2] = class_value(version_function(c, 0));
                    if (!(fvm_object_call(c, child, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                version_flags(c, child, name, 0x1000, 0);
                value = class_value(child);
                if (!(fvm_set_global(c, "versionTarget", &value) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!route)
                {
                    if (kind == 2)
                    {
                        if (!(fvm_exec_to_number(c, 0, &value, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(fvm_object_get(c, child, "missing", &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                else
                {
                    Program code;
                    code.variable("versionTarget");
                    if (kind == 2)
                    {
                        code.op(0x4a);
                    }
                    else
                    {
                        code.text("missing");
                        code.op(0x4e);
                    }
                    code.finish();
                    out = boxing_run(c, code, route - 1, FVM_OK);
                }
                if (!(out.type == FVM_VALUE_NUMBER && out.number == (version < 8 ? 91 : 22)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!route)
                {
                    FVM_Context *other = 0;
                    unsigned char *data = 0;
                    size_t bytes = 0;
                    if (!(fvm_context_create(rt, &other) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_load(other, version);
                    if (!(fvm_value_serialize(c, &value, &data, &bytes) == FVM_OK &&
                          fvm_value_deserialize(other, data, bytes, &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_serialized_free(data);
                    if (!(fvm_gc_pin(other, value.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (kind == 2)
                    {
                        if (!(fvm_exec_to_number(other, 0, &value, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(fvm_object_get(other, value.object, "missing", &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == (version < 8 ? 91 : 22)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(other);
                }
                if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_owner))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
