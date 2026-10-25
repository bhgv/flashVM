/* Frame coercion collects through toString only, preserves aliases and restores roots on throws. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"

static unsigned strings, numbers;
static bool fail_request;


static FVM_Result string_request(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++strings;
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value alive;
    if (!(fvm_object_get(c, frame->receiver, "alive", &alive) == FVM_OK && alive.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (fail_request)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    return fvm_value_string(out, "002");
}


static FVM_Result number_request(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++numbers;
    fvm_value_number(out, 99);
    return FVM_OK;
}


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
        FVM_Context *c = 0;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned throwing = 0; throwing < 2; ++throwing)
        {
            FVM_Object *obj = 0;
            if (!(fvm_object_create(c, &obj) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value value;
            fvm_value_number(&value, 17);
            if (!(fvm_object_set(c, obj, "alive", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            attach(c, obj, "toString", string_request, 0);
            attach(c, obj, "valueOf", number_request, 0);
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = obj;
            fail_request = throwing != 0;
            strings = numbers = 0;
            FVM_GCRootFrame *roots = c->gc_frames;
            FVM_Result r = fvm_exec_frame_request(c, &value, 1, 3, &value);
            if (!(c->gc_frames == roots && strings == 1 && !numbers))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (throwing)
            {
                if (!(r == FVM_EXCEPTION && value.type == FVM_VALUE_OBJECT && value.object == obj))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(r == FVM_OK && value.type == FVM_VALUE_NUMBER && value.number == 5))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
