/* Missing class operands preserve boxing/prototype side effects and host stack strictness. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"

static unsigned getter_calls; /* Getter invocations observed by this fixture. */
static bool fail_super;


static FVM_Object *body_function(FVM_Context *c, FVM_ScriptBody body, FVM_Object *capture)
{
    FVM_Object *fn = 0;
    const unsigned char code[] = {0};
    if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK &&
          fvm_function_set_body(c, fn, body, 0, capture) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fn;
}


static void property(FVM_Context *c, FVM_Object *object, FVM_Object *getter)
{
    FVM_Value args[3], out;
    fvm_value_string(args, "prototype");
    fvm_value_undefined(args + 1);
    args[1].type = FVM_VALUE_OBJECT;
    args[1].object = getter;
    fvm_value_undefined(args + 2);
    args[2].type = FVM_VALUE_NULL;
    if (!(fvm_object_call(c, object, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}

static unsigned boxes;
static bool fail_missing_box;


static FVM_Result missing_box(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++boxes;
    if (!(frame->argc == 1 && frame->argv[0].number == 7 && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (fail_missing_box)
    {
        fvm_value_number(out, 88);
        return fvm_exception_raise(c, out);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result drop_super(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    (void)raw;
    int deleted = 0;
    ++getter_calls;
    if (!(fvm_object_delete(c, c->global, "parentClass", &deleted) == FVM_OK && deleted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_delete(c, frame->receiver, "prototype", &deleted) == FVM_OK && deleted &&
          fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (fail_super)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = frame->capture;
    return FVM_OK;
}


static void finish(Bytes &code)
{
    text(code, "result");
    code.push_back(0x4d);
    code.push_back(0x1d);
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
    for (unsigned profile = 0; profile < 6; ++profile)
    {
        FVM_Context *c = 0;
        unsigned version = profile + 3;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
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
        const unsigned ops[] = {0x2b, 0x54, 0x69};
        for (unsigned i = 0; i < 3; ++i)
        {
            for (unsigned one = 0; one < 2; ++one)
            {
                Bytes code;
                if (one)
                {
                    integer(code, 7);
                }
                code.push_back((unsigned char)ops[i]);
                if (i < 2)
                {
                    finish(code);
                }
                else
                {
                    text(code, "result");
                    integer(code, 123);
                    code.push_back(0x1d);
                }
                code.push_back(0);
                FVM_Result r = fvm_avm1_execute(c, code.begin(), code.size());
                if (!profile)
                {
                    if (!(r == FVM_ERROR))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    FVM_Value out;
                    if (!(r == FVM_OK && fvm_get_global(c, "result", &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (i == 0)
                    {
                        if (!(out.type == FVM_VALUE_NULL))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (i == 1)
                    {
                        if (!(out.type == FVM_VALUE_BOOLEAN && !out.boolean))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == 123))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
        }
        if (profile)
        {
            FVM_Value value;
            FVM_Object *fn = body_function(c, missing_box, 0);
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = fn;
            if (!(fvm_set_global(c, "Number", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned failing = 0; failing < 2; ++failing)
            {
                Bytes code;
                integer(code, 7);
                code.push_back(0x2b);
                finish(code);
                code.push_back(0);
                fail_missing_box = failing != 0;
                boxes = 0;
                FVM_GCRootFrame *roots = c->gc_frames;
                FVM_Result r = fvm_avm1_execute(c, code.begin(), code.size());
                if (!(c->gc_frames == roots && boxes == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (failing)
                {
                    if (!(r == FVM_EXCEPTION && fvm_exception_clear(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(r == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            for (unsigned failing = 0; failing < 2; ++failing)
            {
                FVM_Object *super = 0, *parent = 0;
                if (!(fvm_construct(c, "Object", 0, 0, &super) == FVM_OK && fvm_object_create(c, &parent) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fail_super = failing != 0;
                property(c, super, body_function(c, drop_super, parent));
                fvm_value_undefined(&value);
                value.type = FVM_VALUE_OBJECT;
                value.object = super;
                if (!(fvm_set_global(c, "parentClass", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Bytes code;
                text(code, "parentClass");
                code.push_back(0x1c);
                code.push_back(0x69);
                code.push_back(0);
                unsigned before = getter_calls;
                FVM_GCRootFrame *roots = c->gc_frames;
                FVM_Result r = fvm_avm1_execute(c, code.begin(), code.size());
                if (!(getter_calls == before + 1 && c->gc_frames == roots && !c->avm1_execution_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (failing)
                {
                    if (!(r == FVM_EXCEPTION && fvm_exception_clear(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(r == FVM_OK))
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
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
