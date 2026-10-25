/* Loaded sortOn accepts more than 64 fields and compares the last deciding field. */
#define FVM_SORT_LIVE_LENGTH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_sort_on_live_lengths.cpp"


static FVM_Object *many_fields_list(FVM_Context *c, unsigned count, bool options)
{
    FVM_Object *array = 0;
    if (!(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < count; ++i)
    {
        FVM_Value value;
        if (options)
        {
            fvm_value_number(&value, i + 1 == count ? 18 : 16);
        }
        else
        {
            fvm_value_string(&value, i + 1 == count ? "key" : "same");
        }
        if (!(fvm_table_set_index(c, array, i, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return array;
}


static FVM_Object *many_fields_receiver(FVM_Context *c)
{
    FVM_Object *receiver = sort_live_receiver(c);
    if (!(fvm_gc_pin(c, receiver) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value, zero;
    fvm_value_number(&zero, 0);
    for (unsigned i = 0; i < 3; ++i)
    {
        if (!(fvm_table_get_index(c, receiver, i, &value) == FVM_OK &&
              fvm_object_set(c, value.object, "same", &zero) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return receiver;
}


static FVM_Value many_fields_call(FVM_Context *c,
                                  FVM_Object *receiver,
                                  FVM_Object *fields,
                                  const FVM_Value &options,
                                  unsigned route,
                                  FVM_Result expected = FVM_OK)
{
    FVM_Value args[2] = {class_value(fields), options}, out, value = class_value(receiver);
    if (!(fvm_set_global(c, "manyFieldReceiver", &value) == FVM_OK &&
          fvm_set_global(c, "manyFieldFields", args) == FVM_OK &&
          fvm_set_global(c, "manyFieldOptions", args + 1) == FVM_OK))
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
        p.variable("manyFieldOptions");
        p.variable("manyFieldFields");
        p.number(2);
        p.variable("manyFieldReceiver");
        p.text("sortOn");
        p.op(0x52);
        p.finish();
        out = boxing_run(c, p, route - 1, expected);
    }
    return out;
}
#ifndef FVM_MANY_FIELDS_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned counts[] = {1, 64, 65, 66, 128, 257};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned size = 0; size < 6; ++size)
        {
            for (unsigned options_array = 0; options_array < 2; ++options_array)
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
                        FVM_Object *receiver = many_fields_receiver(c),
                                   *fields = many_fields_list(c, counts[size], false);
                        FVM_Value options, value;
                        if (options_array)
                        {
                            options = class_value(many_fields_list(c, counts[size], true));
                        }
                        else
                        {
                            fvm_value_number(&options, 16);
                        }
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
                            if (!(fvm_object_set(c, bundle, "fields", &value) == FVM_OK &&
                                  fvm_object_set(c, bundle, "options", &options) == FVM_OK))
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
                            if (!(fvm_object_get(c, bundle, "options", &options) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        value = many_fields_call(c, receiver, fields, options, route);
                        if (!(value.type == FVM_VALUE_OBJECT && value.object == receiver))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        const unsigned ascending[] = {1, 2, 3}, descending[] = {3, 2, 1};
                        for (unsigned i = 0; i < 3; ++i)
                        {
                            FVM_Value item;
                            if (!(fvm_table_get_index(c, receiver, i, &item) == FVM_OK &&
                                  fvm_object_get(c, item.object, "key", &value) == FVM_OK &&
                                  value.number == (options_array ? descending[i] : ascending[i])))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending))
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
