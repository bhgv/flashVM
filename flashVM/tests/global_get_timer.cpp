/* Deterministic clocks cover global/Function calls, aliases, independent origins and graph transfer. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/serialization.h"
#include "flashvm/gc.h"
#include "flashvm/execution.h"
#include "../ports/monotonic_port.h"
#include <cmath>

static double sample;
static int available = 1;
static unsigned reads, coercions;


extern "C" int fvm_port_monotonic_ms(double *out)
{
    ++reads;
    if (!available)
    {
        return 0;
    }
    *out = sample;
    return 1;
}


static FVM_Result coercion(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    ++coercions;
    return FVM_ERROR;
}


static FVM_Value call(FVM_Context *c, FVM_Object *fn, FVM_Value *arg, FVM_Result result = FVM_OK)
{
    FVM_Value out;
    fvm_value_number(&out, 99);
    if (!((fn ? fvm_function_invoke(c, fn, 0, arg ? 1 : 0, arg, &out)
              : fvm_call(c, "getTimer", arg ? 1 : 0, arg, &out)) == result))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (result == FVM_OK)
    {
        if (!(out.type == FVM_VALUE_NUMBER))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out;
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
        sample = 1000.25;
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
            sample = 1010;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value fn, arg;
        if (!(fvm_get_global(c, "getTimer", &fn) == FVM_OK && fvm_gc_pin(c, fn.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_numeric_callback_id(fn.object->invocable.callback) == 163 &&
              fvm_numeric_callback_from_id(163) == fn.object->invocable.callback))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *o = 0;
        if (!(fvm_object_create(c, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, o, "valueOf", coercion, 0, 0) == FVM_OK &&
              fvm_object_define_method_rooted(c, o, "toString", coercion, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&arg);
        arg.type = FVM_VALUE_OBJECT;
        arg.object = o;
        for (unsigned route = 0; route < 2; ++route)
        {
            sample = 1234.99;
            if (!(call(c, route ? fn.object : 0, &arg).number == 234))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            sample = 1100;
            if (!(call(c, route ? fn.object : 0, 0).number == 234))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            sample = 1000.25 + 2147483648.99;
            if (!(call(c, route ? fn.object : 0, 0).number == 2147483648.0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            sample = 1000.25 + 4294967297.25;
            if (!(call(c, route ? fn.object : 0, 0).number == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            available = 0;
            call(c, route ? fn.object : 0, 0, FVM_ERROR);
            available = 1;
            sample = HUGE_VAL;
            call(c, route ? fn.object : 0, 0, FVM_ERROR);
            sample = -1;
            call(c, route ? fn.object : 0, 0, FVM_ERROR);
            sample = 1000.25 + 4294967298.25;
            if (!(call(c, route ? fn.object : 0, 0).number == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* Reset fixture clock state between routes, never production state. */
            c->timer_elapsed_ms = 0;
        }
        if (!(!coercions))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned before = reads;
        c->gc_collecting = true;
        call(c, 0, 0, FVM_INVALID_ARGUMENT);
        c->gc_collecting = false;
        c->gc_releasing = true;
        call(c, 0, 0, FVM_INVALID_ARGUMENT);
        c->gc_releasing = false;
        c->destroying = true;
        call(c, 0, 0, FVM_INVALID_ARGUMENT);
        c->destroying = false;
        if (!(reads == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* An imported Function reads its receiver context's clock after sender destruction. */
        unsigned char *bytes = 0;
        size_t size = 0;
        if (!(fvm_value_serialize(c, &fn, &bytes, &size) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        sample = 9000;
        FVM_Context *dest = 0;
        if (!(fvm_context_create(rt, &dest) == FVM_OK && fvm_standard_install(dest) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_deserialize(dest, bytes, size, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(bytes);
        if (!(fvm_gc_pin(dest, fn.object) == FVM_OK && fvm_gc_collect(dest, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sample = 9042.99;
        if (!(call(dest, fn.object, 0).number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(dest);
    }
    /* Installation before load must not bypass the runtime version gate. */
    {
        FVM_Context *early = 0;
        if (!(fvm_context_create(rt, &early) == FVM_OK && fvm_standard_install(early) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value fn;
        if (!(fvm_get_global(early, "getTimer", &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 3, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (!(fvm_load_memory(early, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned before = reads;
        call(early, 0, 0, FVM_UNSUPPORTED);
        call(early, fn.object, 0, FVM_UNSUPPORTED);
        if (!(reads == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(early);
    }
    /* Failure to acquire an origin recovers on the first valid sample. */
    available = 0;
    FVM_Context *c = 0;
    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(c, 0, 0, FVM_ERROR);
    available = 1;
    sample = 200;
    if (!(call(c, 0, 0).number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    sample = 205.99;
    if (!(call(c, 0, 0).number == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    /* Installation must not overwrite a pre-existing case-equivalent own property. */
    for (unsigned version = 5; version <= 8; ++version)
    {
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value v;
        fvm_value_number(&v, 77);
        if (!(fvm_set_global(c, "GETTIMER", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == (version < 7 ? FVM_INVALID_ARGUMENT : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "GETTIMER", &v) == FVM_OK && v.number == 77))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
