/* Collecting/throwing With getters retain scope-only objects and active child mappings. */
#define FVM_WITH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_with.cpp"

static FVM_Object *scope_only;
static FVM_JitCode *explicit_code;
static unsigned getter_calls, collector_calls, failure;


static void with_active(FVM_Context *c)
{
    if (!class_mode)
    {
        return;
    }
    FVM_JitCode *code = explicit_code;
    if (class_mode == 2)
    {
        code = 0;
        for (fvm_detail::map<fvm_detail::string, FVM_JitCode *>::iterator i = c->execution_mode.cache.begin();
             i != c->execution_mode.cache.end();
             ++i)
        {
            if (i->second && i->second->active && !i->second->blocks.empty())
            {
                code = i->second;
            }
        }
    }
    if (!(code && code->active && code->blocks.size() == 1 && code->blocks[0].code->active &&
          fvm_jit_destroy(code) == FVM_INVALID_ARGUMENT &&
          fvm_jit_destroy(code->blocks[0].code) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Result with_get(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++getter_calls;
    if (!(frame->receiver == scope_only))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value absent;
    fvm_value_undefined(&absent);
    if (!(fvm_set_global(c, "owner", &absent) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    with_active(c);
    fvm_value_number(out, 17);
    return failure == 1 ? fvm_exception_raise(c, out) : FVM_OK;
}


static FVM_Result with_collect(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++collector_calls;
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    with_active(c);
    FVM_Value value;
    if (!(fvm_object_get(c, scope_only, "alive", &value) == FVM_OK && value.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 18);
    return failure == 2 ? fvm_exception_raise(c, out) : FVM_OK;
}


static FVM_Object *with_callback(FVM_Context *c, FVM_ScriptBody body)
{
    const unsigned char empty[] = {0};
    FVM_Object *fn = 0;
    if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &fn) == FVM_OK &&
          fvm_function_set_body(c, fn, body, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fn;
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
            for (failure = 0; failure < 3; ++failure)
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
                scope_only = with_owner(c);
                FVM_Value args[3], out;
                fvm_value_number(&out, 17);
                if (!(fvm_object_set(c, scope_only, "alive", &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_string(args, "get");
                args[1] = class_value(with_callback(c, with_get));
                fvm_value_undefined(args + 2);
                args[2].type = FVM_VALUE_NULL;
                if (!(fvm_object_call(c, scope_only, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                out = class_value(with_callback(c, with_collect));
                if (!(fvm_set_global(c, "collect", &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program inner;
                inner.variable("get");
                inner.op(0x17);
                inner.number(0);
                inner.text("collect");
                inner.op(0x3d);
                inner.op(0x17);
                inner.number(42);
                inner.op(0x3e);
                Program p;
                p.variable("owner");
                with_block(p, inner);
                p.number(99);
                p.finish();
                FVM_Object *fn = 0;
                explicit_code = 0;
                if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK &&
                      fvm_gc_pin(c, fn) == FVM_OK &&
                      fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &explicit_code) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_execution_set_mode(c, class_mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) ==
                      FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (class_mode == 1)
                {
                    if (!(fvm_function_set_body(c, fn, fvm_jit_body, explicit_code, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                getter_calls = collector_calls = 0;
                FVM_GCRootFrame *roots = c->gc_frames;
                if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == (failure ? FVM_EXCEPTION : FVM_OK)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(getter_calls == 1 && collector_calls == (failure == 1 ? 0U : 1U)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(c->gc_frames == roots && c->lexical_scopes.empty() && !explicit_code->active &&
                      !explicit_code->blocks[0].code->active && !c->script_depth && !c->avm1_execution_depth &&
                      !c->gc_native_depth && !c->execution_mode.native_pool))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (failure)
                {
                    if (!(fvm_exception_get(c, &out) == FVM_OK && out.number == (failure == 1 ? 17 : 18) &&
                          fvm_exception_clear(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == 42))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_execution_set_mode(c, FVM_EXECUTION_MANUAL, 0) == FVM_OK &&
                      fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(explicit_code) == FVM_OK &&
                      fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
