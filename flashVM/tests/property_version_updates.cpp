/* Successful writable data updates clear version requirements; readonly writes retain them. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned masks[] = {0x1000, 0x8000, 0xfff8};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned flag = 0; flag < 3; ++flag)
        {
            for (unsigned kind = 0; kind < 3; ++kind)
            {
                for (unsigned readonly = 0; readonly < 2; ++readonly)
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
                        FVM_Object *object = 0;
                        if (!(fvm_construct(c, kind == 2 ? "Array" : "Object", 0, 0, &object) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        const char *name = kind == 2 ? "0" : "field";
                        FVM_Value value, out;
                        fvm_value_number(&value, 22);
                        if (!(fvm_object_set(c, object, name, &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (kind == 1)
                        {
                            const unsigned char setter[] = {0x96, 6,  0,  0, 't', 'h', 'i',  's', 0,   0x1c,
                                                            0x96, 13, 0,  0, 'c', 'a', 'l',  'l', 'e', 'd',
                                                            0,    7,  81, 0, 0,   0,   0x4f, 0};
                            FVM_Object *function = 0;
                            if (!(fvm_function_create_avm1(c, setter, sizeof(setter), 0, 0, 0, &function) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Value args[3];
                            fvm_value_string(args, name);
                            args[1] = class_value(version_function(c, 22));
                            args[2] = class_value(function);
                            if (!(fvm_object_call(c, object, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        version_flags(c, object, name, masks[flag] | (readonly ? 4U : 0U), 0);
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
                        unsigned actual = 0;
                        if (!(fvm_object_property_flags(c, object, name, &actual) == FVM_OK &&
                              actual == (readonly ? masks[flag] | 4U : 0U)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (kind == 1)
                        {
                            if (!(fvm_object_get(c, object, "called", &out) == FVM_OK && out.number == 81))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_base_accessor_data(object->accessors.find(name)->second, out) &&
                                  out.type == FVM_VALUE_NUMBER && out.number == (readonly ? 22 : 42)))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else if (kind == 2)
                        {
                            if (!(object->indexed.find(0)->second.number == (readonly ? 22 : 42)))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(object->properties.find(name)->second.number == (readonly ? 22 : 42)))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
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
