/* Native Array methods follow own length data after deletion and ordinary recreation. */
#define FVM_APPLY_DELETED_LENGTH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_apply_deleted_length.cpp"


static FVM_Object *array_deleted_fixture(FVM_Context *c, unsigned kind, FVM_Value &raw, unsigned &count)
{
    FVM_Object *array = 0;
    fvm_value_undefined(&raw);
    count = 0;
    if (!kind)
    {
        FVM_Value pair[2];
        fvm_value_number(pair, 11);
        fvm_value_number(pair + 1, 22);
        if (!(fvm_construct(c, "Array", 2, pair, &array) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&raw, 2);
        count = 2;
        return array;
    }
    array = deleted_length_array(c);
    if (kind >= 2 && kind <= 7)
    {
        if (kind == 2)
        {
            fvm_value_string(&raw, "1");
        }
        if (kind == 3)
        {
            fvm_value_number(&raw, 1);
        }
        if (kind == 4)
        {
            fvm_value_number(&raw, 1.9);
        }
        if (kind == 5)
        {
            fvm_value_boolean(&raw, 1);
        }
        if (kind == 6)
        {
            raw.type = FVM_VALUE_NULL;
        }
        count = kind <= 5 ? 1 : 0;
        if (!(fvm_object_set(c, array, "length", &raw) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (kind == 8 || kind == 9)
    {
        FVM_Object *proto = 0, *old = array->prototype;
        if (!(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK && fvm_object_set_prototype(c, proto, old) == FVM_OK &&
              fvm_object_set_prototype(c, array, proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_number(&value, 2);
        if (!(fvm_object_set(c, proto, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (kind == 9)
        {
            Program p;
            p.number(77);
            p.op(0x2a);
            p.op(0);
            FVM_Object *fn = 0;
            if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value args[3], out;
            fvm_value_string(args, "length");
            args[1] = class_value(fn);
            fvm_value_undefined(args + 2);
            args[2].type = FVM_VALUE_NULL;
            if (!(fvm_object_call(c, proto, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (kind == 10)
    {
        fvm_value_number(&raw, 1);
        if (!(fvm_object_set(c, array, "LeNgTh", &raw) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        count = c->swf_info.version < 7 ? 1 : 0;
    }
    return array;
}


static FVM_Value array_deleted_call(FVM_Context *c, FVM_Object *array, const char *method, unsigned route)
{
    FVM_Value target = class_value(array), arg, out;
    if (!(fvm_set_global(c, "arrayDeletedTarget", &target) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned argc = 0;
    fvm_value_number(&arg, 81);
    if (!std::strcmp(method, "push") || !std::strcmp(method, "unshift"))
    {
        argc = 1;
    }
    if (!route)
    {
        if (!(fvm_object_call(c, array, method, argc, &arg, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        Program p;
        if (argc)
        {
            p.number(81);
        }
        p.number(argc);
        p.variable("arrayDeletedTarget");
        p.text(method);
        p.op(0x52);
        p.finish();
        out = boxing_run(c, p, route - 1, FVM_OK);
    }
    return out;
}
#ifndef FVM_ARRAY_DELETED_LENGTH_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *methods[] = {"join", "slice", "concat", "reverse", "sort", "pop", "shift", "push", "unshift"};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned kind = 0; kind <= 10; ++kind)
        {
            for (unsigned method = 0; method < 9; ++method)
            {
                for (unsigned route = 0; route < 4; ++route)
                {
                    for (unsigned transport = 0; transport < 2; ++transport)
                    {
                        if (route >= 2 && !fvm_jit_available())
                        {
                            continue;
                        }
                        FVM_Context *c = 0, *source = 0;
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
                        FVM_Value raw, value, out;
                        unsigned count = 0;
                        FVM_Object *array = array_deleted_fixture(c, kind, raw, count);
                        if (transport)
                        {
                            source = c;
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
                            value = class_value(array);
                            unsigned char *wire = 0;
                            size_t bytes = 0;
                            if (!(fvm_value_serialize(source, &value, &wire, &bytes) == FVM_OK &&
                                  fvm_value_deserialize(c, wire, bytes, &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            fvm_serialized_free(wire);
                            array = value.object;
                        }
                        if (!(fvm_gc_pin(c, array) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        out = array_deleted_call(c, array, methods[method], route);
                        if (!method)
                        {
                            if (!(out.type == FVM_VALUE_STRING && !std::strcmp(out.string,
                                                                               count == 2   ? "11,22"
                                                                               : count == 1 ? "11"
                                                                                            : "")))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else if (method <= 2)
                        {
                            if (!(out.type == FVM_VALUE_OBJECT &&
                                  fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == count))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else if (method <= 4)
                        {
                            if (!(out.type == FVM_VALUE_OBJECT && out.object == array))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else if (method <= 6)
                        {
                            if (!count)
                            {
                                if (!(out.type == FVM_VALUE_UNDEFINED))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            else
                            {
                                if (!(out.type == FVM_VALUE_NUMBER &&
                                      out.number == (method == 5 && count == 2 ? 22 : 11)))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                        else
                        {
                            if (!(out.type == FVM_VALUE_NUMBER && out.number == count + 1))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        const bool changed = method >= 7 || (method >= 5 && count);
                        if (changed)
                        {
                            if (!(fvm_object_get(c, array, "length", &value) == FVM_OK &&
                                  value.type == FVM_VALUE_NUMBER &&
                                  value.number == (method >= 7 ? count + 1 : count - 1)))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else if (kind <= 7 && kind != 1)
                        {
                            if (!(fvm_object_get(c, array, "length", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            boxing_equal(value, raw);
                        }
                        else if (kind != 10 || version >= 7)
                        {
                            int own = 1;
                            if (!(fvm_object_has_own(c, array, "length", &own) == FVM_OK && !own))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        /* Shrinking from ordinary String/Boolean data must not use stale numeric native tails. */
                        if ((kind == 2 || kind == 5) && (method == 5 || method == 6))
                        {
                            if (!(fvm_table_get_index(c, array, 1, &value) == FVM_OK && value.number == 22))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!(!c->exception_pending && !c->gc_frames && !c->gc_native_depth && !c->script_depth))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_context_destroy(c);
                        if (source)
                        {
                            fvm_context_destroy(source);
                        }
                    }
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
#endif
