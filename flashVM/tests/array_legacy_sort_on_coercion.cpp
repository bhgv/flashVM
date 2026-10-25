#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"

static unsigned ids[] = {0, 1, 2, 3}, converted, fail_id;
static FVM_Object *options_array, *last_option;


static FVM_Result convert(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *data)
{
    unsigned id = *(unsigned *)data;
    if (!(id == converted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++converted;
    if (!id)
    {
        FVM_Value changed = object(last_option);
        if (!(fvm_table_set_index(c, options_array, 1, &changed) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (id == fail_id)
    {
        return FVM_ERROR;
    }
    if (id < 2)
    {
        return fvm_value_string(out, "rank");
    }
    fvm_value_number(out, 16);
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
        FVM_Value coercers[4], rows[2], value, args[2], out;
        FVM_Object *fields = 0;
        for (unsigned i = 0; i < 4; ++i)
        {
            FVM_Object *obj = 0, *fn = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &obj) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_create_native(c, convert, ids + i, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = object(fn);
            if (!(fvm_object_set(c, obj, i < 2 ? "toString" : "valueOf", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            coercers[i] = object(obj);
        }
        last_option = coercers[3].object;
        if (!(fvm_construct(c, "Array", 2, coercers, &fields) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Array", 2, coercers + 2, &options_array) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 2; ++i)
        {
            FVM_Object *row = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &row) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, i ? 2 : 10);
            if (!(fvm_object_set(c, row, "rank", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            rows[i] = object(row);
        }
        for (unsigned mode = 0; mode < 3; ++mode)
        {
            FVM_Object *array = 0;
            if (!(fvm_construct(c, "Array", 2, rows, &array) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 0);
            if (!(fvm_table_set_index(c, options_array, 1, &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            args[0] = object(fields);
            args[1] = object(options_array);
            converted = 0;
            fail_id = mode == 1 ? 1 : mode == 2 ? 3 : 4;
            if (!(fvm_object_call(c, array, "sortOn", 2, args, &out) == (mode ? FVM_ERROR : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(converted == (mode == 1 ? 2U : 4U)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_table_get_index(c, array, 0, &value) == FVM_OK && value.object == rows[mode ? 0 : 1].object))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_table_get_index(c, options_array, 1, &value) == FVM_OK && value.object == last_option))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->gc_frames == 0 && c->avm1_execution_depth == 0 && c->calling_setter_owner == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
