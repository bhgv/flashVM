/* Hidden inherited virtual setters are skipped; own hidden setters still dispatch. */
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
    const unsigned masks[] = {0x1000, 0x8000};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned mask = 0; mask < 2; ++mask)
        {
            for (unsigned kind = 0; kind < 2; ++kind)
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
                    FVM_Object *object = 0, *parent = 0;
                    if (!(fvm_construct(c, kind ? "Array" : "Object", 0, 0, &object) == FVM_OK &&
                          fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    const char *name = kind ? "0" : "field";
                    const unsigned char setter[] = {0x96, 6,  0,  0, 't', 'h', 'i',  's', 0,   0x1c,
                                                    0x96, 13, 0,  0, 'c', 'a', 'l',  'l', 'e', 'd',
                                                    0,    7,  81, 0, 0,   0,   0x4f, 0};
                    FVM_Object *function = 0;
                    if (!(fvm_function_create_avm1(c, setter, sizeof(setter), 0, 0, 0, &function) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value args[3], value, out;
                    fvm_value_string(args, name);
                    args[1] = class_value(version_function(c, 22));
                    args[2] = class_value(function);
                    if (!(fvm_object_call(c, parent, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_set_prototype(c, parent, object->prototype) == FVM_OK &&
                          fvm_object_set_prototype(c, object, parent) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_flags(c, parent, name, masks[mask], 0);
                    const bool hidden = !!(masks[mask] & version_masks[version - 4]);
                    value = class_value(object);
                    if (!(fvm_set_global(c, "versionTarget", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!route)
                    {
                        fvm_value_number(&value, 42);
                        if (!(fvm_object_set(c, object, name, &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        Program code;
                        code.variable("versionTarget");
                        code.text(name);
                        code.number(42);
                        code.op(0x4f);
                        code.number(0);
                        code.finish();
                        out = boxing_run(c, code, route - 1, FVM_OK);
                    }
                    int own = 0;
                    if (!(fvm_object_has_own(c, object, name, &own) == FVM_OK && !!own == hidden &&
                          fvm_object_get(c, object, "called", &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (hidden)
                    {
                        if (!(out.type == FVM_VALUE_UNDEFINED))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(out.number == 81))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    unsigned actual = 0;
                    if (!(fvm_object_property_flags(c, parent, name, &actual) == FVM_OK && actual == masks[mask]))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (kind)
                    {
                        if (!(fvm_object_get(c, object, "length", &out) == FVM_OK && out.number == (hidden ? 1 : 0)))
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
