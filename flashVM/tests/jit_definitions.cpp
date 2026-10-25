/* Native function records retain code, captured pools and lexical scopes after parent release. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/execution_mode.h"
#include <cstdio>


static void definition_word(Program &p, size_t n)
{
    p.op((unsigned char)n);
    p.op((unsigned char)(n >> 8));
}


static void definition_name(Program &p, const char *name)
{
    do
    {
        p.op((unsigned char)*name);
    } while (*name++);
}


static void definition_record(Program &p,
                              const char *name,
                              bool second,
                              bool inlined,
                              const Program &body,
                              unsigned registers = 3,
                              unsigned flags = 0,
                              unsigned parameter = 2)
{
    Program header;
    definition_name(header, name);
    definition_word(header, 1);
    if (second)
    {
        header.op((unsigned char)registers);
        definition_word(header, flags);
        header.op((unsigned char)parameter);
    }
    definition_name(header, "x");
    definition_word(header, body.bytes.size());
    p.op(second ? 0x8e : 0x9b);
    definition_word(p, header.bytes.size() + (inlined ? body.bytes.size() : 0));
    for (size_t i = 0; i < header.bytes.size(); ++i)
    {
        p.op(header.bytes[i]);
    }
    for (size_t i = 0; i < body.bytes.size(); ++i)
    {
        p.op(body.bytes[i]);
    }
}


static void definition_pool(Program &p, const char *text)
{
    p.op(0x88);
    definition_word(p, std::strlen(text) + 3);
    definition_word(p, 1);
    definition_name(p, text);
}


static void definition_register(Program &p, unsigned index)
{
    p.op(0x96);
    definition_word(p, 2);
    p.op(4);
    p.op((unsigned char)index);
}


static FVM_Value definition_invoke(FVM_Context *c, FVM_Object *fn, Program body)
{
    if (body.bytes.empty() || body.bytes.back() != 0)
    {
        body.op(0);
    }
    FVM_JitCode *code = 0;
    FVM_Value arg, out;
    fvm_value_number(&arg, 88);
    if (!(fvm_jit_compile(body.bytes.begin(), body.bytes.size(), &code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_execution_set_mode(c, class_mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (class_mode == 1)
    {
        if (!(fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK))
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
    FVM_GCRootFrame *roots = c->gc_frames;
    unsigned long before = c->execution_mode.stats.native_entries;
    if (!(fvm_function_invoke(c, fn, 0, 1, &arg, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->execution_mode.stats.native_entries == before + (class_mode != 0) && c->gc_frames == roots &&
          !code->active && !c->script_depth && !c->gc_native_depth && !c->avm1_execution_depth &&
          !c->execution_mode.native_pool))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (out.type == FVM_VALUE_OBJECT)
    {
        if (!(fvm_gc_pin(c, out.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_execution_set_mode(c, FVM_EXECUTION_MANUAL, 0) == FVM_OK &&
          fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out;
}
#ifndef FVM_DEFINITIONS_FIXTURE_ONLY


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
            for (unsigned second = 0; second < 2; ++second)
            {
                for (unsigned inlined = 0; inlined < 2; ++inlined)
                {
                    for (unsigned named = 0; named < 2; ++named)
                    {
                        for (unsigned explicit_end = 0; explicit_end < 2; ++explicit_end)
                        {
                            Program body;
                            if (second)
                            {
                                definition_register(body, 2);
                            }
                            else
                            {
                                body.variable("x");
                            }
                            body.op(0x3e);
                            if (explicit_end)
                            {
                                body.op(0);
                            }
                            Program parent;
                            definition_record(parent, named ? "made" : "", second, inlined, body);
                            if (named)
                            {
                                parent.variable("made");
                            }
                            parent.finish();
                            FVM_Value fn = class_run(c, parent);
                            if (!(fn.type == FVM_VALUE_OBJECT))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Value out = definition_invoke(c, fn.object, body);
                            if (out.type != FVM_VALUE_NUMBER || out.number != 88)
                            {
                                std::fprintf(
                                    stderr,
                                    "version=%u mode=%u second=%u inline=%u named=%u end=%u type=%d number=%g\n",
                                    version,
                                    class_mode,
                                    second,
                                    inlined,
                                    named,
                                    explicit_end,
                                    (int)out.type,
                                    out.number);
                            }
                            if (!(out.type == FVM_VALUE_NUMBER && out.number == 88))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_gc_unpin(c, fn.object) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                }
            }
            for (unsigned second = 0; second < 2; ++second)
            {
                Program body;
                body.variable("captured");
                body.finish();
                Program parent;
                parent.text("captured");
                parent.number(42);
                parent.op(0x3c);
                definition_record(parent, "", second, false, body);
                parent.finish();
                FVM_Value fn = class_run(c, parent), out = definition_invoke(c, fn.object, body);
                if (!(out.type == FVM_VALUE_NUMBER && out.number == 42))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_unpin(c, fn.object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program pooled;
                pooled.op(0x96);
                definition_word(pooled, 2);
                pooled.op(8);
                pooled.op(0);
                pooled.finish();
                Program pool;
                definition_pool(pool, "snapshot");
                definition_record(pool, "", second, true, pooled);
                definition_pool(pool, "replacement");
                pool.finish();
                fn = class_run(c, pool);
                out = definition_invoke(c, fn.object, pooled);
                if (!(out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 8 &&
                      !std::memcmp(out.string, "snapshot", 8)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_unpin(c, fn.object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program grand;
                grand.number(42);
                grand.finish();
                Program child;
                definition_record(child, "", second, false, grand);
                child.finish();
                Program nested;
                definition_record(nested, "", second, true, child);
                nested.finish();
                fn = class_run(c, nested);
                out = definition_invoke(c, fn.object, child);
                if (!(out.type == FVM_VALUE_OBJECT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value result = definition_invoke(c, out.object, grand);
                if (!(result.number == 42))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_unpin(c, fn.object) == FVM_OK && fvm_gc_unpin(c, out.object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            Program global;
            definition_register(global, 1);
            global.finish();
            Program preload;
            definition_record(preload, "", true, false, global, 3, 256, 2);
            preload.finish();
            FVM_Value preloaded = class_run(c, preload);
            FVM_Value global_result = definition_invoke(c, preloaded.object, global);
            if (!(global_result.type == FVM_VALUE_OBJECT && global_result.object == c->global))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_unpin(c, preloaded.object) == FVM_OK && fvm_gc_unpin(c, global_result.object) == FVM_OK))
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
