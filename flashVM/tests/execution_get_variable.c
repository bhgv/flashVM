/* Complete C89 GetVariable converts loaded names and filters SWF4 display values only. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"

static unsigned conversions;
static int fail;


static FVM_Result name_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    FVM_Value value;
    (void)raw;
    ++conversions;
    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, frame->receiver, "alive", &value) == FVM_OK &&
          value.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (fail)
    {
        fvm_value_number(&value, 77);
        return fvm_exception_raise(c, &value);
    }
    return fvm_value_string_bytes(out, "name\0x", 6);
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
        FVM_Object *clip, *object, *fn, *target;
        FVM_Value key, value, out;
        const unsigned char code[] = {0};
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
        fvm_value_number(&value, 42);
        if (!(fvm_set_global(c, "17", &value) == FVM_OK && fvm_set_global_bytes(c, "name\0x", 6, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&key, 17);
        fvm_value_number(&out, 99);
        if (!(fvm_exec_get_variable(c, 0, 0, 0, 0, &key, &out) == (version ? FVM_OK : FVM_INVALID_ARGUMENT) &&
              out.number == (version ? 42 : 99)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string_bytes(&key, "name\0x", 6);
        if (!(fvm_exec_get_variable(c, 0, 0, 0, 0, &key, &key) == FVM_OK && key.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_create(c, "clip", 1, &clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = clip;
        if (!(fvm_set_global(c, "clip", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "clip");
        if (!(fvm_exec_get_variable(c, 0, 0, 0, 0, &key, &out) == FVM_OK &&
              out.type == (version == 4 ? FVM_VALUE_UNDEFINED : FVM_VALUE_OBJECT)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &object) == FVM_OK &&
              fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK &&
              fvm_function_set_body(c, fn, name_body, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 17);
        if (!(fvm_object_set(c, object, "alive", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = fn;
        if (!(fvm_object_set(c, object, "toString", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&key);
        key.type = FVM_VALUE_OBJECT;
        key.object = object;
        fvm_value_number(&out, 99);
        if (version)
        {
            if (!(fvm_gc_pin(c, object) == FVM_OK && fvm_object_create(c, &target) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 55);
            if (!(fvm_object_set_bytes(c, target, "name\0x", 6, &value) == FVM_OK &&
                  fvm_exec_get_variable(c, 0, target, 0, 0, &key, &out) == FVM_OK && out.number == 55))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fail = 1;
            fvm_value_number(&out, 99);
            if (!(fvm_exec_get_variable(c, 0, target, 0, 0, &key, &out) == FVM_EXCEPTION && out.number == 99 &&
                  fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fail = 0;
        }
        else
        {
            if (!(fvm_exec_get_variable(c, 0, 0, 0, 0, &key, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_string(&key, "missing");
        if (!(fvm_exec_get_variable(c, 0, 0, 0, 0, &key, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(conversions == 10))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
