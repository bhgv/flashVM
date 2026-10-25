/* sortOn field/option Array lists use their live own length, not cached native storage. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"


static FVM_Object *sort_live_list(FVM_Context *c, bool options, unsigned kind)
{
    FVM_Object *list = 0;
    if (!(fvm_construct(c, "Array", 0, 0, &list) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    if (options)
    {
        fvm_value_number(&value, 18);
    }
    else
    {
        fvm_value_string(&value, "key");
    }
    if (!(fvm_table_set_index(c, list, 0, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!kind)
    {
        return list;
    }
    if (!(fvm_object_set_property_flags(c, list, "length", 0, FVM_PROPERTY_VALID_FLAGS) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int removed = 0;
    if (!(fvm_object_delete(c, list, "length", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (kind == 1)
    {
        return list;
    }
    fvm_value_number(&value, 1);
    if (kind == 2)
    {
        fvm_value_string(&value, "1");
    }
    if (kind == 3)
    {
        fvm_value_number(&value, 0);
    }
    if (kind == 4)
    {
        fvm_value_number(&value, -1);
    }
    if (kind == 5)
    {
        fvm_value_boolean(&value, 1);
    }
    if (kind == 6)
    {
        value.type = FVM_VALUE_NULL;
    }
    if (kind == 7)
    {
        FVM_Object *proto = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK &&
              fvm_object_set_prototype(c, proto, list->prototype) == FVM_OK &&
              fvm_object_set_prototype(c, list, proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, proto, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(fvm_object_set(c, list, kind == 8 ? "LeNgTh" : "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return list;
}


static FVM_Object *sort_live_receiver(FVM_Context *c)
{
    FVM_Value values[3];
    const unsigned keys[] = {2, 1, 3};
    for (unsigned i = 0; i < 3; ++i)
    {
        FVM_Object *item = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &item) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_number(&value, keys[i]);
        if (!(fvm_object_set(c, item, "key", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        values[i] = class_value(item);
    }
    FVM_Object *array = 0;
    if (!(fvm_construct(c, "Array", 3, values, &array) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return array;
}


static FVM_Value sort_live_call(FVM_Context *c,
                                FVM_Object *receiver,
                                FVM_Object *fields,
                                FVM_Object *options,
                                unsigned route,
                                FVM_Result expected = FVM_OK)
{
    FVM_Value args[2] = {class_value(fields), class_value(options)}, value = class_value(receiver), out;
    if (!(fvm_set_global(c, "sortLiveReceiver", &value) == FVM_OK &&
          fvm_set_global(c, "sortLiveFields", args) == FVM_OK &&
          fvm_set_global(c, "sortLiveOptions", args + 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!route)
    {
        if (!(fvm_object_call(c, receiver, "sortOn", 2, args, &out) == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (expected == FVM_EXCEPTION)
        {
            if (!(fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        Program p;
        p.variable("sortLiveOptions");
        p.variable("sortLiveFields");
        p.number(2);
        p.variable("sortLiveReceiver");
        p.text("sortOn");
        p.op(0x52);
        p.finish();
        out = boxing_run(c, p, route - 1, expected);
    }
    return out;
}
#ifndef FVM_SORT_LIVE_LENGTH_FIXTURE_ONLY


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
        for (unsigned field_kind = 0; field_kind < 9; ++field_kind)
        {
            for (unsigned option_kind = 0; option_kind < 9; ++option_kind)
            {
                for (unsigned transport = 0; transport < 2; ++transport)
                {
                    for (unsigned route = 0; route < 4; ++route)
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
                        FVM_Object *receiver = sort_live_receiver(c), *fields = sort_live_list(c, false, field_kind),
                                   *options = sort_live_list(c, true, option_kind);
                        FVM_Value value;
                        if (transport)
                        {
                            FVM_Object *bundle = 0;
                            if (!(fvm_object_create(c, &bundle) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            value = class_value(receiver);
                            if (!(fvm_object_set(c, bundle, "receiver", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            value = class_value(fields);
                            if (!(fvm_object_set(c, bundle, "fields", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            value = class_value(options);
                            if (!(fvm_object_set(c, bundle, "options", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            value = class_value(bundle);
                            unsigned char *wire = 0;
                            size_t bytes = 0;
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
                            if (!(fvm_value_serialize(source, &value, &wire, &bytes) == FVM_OK &&
                                  fvm_value_deserialize(c, wire, bytes, &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            fvm_serialized_free(wire);
                            bundle = value.object;
                            if (!(fvm_object_get(c, bundle, "receiver", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            receiver = value.object;
                            if (!(fvm_object_get(c, bundle, "fields", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            fields = value.object;
                            if (!(fvm_object_get(c, bundle, "options", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            options = value.object;
                        }
                        FVM_Object *original[3];
                        for (unsigned i = 0; i < 3; ++i)
                        {
                            if (!(fvm_table_get_index(c, receiver, i, &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            original[i] = value.object;
                        }
                        value = sort_live_call(c, receiver, fields, options, route);
                        if (!(value.type == FVM_VALUE_OBJECT && value.object == receiver))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        const bool has_fields =
                            field_kind == 0 || field_kind == 2 || field_kind == 5 || (field_kind == 8 && version < 7);
                        const bool has_options = option_kind == 0 || option_kind == 2 || option_kind == 5 ||
                                                 (option_kind == 8 && version < 7);
                        const unsigned ascending[] = {1, 0, 2}, descending[] = {2, 0, 1};
                        for (unsigned i = 0; i < 3; ++i)
                        {
                            if (!(fvm_table_get_index(c, receiver, i, &value) == FVM_OK &&
                                  value.object == original[!has_fields   ? i
                                                           : has_options ? descending[i]
                                                                         : ascending[i]]))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!(!c->gc_frames && !c->gc_native_depth && !c->exception_pending && !c->script_depth))
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
