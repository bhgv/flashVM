/* Native Try/Catch/Finally shares state, restores host bindings and propagates overriding exits. */
#define FVM_WITH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_with.cpp"


static void try_block(Program &p,
                      const Program &body,
                      const Program &caught,
                      const Program &final,
                      unsigned flags = 3,
                      const char *name = "e",
                      unsigned reg = 0)
{
    Program header;
    header.op((unsigned char)flags);
    definition_word(header, body.bytes.size());
    definition_word(header, caught.bytes.size());
    definition_word(header, final.bytes.size());
    if (flags & 4)
    {
        header.op((unsigned char)reg);
    }
    else
    {
        definition_name(header, name);
    }
    p.op(0x8f);
    definition_word(p, header.bytes.size());
    for (size_t i = 0; i < header.bytes.size(); ++i)
    {
        p.op(header.bytes[i]);
    }
    const Program *blocks[] = {&body, &caught, &final};
    for (unsigned j = 0; j < 3; ++j)
    {
        for (size_t i = 0; i < blocks[j]->bytes.size(); ++i)
        {
            p.op(blocks[j]->bytes[i]);
        }
    }
}


static Program try_exit(unsigned value, bool throwing = false, bool ended = false)
{
    Program p;
    p.number(value);
    p.op(throwing ? 0x2a : 0x3e);
    if (ended)
    {
        p.op(0);
    }
    return p;
}
#ifndef FVM_TRY_FIXTURE_ONLY


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
            Program empty, normal;
            normal.number(17);
            normal.op(0x17);
            for (unsigned end = 0; end < 2; ++end)
            {
                Program raised = try_exit(42, true, end != 0), caught;
                caught.variable("e");
                caught.op(0x3e);
                if (end)
                {
                    caught.op(0);
                }
                Program p;
                try_block(p, raised, caught, normal);
                p.number(99);
                if (!(with_run(c, p).number == 42))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program returning = try_exit(11, false, end != 0), overriding = try_exit(22, false, end != 0),
                        rethrow = try_exit(33, true, end != 0);
                Program keep;
                try_block(keep, returning, empty, normal, 2);
                keep.number(99);
                if (!(with_run(c, keep).number == 11))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program replace;
                try_block(replace, returning, empty, overriding, 2);
                replace.number(99);
                if (!(with_run(c, replace).number == 22))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program suppress;
                try_block(suppress, raised, empty, overriding, 2);
                suppress.number(99);
                if (!(with_run(c, suppress).number == 22))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program fail;
                try_block(fail, returning, empty, rethrow, 2);
                fail.number(99);
                with_run(c, fail, FVM_EXCEPTION);
                Program catch_throw;
                try_block(catch_throw, raised, rethrow, normal);
                catch_throw.number(99);
                with_run(c, catch_throw, FVM_EXCEPTION);
                Program final_wins;
                try_block(final_wins, raised, rethrow, overriding);
                final_wins.number(99);
                if (!(with_run(c, final_wins).number == 22))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program reg;
                definition_register(reg, 0);
                reg.op(0x3e);
                Program register_catch;
                try_block(register_catch, raised, reg, normal, 7, "", 0);
                register_catch.number(99);
                if (!(with_run(c, register_catch).number == 42))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            Program raised = try_exit(42, true), restore;
            restore.op(0x17);
            restore.number(42);
            restore.op(0x2a);
            Program baseline;
            baseline.number(77);
            try_block(baseline, restore, empty, empty, 1);
            if (!(with_run(c, baseline).number == 77))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program binding;
            binding.text("e");
            binding.number(17);
            binding.op(0x3c);
            try_block(binding, raised, empty, empty, 1);
            binding.variable("e");
            if (!(with_run(c, binding).number == (version ? 42 : 17)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program regbody;
            definition_register(regbody, 2);
            regbody.op(0x3e);
            Program bank;
            try_block(bank, raised, regbody, empty, 5, "", 2);
            bank.number(99);
            bank.finish();
            FVM_ScriptFunction definition;
            definition.function2 = true;
            definition.register_count = 3;
            definition.body = bank.bytes;
            FVM_Object *fn = 0;
            if (!(fvm_function_make_script(c, definition, 0, &fn) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(definition_invoke(c, fn, bank).number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program invalid_register;
            try_block(invalid_register, raised, try_exit(99), empty, 5, "", 255);
            invalid_register.number(77);
            FVM_Value invalid = with_run(c, invalid_register, version ? FVM_OK : FVM_INVALID_ARGUMENT);
            if (version)
            {
                if (!(invalid.number == 99))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            Program body_pool;
            definition_pool(body_pool, "body");
            body_pool.number(1);
            body_pool.op(0x2a);
            Program catch_pool;
            definition_pool(catch_pool, "catch");
            Program final_pool;
            definition_pool(final_pool, "finally");
            Program pooled;
            try_block(pooled, body_pool, catch_pool, final_pool);
            pooled.op(0x96);
            definition_word(pooled, 2);
            pooled.op(8);
            pooled.op(0);
            FVM_Value out = with_run(c, pooled);
            if (!(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "finally")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value counted;
            if (!(fvm_value_string_bytes(&counted, "a\0b", 3) == FVM_OK &&
                  fvm_set_global(c, "text", &counted) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program string_throw;
            string_throw.variable("text");
            string_throw.op(0x2a);
            Program string_catch;
            definition_register(string_catch, 0);
            string_catch.op(0x3e);
            Program strings;
            try_block(strings, string_throw, string_catch, normal, 7, "", 0);
            strings.number(99);
            out = with_run(c, strings);
            if (!(out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 3 &&
                  !std::memcmp(out.string, "a\0b", 3)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program nested;
            try_block(nested, raised, try_exit(31), try_exit(32));
            Program scoped;
            scoped.variable("owner");
            with_block(scoped, nested);
            scoped.number(99);
            if (!(with_run(c, scoped).number == 32))
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
