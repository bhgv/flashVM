/* Native With shares operands, registers, pools and Return propagation with the interpreter. */
#define FVM_DEFINITIONS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_definitions.cpp"
#include "../src/standard/function_state.hpp"


static void with_block(Program &p, const Program &body, bool inlined = false)
{
    p.op(0x94);
    definition_word(p, 2 + (inlined ? body.bytes.size() : 0));
    definition_word(p, body.bytes.size());
    for (size_t i = 0; i < body.bytes.size(); ++i)
    {
        p.op(body.bytes[i]);
    }
}


static FVM_Object *with_owner(FVM_Context *c)
{
    FVM_Object *owner = 0;
    FVM_Value value;
    if (!(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 5);
    if (!(fvm_object_set(c, owner, "x", &value) == FVM_OK && fvm_object_set(c, owner, "captured", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = class_value(owner);
    if (!(fvm_set_global(c, "owner", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return owner;
}


static FVM_Value with_run(FVM_Context *c, Program p, FVM_Result expected = FVM_OK)
{
    size_t lexical = c->lexical_scopes.size();
    p.finish();
    FVM_Value out = class_run(c, p, expected);
    if (!(c->lexical_scopes.size() == lexical))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out;
}
#ifndef FVM_WITH_FIXTURE_ONLY


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
            if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            with_owner(c);
            for (unsigned inlined = 0; inlined < 2; ++inlined)
            {
                for (unsigned end = 0; end < 2; ++end)
                {
                    Program sum;
                    sum.variable("x");
                    sum.op(0x0a);
                    if (end)
                    {
                        sum.op(0);
                    }
                    Program p;
                    p.number(77);
                    p.variable("owner");
                    with_block(p, sum, inlined != 0);
                    FVM_Value out = with_run(c, p);
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == 82))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    Program returning;
                    returning.number(42);
                    returning.op(0x3e);
                    if (end)
                    {
                        returning.op(0);
                    }
                    Program early;
                    early.variable("owner");
                    with_block(early, returning, inlined != 0);
                    early.number(99);
                    out = with_run(c, early);
                    if (!(out.number == 42))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    Program inner;
                    inner.variable("owner");
                    with_block(inner, returning, inlined != 0);
                    inner.number(99);
                    inner.op(0x17);
                    Program nested;
                    nested.variable("owner");
                    with_block(nested, inner, inlined != 0);
                    nested.number(101);
                    out = with_run(c, nested);
                    if (!(out.number == 42))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    Program pool;
                    definition_pool(pool, "changed");
                    if (end)
                    {
                        pool.op(0);
                    }
                    Program pooled;
                    definition_pool(pooled, "original");
                    pooled.variable("owner");
                    with_block(pooled, pool, inlined != 0);
                    pooled.op(0x96);
                    definition_word(pooled, 2);
                    pooled.op(8);
                    pooled.op(0);
                    out = with_run(c, pooled);
                    if (!(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "changed")))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            Program empty;
            Program p;
            p.number(77);
            p.variable("owner");
            with_block(p, empty);
            if (!(with_run(c, p).number == 77))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program raising;
            raising.text("boom");
            raising.op(0x2a);
            for (unsigned type = 0; type < 2; ++type)
            {
                Program skip;
                skip.number(77);
                skip.op(0x96);
                definition_word(skip, 1);
                skip.op(type ? 3 : 2);
                with_block(skip, raising);
                if (!(with_run(c, skip).number == 77))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            Program missing;
            with_block(missing, empty);
            missing.number(77);
            with_run(c, missing, version ? FVM_OK : FVM_ERROR);
            Program throwing;
            throwing.variable("owner");
            with_block(throwing, raising);
            throwing.number(77);
            with_run(c, throwing, FVM_EXCEPTION);
            if (!(with_run(c, p).number == 77))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program reg;
            reg.number(42);
            reg.op(0x87);
            definition_word(reg, 1);
            reg.op(0);
            reg.op(0x17);
            Program bank;
            bank.variable("owner");
            with_block(bank, reg);
            definition_register(bank, 0);
            if (!(with_run(c, bank).number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program private_store;
            private_store.number(42);
            private_store.op(0x87);
            definition_word(private_store, 1);
            private_store.op(2);
            private_store.op(0x17);
            Program private_bank;
            private_bank.variable("owner");
            with_block(private_bank, private_store);
            definition_register(private_bank, 2);
            private_bank.finish();
            FVM_ScriptFunction private_definition;
            private_definition.function2 = true;
            private_definition.register_count = 3;
            private_definition.parameters.push_back(fvm_detail::string("x"));
            private_definition.parameter_registers.push_back(2);
            private_definition.body = private_bank.bytes;
            FVM_Object *private_fn = 0;
            if (!(fvm_function_make_script(c, private_definition, 0, &private_fn) == FVM_OK &&
                  fvm_gc_pin(c, private_fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(definition_invoke(c, private_fn, private_bank).number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program boxed;
            boxed.text("ran");
            boxed.number(17);
            boxed.op(0x1d);
            Program primitive;
            primitive.number(42);
            with_block(primitive, boxed);
            primitive.variable("ran");
            if (!(with_run(c, primitive).number == 17))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                FVM_Value absent;
                fvm_value_undefined(&absent);
                if (!(fvm_set_global(c, "Number", &absent) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(with_run(c, primitive).number == 17))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            Program value;
            value.variable("captured");
            value.finish();
            Program capture;
            definition_record(capture, "", false, false, value);
            capture.op(0x3e);
            Program closure;
            closure.variable("owner");
            with_block(closure, capture);
            closure.number(99);
            FVM_Value fn = with_run(c, closure), absent;
            if (!(fn.type == FVM_VALUE_OBJECT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&absent);
            if (!(fvm_set_global(c, "owner", &absent) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value out = definition_invoke(c, fn.object, value);
            if (out.type != FVM_VALUE_NUMBER || out.number != 5)
            {
                std::fprintf(stderr,
                             "closure version=%u mode=%u type=%d value=%g\n",
                             version,
                             class_mode,
                             (int)out.type,
                             out.number);
            }
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 5))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
