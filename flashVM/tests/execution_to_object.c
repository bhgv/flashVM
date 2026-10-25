/* C89 primitive boxing, loaded global overrides and alias-safe output. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/metamethod.h"

static unsigned hooks;


static FVM_Result
meta(FVM_Context *c, FVM_Object *receiver, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    (void)c;
    (void)receiver;
    (void)raw;
    if (!((n == 3 || n == 4) && args[0].type == FVM_VALUE_NUMBER && args[0].number == 42 &&
          args[2].type == FVM_VALUE_STRING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++hooks;
    fvm_value_number(out, 73);
    return FVM_OK;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned version;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (version = 0; version <= 8; ++version)
    {
        FVM_Context *c;
        FVM_Object *saved, *table;
        FVM_Value input, out, value, key;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (version && version < 4)
        {
            continue;
        }
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
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
        fvm_value_number(&input, 42);
        if (!(fvm_exec_to_object(c, &input, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, out.object, "valueOf", 0, 0, &value) == FVM_OK && value.type == FVM_VALUE_NUMBER &&
              value.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        saved = out.object;
        if (!(fvm_exec_to_object(c, &out, &out) == FVM_OK && out.object == saved))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string_bytes(&input, "a\0b", 3);
        if (!(fvm_exec_to_object(c, &input, &input) == FVM_OK && input.type == FVM_VALUE_OBJECT &&
              fvm_object_get(c, input.object, "length", &out) == FVM_OK && out.number == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_boolean(&input, 1);
        if (!(fvm_exec_to_object(c, &input, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT &&
              fvm_object_call(c, out.object, "valueOf", 0, 0, &value) == FVM_OK && value.type == FVM_VALUE_BOOLEAN &&
              value.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&input);
        input.type = FVM_VALUE_NULL;
        if (!(fvm_exec_to_object(c, &input, &input) == FVM_OK && input.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&input);
        if (!(fvm_exec_to_object(c, &input, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&out, 99);
        if (!(fvm_exec_to_object(c, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            fvm_value_number(&input, 42);
            fvm_value_undefined(&value);
            if (!(fvm_set_global(c, "Number", &value) == FVM_OK && fvm_exec_to_object(c, &input, &out) == FVM_OK &&
                  out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(&key, "missing");
            if (!(fvm_exec_member(c, 0, &input, &key, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED &&
                  fvm_exec_member(c, 1, &input, &key, &value, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&input);
            input.type = FVM_VALUE_NULL;
            if (!(fvm_exec_member(c, 1, &input, &key, &value, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&input, 42);
            if (!(fvm_object_create(c, &table) == FVM_OK && fvm_meta_register(c, table, "__index", meta, 0) == FVM_OK &&
                  fvm_meta_register(c, table, "__newindex", meta, 0) == FVM_OK &&
                  fvm_value_set_metatable(c, &input, table) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exec_member(c, 0, &input, &key, 0, &out) == FVM_OK && out.number == 73 &&
                  fvm_exec_member(c, 1, &input, &key, &value, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    if (!(hooks == 10))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
