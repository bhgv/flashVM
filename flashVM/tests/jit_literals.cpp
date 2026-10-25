/* Native initializers retain versioned count rules, stack order, own Array slots and counted keys. */
#define main literal_reference_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "jit_references.cpp"

#undef main
#include "flashvm/property.h"
#include "flashvm/gc_roots.h"
#include "flashvm/exception.h"
#include <limits>


struct LiteralProperty
{
    const char *key;
    size_t size;
    FVM_ValueType type;
    double number;
    int own;
};


static void literal_properties(FVM_Context *c, FVM_Object *object, const LiteralProperty *props, unsigned count)
{
    for (unsigned i = 0; i < count; ++i)
    {
        FVM_Value value;
        int own = 0;
        if (!(fvm_object_get_bytes(c, object, props[i].key, props[i].size, &value) == FVM_OK &&
              value.type == props[i].type))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (value.type == FVM_VALUE_NUMBER)
        {
            if (!(value.number == props[i].number))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (props[i].own >= 0)
        {
            if (!(fvm_object_has_own_bytes(c, object, props[i].key, props[i].size, &own) == FVM_OK &&
                  own == props[i].own))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
}


static void literal_check(FVM_Context *c,
                          const Program &p,
                          const FVM_Value *arg,
                          FVM_Result expected,
                          FVM_ValueType type,
                          const LiteralProperty *props = 0,
                          unsigned count = 0)
{
    const char *params[] = {"input"};
    FVM_Object *fn = 0;
    FVM_JitCode *code = 0;
    FVM_Value out;
    FVM_GCRootScope *hold = 0;
    if (!(fvm_gc_roots_begin(c, &hold) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (arg)
    {
        if (!(fvm_gc_roots_add(hold, arg) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 1, params, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, fn) == FVM_OK && fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &code) == FVM_OK))
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
        if (!(fvm_function_set_body(c, fn, mode == 1 ? fvm_jit_body : 0, mode == 1 ? code : 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned long entries = c->execution_mode.stats.native_entries;
        FVM_GCRootFrame *roots = c->gc_frames;
        if (!(fvm_function_invoke(c, fn, 0, arg ? 1 : 0, arg, &out) == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->execution_mode.stats.native_entries == entries + (mode != 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->gc_frames == roots && !c->script_depth && !c->gc_native_depth && !code->active))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (expected == FVM_EXCEPTION)
        {
            if (!(fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (expected == FVM_OK)
        {
            if (!(out.type == type))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (type == FVM_VALUE_OBJECT)
            {
                if (!(fvm_gc_roots_add(hold, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                literal_properties(c, out.object, props, count);
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                literal_properties(c, out.object, props, count);
            }
            if (type == FVM_VALUE_NUMBER)
            {
                if (!(out.number == 17))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    if (!(fvm_execution_set_mode(c, FVM_EXECUTION_MANUAL, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (expected == FVM_OK && type == FVM_VALUE_OBJECT)
    {
        literal_properties(c, out.object, props, count);
    }
    if (!(fvm_gc_roots_end(hold) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#ifndef FVM_LITERAL_FIXTURE_ONLY


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
        const LiteralProperty array[] = {{"length", 6, FVM_VALUE_NUMBER, 2, -1},
                                         {"0", 1, FVM_VALUE_NUMBER, 42, 1},
                                         {"1", 1, FVM_VALUE_NUMBER, 17, 1}};
        Program a;
        a.number(17);
        a.number(42);
        a.number(2);
        a.op(0x42);
        a.finish();
        literal_check(c, a, 0, FVM_OK, FVM_VALUE_OBJECT, array, 3);
        const LiteralProperty duplicate[] = {{"same", 4, FVM_VALUE_NUMBER, 1, 1}};
        Program o;
        o.text("same");
        o.number(1);
        o.text("same");
        o.number(2);
        o.number(2);
        o.op(0x43);
        o.finish();
        literal_check(c, o, 0, FVM_OK, FVM_VALUE_OBJECT, duplicate, 1);
        const LiteralProperty sparse[] = {{"length", 6, FVM_VALUE_NUMBER, 2, -1},
                                          {"0", 1, FVM_VALUE_NUMBER, 7, 1},
                                          {"1", 1, FVM_VALUE_UNDEFINED, 0, 1}};
        Program short_array;
        short_array.number(7);
        short_array.number(2);
        short_array.op(0x42);
        short_array.finish();
        literal_check(c, short_array, 0, version ? FVM_OK : FVM_INVALID_ARGUMENT, FVM_VALUE_OBJECT, sparse, 3);
        const LiteralProperty empty[] = {{"", 0, version ? FVM_VALUE_UNDEFINED : FVM_VALUE_NUMBER, 7, version ? 0 : 1}};
        Program empty_key;
        empty_key.text("");
        empty_key.number(7);
        empty_key.number(1);
        empty_key.op(0x43);
        empty_key.finish();
        literal_check(c, empty_key, 0, FVM_OK, FVM_VALUE_OBJECT, empty, 1);
        const char *missing_name = version < 7 ? "" : "undefined";
        const LiteralProperty partial[] = {{missing_name,
                                            std::strlen(missing_name),
                                            version < 7 ? FVM_VALUE_UNDEFINED : FVM_VALUE_NUMBER,
                                            7,
                                            version < 7 ? 0 : 1}};
        Program partial_pair;
        partial_pair.number(7);
        partial_pair.number(1);
        partial_pair.op(0x43);
        partial_pair.finish();
        literal_check(c, partial_pair, 0, version ? FVM_OK : FVM_INVALID_ARGUMENT, FVM_VALUE_OBJECT, partial, 1);
        FVM_Value key;
        if (!(fvm_value_string_bytes(&key, "a\0b", 3) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const LiteralProperty counted[] = {{"a\0b", 3, FVM_VALUE_NUMBER, 7, 1}};
        Program counted_key;
        counted_key.variable("input");
        counted_key.number(7);
        counted_key.number(1);
        counted_key.op(0x43);
        counted_key.finish();
        literal_check(c, counted_key, &key, FVM_OK, FVM_VALUE_OBJECT, counted, 1);
        for (unsigned object = 0; object < 2; ++object)
        {
            unsigned char op = object ? 0x43 : 0x42;
            Program missing;
            missing.op(op);
            missing.finish();
            literal_check(c, missing, 0, version ? FVM_OK : FVM_ERROR, FVM_VALUE_OBJECT);
            Program rejected;
            rejected.number(17);
            rejected.number(0xffffffffU);
            rejected.op(op);
            rejected.op(0x17);
            rejected.finish();
            literal_check(c, rejected, 0, version ? FVM_OK : FVM_INVALID_ARGUMENT, FVM_VALUE_NUMBER);
            Program bound;
            bound.number(65537);
            bound.op(op);
            bound.finish();
            literal_check(c, bound, 0, version ? FVM_UNSUPPORTED : FVM_INVALID_ARGUMENT, FVM_VALUE_UNDEFINED);
            Program dynamic;
            dynamic.variable("input");
            dynamic.op(op);
            dynamic.finish();
            FVM_Value count;
            fvm_value_number(&count, std::numeric_limits<double>::quiet_NaN());
            literal_check(c, dynamic, &count, version ? FVM_OK : FVM_INVALID_ARGUMENT, FVM_VALUE_OBJECT);
            fvm_value_number(&count, std::numeric_limits<double>::infinity());
            literal_check(c, dynamic, &count, version ? FVM_OK : FVM_INVALID_ARGUMENT, FVM_VALUE_UNDEFINED);
            fvm_value_number(&count, 1.5);
            Program fractional;
            if (object)
            {
                fractional.text("value");
            }
            fractional.number(17);
            fractional.variable("input");
            fractional.op(op);
            fractional.finish();
            const LiteralProperty fraction[] = {{object ? "value" : "0", object ? 5U : 1U, FVM_VALUE_NUMBER, 17, 1}};
            literal_check(
                c, fractional, &count, version ? FVM_OK : FVM_INVALID_ARGUMENT, FVM_VALUE_OBJECT, fraction, 1);
            fvm_value_string(&count, "0");
            literal_check(c, dynamic, &count, version ? FVM_OK : FVM_ERROR, FVM_VALUE_OBJECT);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
