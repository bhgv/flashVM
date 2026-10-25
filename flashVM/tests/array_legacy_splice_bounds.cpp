/* Loaded splice wraps start/delete counts to Int32 without repeating conversion. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"


static FVM_Object *source(FVM_Context *c)
{
    FVM_Value values[4];
    const char *items[] = {"A", "B", "C", "D"};
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_value_string(values + i, items[i]);
    }
    FVM_Object *o = 0;
    if (!(fvm_construct(c, "Array", 4, values, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return o;
}

static unsigned conversions;


static FVM_Result index(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++conversions;
    fvm_value_number(out, 4294967296.0);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value args[2], out, value;
        FVM_Object *o = source(c);
        fvm_value_number(args, 4294967296.0);
        fvm_value_string(args + 1, "4294967297");
        if (!(fvm_object_call(c, o, "splice", 2, args, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, out.object, "length", 1);
        number(c, o, "length", 3);
        if (!(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK && !std::strcmp(value.string, "A")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_table_get_index(c, o, 0, &value) == FVM_OK && !std::strcmp(value.string, "B")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        o = source(c);
        fvm_value_number(args, 4294967295.0);
        fvm_value_number(args + 1, 1);
        if (!(fvm_object_call(c, o, "splice", 2, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "length", 3);
        if (!(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK && !std::strcmp(value.string, "D")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        o = source(c);
        fvm_value_number(args, 0);
        fvm_value_number(args + 1, 4294967295.0);
        if (!(fvm_object_call(c, o, "splice", 2, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "length", 4);
        FVM_Object *endpoint = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &endpoint) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, endpoint, "valueOf", index, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        args[0] = object(endpoint);
        fvm_value_number(args + 1, 1);
        conversions = 0;
        if (!(fvm_object_call(c, o, "splice", 2, args, &out) == FVM_OK && conversions == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "length", 3);
        if (!(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK && !std::strcmp(value.string, "A")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
