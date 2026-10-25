/* Native CallMethod/NewMethod differential cases, argument ordering and collecting callbacks. */
#define main references_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "jit_references.cpp"

#undef main
#include "flashvm/exception.h"
#include <cstdio>

static unsigned callbacks;
static bool throwing;
static FVM_JitCode *active;


static FVM_Result called(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++callbacks;
    if (!(frame->argc == 2 && frame->argv[0].number == 17 && frame->argv[1].number == 25))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Result collected = fvm_gc_collect(c, 0);
    if (collected != FVM_OK)
    {
        return collected;
    }
    if (active)
    {
        if (!(fvm_jit_destroy(active) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_number(out, 42);
    return throwing ? fvm_exception_raise(c, out) : FVM_OK;
}


static void check(FVM_Context *c,
                  const Program &p,
                  FVM_Object *owner,
                  FVM_ValueType type,
                  double number,
                  FVM_Result expected = FVM_OK,
                  bool callback = false)
{
    const char *parameters[] = {"input"};
    FVM_Object *fn = 0;
    FVM_Value arg, out;
    if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 1, parameters, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_JitCode *code = 0;
    if (!(fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&arg);
    arg.type = FVM_VALUE_OBJECT;
    arg.object = owner;
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
        active = mode == 1 ? code : 0;
        callbacks = 0;
        unsigned long before = c->execution_mode.stats.native_entries;
        FVM_GCRootFrame *roots = c->gc_frames;
        FVM_Result status = fvm_function_invoke(c, fn, 0, 1, &arg, &out);
        if (status != expected)
        {
            std::fprintf(stderr,
                         "version=%u mode=%u op=%u status=%d expected=%d callback=%d\n",
                         c->swf_loaded ? c->swf_info.version : 0,
                         mode,
                         p.bytes[p.bytes.size() - 3],
                         status,
                         expected,
                         callback);
        }
        if (!(status == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->execution_mode.stats.native_entries == before + (mode != 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth && !code->active))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (callback)
        {
            if (!(callbacks == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
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
            if (type == FVM_VALUE_NUMBER)
            {
                if (!(out.number == number))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    active = 0;
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
}
#ifndef FVM_METHOD_FIXTURE_ONLY


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
        FVM_Object *owner = 0, *method = 0;
        FVM_Value value;
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
        if (!(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK && fvm_gc_pin(c, owner) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned char empty[] = {0};
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &method) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, method, called, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = method;
        if (!(fvm_object_set(c, owner, "run", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned construct = 0; construct < 2; ++construct)
        {
            Program p;
            p.number(25);
            p.number(17);
            p.number(2);
            p.variable("input");
            p.text("run");
            p.op(construct ? 0x53 : 0x52);
            p.finish();
            throwing = false;
            check(c, p, owner, construct ? FVM_VALUE_OBJECT : FVM_VALUE_NUMBER, 42, FVM_OK, true);
            throwing = true;
            check(c, p, owner, FVM_VALUE_UNDEFINED, 0, FVM_EXCEPTION, true);
            throwing = false;
            Program nullish;
            nullish.number(0);
            nullish.op(0x96);
            nullish.op(1);
            nullish.op(0);
            nullish.op(2);
            nullish.text("run");
            nullish.op(construct ? 0x53 : 0x52);
            nullish.finish();
            check(c, nullish, owner, FVM_VALUE_UNDEFINED, 0, construct && !version ? FVM_INVALID_ARGUMENT : FVM_OK);
        }
        /* A nullish owner discards a coerced count and both supplied arguments. */
        Program discard;
        discard.number(25);
        discard.number(17);
        discard.text("2");
        discard.op(0x96);
        discard.op(1);
        discard.op(0);
        discard.op(3);
        discard.text("run");
        discard.op(0x52);
        discard.finish();
        check(c, discard, owner, FVM_VALUE_UNDEFINED, 0, version ? FVM_OK : FVM_ERROR);
        Program primitive;
        primitive.number(1);
        primitive.number(1);
        primitive.text("abc");
        primitive.text("charCodeAt");
        primitive.op(0x52);
        primitive.finish();
        check(c, primitive, owner, FVM_VALUE_NUMBER, 98);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}

#endif
