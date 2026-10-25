/* Radix coercion cannot replace captured numeric data; native GC and exception policy stay intact. */
#define FVM_LOADED_RADIX_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "number_loaded_radix.cpp"
#include "flashvm/exception.h"

static FVM_Object *source;
static unsigned calls, phase;


static FVM_Result radix_change(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++calls;
    if (!(c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    source->resource_result.number = 7.5;
    if (phase == 1)
    {
        fvm_value_number(out, 81);
        return fvm_exception_raise(c, out);
    }
    if (phase == 2)
    {
        return FVM_ERROR;
    }
    fvm_value_number(out, 3);
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
        for (phase = 0; phase < 3; ++phase)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            radix_loaded(c, version);
            source = radix_box(c, 42.9);
            if (!(fvm_gc_pin(c, source) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *radix = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &radix) == FVM_OK &&
                  fvm_object_define_method_rooted(c, radix, "valueOf", radix_change, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value arg = object(radix), out;
            calls = 0;
            FVM_Result r = fvm_object_call(c, source, "toString", 1, &arg, &out);
            if (!(calls == 1 && source->resource_result.number == 7.5))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!phase)
            {
                if (!(r == FVM_OK && !std::strcmp(out.string, "1120")))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else if (phase == 1)
            {
                if (!(r == FVM_EXCEPTION && fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(r == FVM_ERROR))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(!c->gc_frames && !c->gc_native_depth && !c->primitive_depth && !c->string_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            radix_expect(c, 7.5, 3, "21");
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
