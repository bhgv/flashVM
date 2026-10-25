/* The original builtin prototype is Object text, real clips remain paths, and copies retain identity. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/transfer.h"
#include "flashvm/channel.h"
#include "flashvm/exception.h"
#include <cstring>

static bool failing;


static FVM_Result custom(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failing)
    {
        FVM_Value e;
        fvm_value_number(&e, 77);
        return fvm_exception_raise(c, &e);
    }
    return fvm_value_string(out, "custom");
}


static void text(FVM_Context *c, const FVM_Value &value, const char *expected)
{
    FVM_Value out;
    if (!(fvm_exec_to_string(c, &value, &out) == FVM_OK && out.type == FVM_VALUE_STRING &&
          !std::strcmp(out.string, expected)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
        FVM_Context *c = 0, *other = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value klass, proto, out;
        if (!(fvm_get_global(c, "MovieClip", &klass) == FVM_OK &&
              fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK && fvm_gc_pin(c, proto.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        text(c, proto, "[object Object]");
        if (!(fvm_object_call(c, proto.object, "toString", 0, 0, &out) == FVM_OK &&
              !std::strcmp(out.string, "[object Object]")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        text(c, proto, "[object Object]");
        FVM_Channel *channel = 0;
        if (!(fvm_channel_create(&channel) == FVM_OK && fvm_channel_send_value(c, channel, &proto, 0) == FVM_OK &&
              fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(other, out.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        text(other, out, "[object Object]");
        fvm_channel_destroy(channel);
        FVM_Object *clip = 0;
        if (!(fvm_construct(c, "MovieClip", 0, 0, &clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = clip;
        text(c, value, "_level0");
        const unsigned char code[] = {0};
        FVM_Object *fn = 0;
        if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK &&
              fvm_function_set_body(c, fn, custom, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = fn;
        if (!(fvm_object_set(c, proto.object, "toString", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        text(c, proto, "custom");
        failing = true;
        fvm_value_number(&out, 99);
        if (!(fvm_exec_to_string(c, &proto, &out) == FVM_EXCEPTION && out.number == 99 &&
              fvm_exception_clear(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        failing = false;
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
