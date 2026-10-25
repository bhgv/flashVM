/* Folded Array length watchers observe raw resource length before resize. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cstring>

static unsigned length_watch_calls;


static FVM_Result length_watch(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++length_watch_calls;
    if (!(frame->argc == 4 && fvm_value_string_size(frame->argv) == 6 &&
          !std::memcmp(frame->argv[0].string, "LENGTH", 6)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame->argv[1].type == FVM_VALUE_NUMBER && frame->argv[1].number == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame->argv[2].number == 5 && frame->argv[3].number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 4);
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
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
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
        FVM_Object *array = 0, *fn = 0;
        FVM_Value value, args[3], out;
        fvm_value_number(&value, 3);
        if (!(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned char end[] = {0};
        if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &fn) == FVM_OK &&
              fvm_function_set_body(c, fn, length_watch, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(args, "length");
        fvm_value_undefined(args + 1);
        args[1].type = FVM_VALUE_OBJECT;
        args[1].object = fn;
        fvm_value_number(args + 2, 42);
        if (!(fvm_object_call(c, array, "watch", 3, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Host rejects native length watchers; loaded contexts accept them. */
        if (!(!!out.boolean == (version != 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        length_watch_calls = 0;
        fvm_value_number(&value, 5);
        if (!(fvm_object_set(c, array, "LENGTH", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const bool folded = version && version <= 6;
        if (!(length_watch_calls == (folded ? 1U : 0U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, array, "length", &out) == FVM_OK && out.number == (folded ? 4 : 3)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, array, "LENGTH", &out) == FVM_OK && out.number == (folded ? 4 : 5)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 7);
        if (!(fvm_object_set_bytes(c, array, "LENGTH\0tail", 11, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(length_watch_calls == (folded ? 1U : 0U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, array, "length", &out) == FVM_OK && out.number == (folded ? 4 : 3)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get_bytes(c, array, "LENGTH\0tail", 11, &out) == FVM_OK && out.number == 7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->watch_running.empty() && !c->gc_frames && !c->gc_native_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
