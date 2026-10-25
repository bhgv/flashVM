/* Native fast sort bypasses loaded watchers and uses versioned primitive string keys. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/table.h"
#include "flashvm/gc.h"

static unsigned calls;


static FVM_Result watcher(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *args, FVM_Value *out, void *)
{
    if (!(n == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++calls;
    *out = args[2];
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
            unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
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
        FVM_Value values[3], args[2], out, value;
        FVM_Object *o = 0, *fn = 0;
        fvm_value_boolean(values, 1);
        fvm_value_boolean(values + 1, 0);
        fvm_value_string(values + 2, "0.5");
        if (!(fvm_construct(c, "Array", 3, values, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_native(c, watcher, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(args, "0");
        fvm_value_undefined(args + 1);
        args[1].type = FVM_VALUE_OBJECT;
        args[1].object = fn;
        if (!(fvm_object_call(c, o, "watch", 2, args, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        calls = 0;
        if (!(fvm_object_call(c, o, "sort", 0, 0, &out) == FVM_OK && calls == (version ? 0U : 1U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_table_get_index(c, o, 0, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version == 4)
        {
            if (!(value.type == FVM_VALUE_BOOLEAN && !value.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(value.type == FVM_VALUE_STRING))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_Object *rows[3] = {0, 0, 0};
        for (unsigned i = 0; i < 3; ++i)
        {
            if (!(fvm_construct(c, "Object", 0, 0, rows + i) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(c, rows[i]) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(values + i);
            values[i].type = FVM_VALUE_OBJECT;
            values[i].object = rows[i];
        }
        fvm_value_boolean(&value, 0);
        if (!(fvm_object_set(c, rows[0], "x", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&value, "0.5");
        if (!(fvm_object_set(c, rows[2], "x", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Array", 3, values, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(args, "x");
        if (!(fvm_object_call(c, o, "sortOn", 1, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_table_get_index(c, o, 0, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(value.object == (version && version < 7 ? rows[1] : rows[2])))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_table_get_index(c, o, 1, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(value.object == (version == 4 ? rows[0] : version && version < 7 ? rows[2] : rows[0])))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(values, 3);
        fvm_value_number(values + 1, 1);
        fvm_value_number(values + 2, 2);
        if (!(fvm_construct(c, "Array", 3, values, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(args, "missing");
        fvm_value_number(args + 1, 18);
        if (!(fvm_object_call(c, o, "sortOn", 2, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_table_get_index(c, o, 1, &value) == FVM_OK && value.number == (version ? 2 : 1)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
