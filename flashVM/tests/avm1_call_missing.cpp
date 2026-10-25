/* Missing fixed call operands preserve top-relative roles and zero-argument invocation. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"

static unsigned invoked;
static bool throw_call;


static FVM_Result zero_call(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(frame->argc == 0 && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++invoked;
    fvm_value_number(out, 42);
    if (throw_call)
    {
        return fvm_exception_raise(c, out);
    }
    return FVM_OK;
}


static FVM_Result forbidden_name(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *, void *)
{
    if (!(false))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_ERROR;
}


static void global_object(FVM_Context *c, const char *key, FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = o;
    if (!(fvm_set_global(c, key, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void fetch(Bytes &b, const char *key)
{
    text(b, key);
    b.push_back(0x1c);
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
    const unsigned char ops[] = {0x3d, 0x40, 0x52, 0x53};
    for (unsigned profile = 0; profile < 6; ++profile)
    {
        FVM_Context *c = 0;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)(profile + 3);
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
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
        const unsigned char empty[] = {0};
        FVM_Object *fn = 0, *name_obj = 0;
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &fn) == FVM_OK &&
              fvm_function_set_body(c, fn, zero_call, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        global_object(c, "callee", fn);
        if (!(fvm_object_create(c, &name_obj) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        attach(c, name_obj, "toString", forbidden_name, 0);
        global_object(c, "nameObject", name_obj);
        for (unsigned op = 0; op < 4; ++op)
        {
            for (unsigned provided = 0; provided < (op < 2 ? 2 : 3); ++provided)
            {
                Bytes b;
                if (op < 2)
                {
                    if (provided)
                    {
                        text(b, "callee");
                    }
                }
                else if (provided == 1)
                {
                    fetch(b, "nameObject");
                }
                else if (provided == 2)
                {
                    fetch(b, "callee");
                    b.push_back(0x96);
                    u16(b, 1);
                    b.push_back(3);
                }
                b.push_back(ops[op]);
                text(b, "result");
                b.push_back(0x4d);
                b.push_back(0x1d);
                b.push_back(0);
                invoked = 0;
                throw_call = false;
                FVM_GCRootFrame *roots = c->gc_frames;
                FVM_Result r = fvm_avm1_execute(c, b.begin(), b.size());
                if (!(c->gc_frames == roots && !c->avm1_execution_depth && !c->script_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!profile)
                {
                    if (!(r == FVM_ERROR && !invoked))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    continue;
                }
                if (!(r == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value out;
                if (!(fvm_get_global(c, "result", &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                bool called = op < 2 ? provided != 0 : provided == 2;
                if (!(invoked == (called ? 1u : 0u)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!called)
                {
                    if (!(out.type == FVM_VALUE_UNDEFINED))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (op == 0 || op == 2)
                {
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == 42))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(out.type == FVM_VALUE_OBJECT))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (called)
                {
                    throw_call = true;
                    invoked = 0;
                    r = fvm_avm1_execute(c, b.begin(), b.size());
                    if (!(r == FVM_EXCEPTION && invoked == 1 && c->gc_frames == roots && !c->avm1_execution_depth &&
                          !c->script_depth))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_exception_clear(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    throw_call = false;
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
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
