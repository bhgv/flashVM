/* Function calls preserve the raw first argument; empty loaded construction is ordinary. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/property.h"
#include "flashvm/serialization.h"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4)
        {
            continue;
        }
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value klass, proto, input, out;
        if (!(fvm_get_global(c, "Function", &klass) == FVM_OK &&
              fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned kind = 0; kind < 8; ++kind)
        {
            fvm_value_undefined(&input);
            if (kind == 2)
            {
                input.type = FVM_VALUE_NULL;
            }
            if (kind == 3)
            {
                fvm_value_number(&input, 42);
            }
            if (kind == 4)
            {
                fvm_value_string(&input, "a");
            }
            if (kind == 5)
            {
                input = proto;
            }
            if (kind == 6)
            {
                fvm_value_boolean(&input, 1);
            }
            if (kind == 7)
            {
                if (!(fvm_value_string_bytes(&input, "a\0b", 3) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            unsigned count = kind ? 1 : 0;
            if (!(fvm_set_global(c, "input", &input) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned route = 0; route < (version ? 11U : 5U); ++route)
            {
                if (route == 0)
                {
                    if (!(fvm_call(c, "Function", count, count ? &input : 0, &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (route == 1)
                {
                    if (!(fvm_function_invoke(c, klass.object, 0, count, count ? &input : 0, &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!fvm_jit_available())
                    {
                        continue;
                    }
                    class_mode = route < 5 ? route - 2 : (route - 5) % 3;
                    Program code;
                    if (count)
                    {
                        code.variable("input");
                    }
                    code.number(count);
                    if (route >= 8)
                    {
                        code.variable("Function");
                        code.text("");
                        code.op(0x53);
                    }
                    else
                    {
                        code.text("Function");
                        code.op(route >= 5 ? 0x40 : 0x3d);
                    }
                    code.finish();
                    out = class_run(c, code);
                }
                if (!version)
                {
                    if (!(out.type == FVM_VALUE_OBJECT && out.object->invocable.callback))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (!count)
                {
                    if (!(out.type == FVM_VALUE_OBJECT && out.object->prototype == (route >= 5 ? proto.object : 0) &&
                          !out.object->invocable.callback))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(out.type == input.type))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (kind == 3)
                    {
                        if (!(out.number == 42))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (kind == 4)
                    {
                        if (!(fvm_string_size(out) == 1 && out.string[0] == 'a'))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (kind == 5)
                    {
                        if (!(out.object == proto.object))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (kind == 6)
                    {
                        if (!(out.boolean))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (kind == 7)
                    {
                        if (!(fvm_string_size(out) == 3 && !std::memcmp(out.string, "a\0b", 3)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
        }
        if (version)
        {
            FVM_Object *rejected = (FVM_Object *)1;
            fvm_value_number(&input, 42);
            if (!(fvm_function_construct(c, klass.object, 1, &input, &rejected) == FVM_ERROR && !rejected))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_Object *instance = 0;
        if (!(fvm_function_construct(c, klass.object, 0, 0, &instance) == FVM_OK &&
              instance->prototype == proto.object))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(version ? !instance->invocable.callback : !!instance->invocable.callback))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            FVM_Value identity;
            if (!(fvm_object_get(c, instance, "__constructor__", &identity) == FVM_OK &&
                  identity.object == klass.object))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_construct(c, klass.object, 1, &proto, &instance) == FVM_OK && instance == proto.object))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (version && fvm_jit_available())
        {
            for (unsigned mode = 0; mode < 3; ++mode)
            {
                for (unsigned argument = 0; argument < 2; ++argument)
                {
                    if (!(fvm_set_global(c, "input", &proto) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    class_mode = mode;
                    Program code;
                    if (argument)
                    {
                        code.variable("input");
                    }
                    code.number(argument);
                    code.text("Function");
                    code.op(0x40);
                    code.finish();
                    out = class_run(c, code);
                    if (!(out.type == FVM_VALUE_OBJECT))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (argument)
                    {
                        if (!(out.object == proto.object))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(!out.object->invocable.callback && out.object->prototype == proto.object))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
        }
        FVM_Context *other = 0;
        if (!(fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        unsigned char *data = 0;
        size_t size = 0;
        if (!(fvm_value_serialize(c, &klass, &data, &size) == FVM_OK &&
              fvm_value_deserialize(other, data, size, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(data);
        FVM_Object *copy = out.object;
        if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&input, 29);
        if (!(fvm_function_invoke(other, copy, 0, 1, &input, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(version ? out.type == FVM_VALUE_NUMBER && out.number == 29 : out.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
