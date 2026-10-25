/* Version flags affect ordinary reads/calls, but not raw membership, keys or Array data. */
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
    const unsigned bits[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65535};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned flag = 0; flag < sizeof(bits) / sizeof(bits[0]); ++flag)
        {
            for (unsigned kind = 0; kind < 4; ++kind)
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
                    FVM_Object *target = 0, *parent = 0;
                    if (!(fvm_construct(c, kind == 3 ? "Array" : "Object", 0, 0, &target) == FVM_OK &&
                          fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    const char *name = kind == 3 ? "0" : "field";
                    FVM_Value value, original, out;
                    fvm_value_number(&value, 11);
                    if (!(fvm_object_set(c, parent, name, &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (kind == 0 || kind == 3)
                    {
                        fvm_value_number(&value, 22);
                        if (!(fvm_object_set(c, target, name, &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (kind == 1)
                    {
                        FVM_Value args[3];
                        fvm_value_string(args, name);
                        args[1] = class_value(version_function(c, 22));
                        args[2] = class_value(version_function(c, 0));
                        if (!(fvm_object_call(c, target, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (kind == 2)
                    {
                        FVM_Value klass, proto;
                        if (!(fvm_get_global(c, "Object", &klass) == FVM_OK &&
                              fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        target->methods[name] = proto.object->methods.find("valueOf")->second;
                        value = class_value(version_function(c, 91));
                        if (!(fvm_object_set(c, parent, name, &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_object_get(c, target, name, &original) == FVM_OK &&
                          fvm_object_set_prototype(c, parent, target->prototype) == FVM_OK &&
                          fvm_object_set_prototype(c, target, parent) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value = class_value(target);
                    if (!(fvm_set_global(c, "versionTarget", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_flags(c, target, name, bits[flag], 0, route);
                    const bool visible = !(bits[flag] & version_masks[version - 4]);
                    if (!route)
                    {
                        if (!(fvm_object_get(c, target, name, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        class_mode = route - 1;
                        Program code;
                        code.variable("versionTarget");
                        code.text(name);
                        code.op(0x4e);
                        code.finish();
                        out = boxing_run(c, code, route - 1, FVM_OK);
                    }
                    if (kind == 2)
                    {
                        if (!(out.type == FVM_VALUE_OBJECT))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (visible)
                        {
                            if (!(out.object == original.object))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!(fvm_object_call(c, target, name, 0, 0, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (visible)
                        {
                            if (!(out.type == FVM_VALUE_OBJECT && out.object == target))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(out.type == FVM_VALUE_NUMBER && out.number == 91))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    else
                    {
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == (visible ? 22 : 11)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    int own = 0;
                    if (!(fvm_object_has_own(c, target, name, &own) == FVM_OK && own))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_enumerable(c, target, name, !(bits[flag] & 1));
                    fvm_value_string(&value, name);
                    if (!(fvm_object_call(c, target, "isPropertyEnumerable", 1, &value, &out) == FVM_OK &&
                          out.type == FVM_VALUE_BOOLEAN && !!out.boolean == !(bits[flag] & 1)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (kind == 3)
                    {
                        if (!(fvm_object_call(c, target, "slice", 0, 0, &out) == FVM_OK &&
                              fvm_table_get_index(c, out.object, 0, &out) == FVM_OK && out.number == 22))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    unsigned actual = 0;
                    if (!(fvm_object_property_flags(c, target, name, &actual) == FVM_OK && actual == bits[flag]))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    /* Hidden fields remain reachable for attribute clearing, without ordinary lookup. */
                    version_flags(c, target, name, 0, 65535, route);
                    if (!(fvm_object_get(c, target, name, &out) == FVM_OK && out.type == original.type))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (out.type == FVM_VALUE_NUMBER)
                    {
                        if (!(out.number == 22))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!c->gc_frames && !c->gc_native_depth))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
