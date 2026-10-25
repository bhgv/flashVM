/* Raw IEEE literals and unary operations execute equivalently in manual and automatic JIT. */
#define main jit_reference_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "jit_references.cpp"

#undef main
#include "flashvm/exception.h"
#include <cmath>
#include <limits>


static void same_number(double actual, double expected)
{
    if (expected != expected)
    {
        if (!(actual != actual))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(actual == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (expected == 0)
        {
            if (!((1.0 / actual < 0) == (1.0 / expected < 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
}


static void compare(FVM_Context *c, const Program &p, const FVM_Value *arg, bool numeric = false, double expected = 0)
{
    const char *parameters[] = {"x"};
    FVM_Object *fn = 0;
    FVM_JitCode *native = 0;
    FVM_Value baseline, out;
    FVM_Result first = FVM_ERROR;
    fvm_detail::string text;
    if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 1, parameters, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, fn) == FVM_OK && fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &native) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < 3; ++mode)
    {
        if (!(fvm_execution_set_mode(c, mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, fn, mode == 1 ? fvm_jit_body : 0, mode == 1 ? native : 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned long entries = c->execution_mode.stats.native_entries;
        fvm_value_undefined(&out);
        FVM_Result status = fvm_function_invoke(c, fn, 0, arg ? 1 : 0, arg, &out);
        if (!(c->execution_mode.stats.native_entries == entries + (mode != 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!mode)
        {
            first = status;
            baseline = out;
            if (status == FVM_OK && out.type == FVM_VALUE_STRING)
            {
                text = fvm_detail::string(out.string, fvm_string_size(out));
            }
        }
        else
        {
            if (!(status == first))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (status == FVM_OK)
            {
                if (!(out.type == baseline.type))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (out.type == FVM_VALUE_NUMBER)
                {
                    same_number(out.number, baseline.number);
                }
                if (out.type == FVM_VALUE_BOOLEAN)
                {
                    if (!(out.boolean == baseline.boolean))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (out.type == FVM_VALUE_STRING)
                {
                    if (!(text == fvm_detail::string(out.string, fvm_string_size(out))))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        if (numeric)
        {
            if (!(status == FVM_OK && out.type == FVM_VALUE_NUMBER))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            same_number(out.number, expected);
        }
        if (status == FVM_EXCEPTION)
        {
            if (!(fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(fvm_execution_set_mode(c, FVM_EXECUTION_MANUAL, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(native) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void bits(Program &p, unsigned long value)
{
    for (unsigned i = 0; i < 4; ++i)
    {
        p.op((unsigned char)(value >> (i * 8)));
    }
}

static unsigned calls;
static bool throwing;


static FVM_Result coercion(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++calls;
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 42);
    return throwing ? fvm_exception_raise(c, out) : FVM_OK;
}


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    const unsigned char unary[] = {0x18, 0x4a, 0x4b, 0x44, 0x50, 0x51, 0x12};


    struct Literal
    {
        unsigned long high, low;
        double expected;
    };

    const Literal floats[] = {{0, 0, 0.0},
                              {0x80000000UL, 0, -0.0},
                              {0x3fc00000UL, 0, 1.5},
                              {1, 0, std::ldexp(1.0, -149)},
                              {0x00800000UL, 0, std::ldexp(1.0, -126)},
                              {0x007fffffUL, 0, std::ldexp(8388607.0, -149)},
                              {0x7f7fffffUL, 0, std::ldexp(16777215.0, 104)},
                              {0x7f800000UL, 0, std::numeric_limits<double>::infinity()},
                              {0xff800000UL, 0, -std::numeric_limits<double>::infinity()},
                              {0x7fc00001UL, 0, std::numeric_limits<double>::quiet_NaN()}};
    const Literal doubles[] = {{0, 0, 0.0},
                               {0x80000000UL, 0, -0.0},
                               {0x3ff80000UL, 0, 1.5},
                               {0, 1, std::ldexp(1.0, -1074)},
                               {0x00100000UL, 0, std::ldexp(1.0, -1022)},
                               {0x000fffffUL, 0xffffffffUL, std::ldexp(4503599627370495.0, -1074)},
                               {0x7fefffffUL, 0xffffffffUL, std::numeric_limits<double>::max()},
                               {0x7ff00000UL, 0, std::numeric_limits<double>::infinity()},
                               {0xfff00000UL, 0, -std::numeric_limits<double>::infinity()},
                               {0x7ff80000UL, 1, std::numeric_limits<double>::quiet_NaN()}};
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
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
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
        if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned format = 0; format < 2; ++format)
        {
            /* Keep the bounded automatic cache below capacity so each case
             * verifies native execution rather than the intentional full-cache fallback. */
            FVM_Context *literal_context = 0;
            if (!(fvm_context_create(rt, &literal_context) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                if (!(fvm_load_memory(literal_context, swf, sizeof(swf)) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_standard_install(literal_context) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (size_t i = 0; i < sizeof(floats) / sizeof(floats[0]); ++i)
            {
                const Literal &literal = format ? doubles[i] : floats[i];
                Program p;
                p.op(0x96);
                p.op(format ? 9 : 5);
                p.op(0);
                p.op(format ? 6 : 1);
                bits(p, literal.high);
                if (format)
                {
                    bits(p, literal.low);
                }
                p.finish();
                compare(literal_context, p, 0, true, literal.expected);
            }
            fvm_context_destroy(literal_context);
        }
        FVM_Value inputs[10];
        fvm_value_number(inputs, 42);
        fvm_value_number(inputs + 1, -1.75);
        fvm_value_number(inputs + 2, 4294967297.0);
        fvm_value_number(inputs + 3, std::numeric_limits<double>::quiet_NaN());
        fvm_value_string(inputs + 4, "");
        fvm_value_string(inputs + 5, "12.5");
        fvm_value_string(inputs + 6, "junk");
        fvm_value_undefined(inputs + 7);
        inputs[7].type = FVM_VALUE_NULL;
        fvm_value_undefined(inputs + 8);
        fvm_value_boolean(inputs + 9, 1);
        for (size_t i = 0; i < sizeof(unary); ++i)
        {
            Program p;
            p.variable("x");
            p.op(unary[i]);
            p.finish();
            for (size_t j = 0; j < sizeof(inputs) / sizeof(inputs[0]); ++j)
            {
                compare(c, p, inputs + j);
            }
            Program missing;
            missing.op(unary[i]);
            missing.finish();
            compare(c, missing, 0);
        }
        FVM_Object *object = 0, *fn = 0;
        const unsigned char empty[] = {0};
        FVM_Value value, arg;
        if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, fn, coercion, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = fn;
        if (!(fvm_object_set(c, object, "valueOf", &value) == FVM_OK &&
              fvm_object_set(c, object, "toString", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&arg);
        arg.type = FVM_VALUE_OBJECT;
        arg.object = object;
        for (unsigned fail = 0; fail < 2; ++fail)
        {
            for (size_t i = 0; i < sizeof(unary); ++i)
            {
                throwing = fail != 0;
                calls = 0;
                Program p;
                p.variable("x");
                p.op(unary[i]);
                p.finish();
                compare(c, p, &arg);
                if (!(calls == ((unary[i] == 0x44 || unary[i] == 0x12) ? 0 : 3)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
