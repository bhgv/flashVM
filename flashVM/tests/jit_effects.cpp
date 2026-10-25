/* Time, random state, trace and thrown values agree across all three execution modes. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include <cmath>

static unsigned traces, conversions;
static bool raising;
static FVM_Result sink_status;


static FVM_Result
effect_sink(FVM_Context *c, FVM_Object *receiver, unsigned argc, const FVM_Value *argv, FVM_Value *, void *)
{
    ++traces;
    if (!(receiver == c->global && argc == 1 && argv[0].type == FVM_VALUE_STRING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string_size(argv) == 3 && !std::memcmp(argv[0].string, "a\0b", 3)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return sink_status;
}


static FVM_Result effect_convert(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    ++conversions;
    FVM_Value empty;
    fvm_value_undefined(&empty);
    if (!(fvm_set_global(c, "input", &empty) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value marker;
    if (!(fvm_object_get(c, frame->receiver, "alive", &marker) == FVM_OK && marker.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (raw)
    {
        if (!(fvm_value_string_bytes(out, "a\0b", 3) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        fvm_value_number(out, 10);
    }
    return raising ? fvm_exception_raise(c, out) : FVM_OK;
}


static void effect_object(FVM_Context *c, bool text)
{
    const unsigned char empty[] = {0};
    FVM_Object *object = 0, *fn = 0;
    FVM_Value value;
    if (!(fvm_object_create(c, &object) == FVM_OK))
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
    if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_set_body(c, fn, effect_convert, text ? (void *)1 : 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = class_value(fn);
    if (!(fvm_object_set(c, object, text ? "toString" : "valueOf", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = class_value(object);
    if (!(fvm_set_global(c, "input", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (class_mode = 0; class_mode < 3; ++class_mode)
        {
            if (version && version < 4)
            {
                continue;
            }
            FVM_Context *c = 0;
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
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
            Program random;
            random.number(10);
            random.op(0x30);
            random.finish();
            FVM_Value out = class_run(c, random);
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 8 && c->random_state == 1015568748UL))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program zero;
            zero.number(0);
            zero.op(0x30);
            zero.finish();
            out = class_run(c, zero);
            if (!(out.number == 0 && c->random_state == 1015568748UL))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            effect_object(c, false);
            Program bound;
            bound.variable("input");
            bound.op(0x30);
            bound.finish();
            conversions = 0;
            raising = false;
            out = class_run(c, bound);
            if (!(conversions == 1 && out.number == 7 && c->random_state == 1586005467UL))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            effect_object(c, false);
            conversions = 0;
            raising = true;
            class_run(c, bound, FVM_EXCEPTION);
            if (!(conversions == 1 && c->random_state == 1586005467UL))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            raising = false;
            Program time;
            time.op(0x34);
            time.finish();
            double previous = 0;
            for (unsigned i = 0; i < 3; ++i)
            {
                out = class_run(c, time);
                if (!(out.type == FVM_VALUE_NUMBER && out.number >= previous && out.number == std::floor(out.number)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                previous = out.number;
            }
            Program trace;
            trace.variable("input");
            trace.op(0x26);
            trace.number(42);
            trace.finish();
            for (unsigned which = 0; which < 4; ++which)
            {
                effect_object(c, true);
                conversions = traces = 0;
                raising = which == 3;
                sink_status = which == 2 ? FVM_ERROR : FVM_OK;
                if (!(fvm_avm1_set_trace_rooted(c, which ? effect_sink : 0, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                out = class_run(c, trace, raising ? FVM_EXCEPTION : sink_status);
                if (!(conversions == 1 && traces == ((which && !raising) ? 1U : 0U)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!which || which == 1)
                {
                    if (!(out.number == 42))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            raising = false;
            if (!(fvm_avm1_set_trace_rooted(c, 0, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program thrown;
            thrown.text("retained exception");
            thrown.op(0x2a);
            thrown.number(99);
            thrown.finish();
            /* This helper clears the exception only after checking status and native unwinding. */
            class_run(c, thrown, FVM_EXCEPTION);
            const unsigned char missing[] = {0x30, 0x26, 0x2a};
            for (unsigned i = 0; i < 3; ++i)
            {
                Program p;
                p.op(missing[i]);
                p.number(42);
                p.finish();
                class_run(c, p, version ? (i == 2 ? FVM_EXCEPTION : FVM_OK) : FVM_ERROR);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
