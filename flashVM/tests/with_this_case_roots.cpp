/* With keyword reads/calls/Catch skip shadow getters; ordinary reads retain detached captures. */
#define FVM_WITH_THIS_CASE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "with_this_case.cpp"
#include "flashvm/exception.h"

static FVM_Object *case_owner, *case_capture;
static unsigned case_calls, case_failure, case_operation;


static FVM_Result case_getter(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++case_calls;
    if (!(frame->receiver == case_owner && c->script_this == case_owner))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int removed = 0;
    if (!(fvm_object_delete(c, case_owner, "THIS", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_delete(c, c->global, "owner", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set_prototype(c, case_owner, 0) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool alive = false;
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        alive |= c->objects[i] == case_capture;
    }
    if (!(alive))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (case_failure == 1)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    if (case_failure == 2)
    {
        return FVM_ERROR;
    }
    if (case_operation == 1 || case_operation == 3)
    {
        return fvm_object_get(c, case_capture, "returned", out);
    }
    fvm_value_number(out, 91);
    return FVM_OK;
}


static void case_word(Program &p, size_t n)
{
    p.op((unsigned char)n);
    p.op((unsigned char)(n >> 8));
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
    for (unsigned version = 5; version <= 8; ++version)
    {
        for (case_failure = 0; case_failure < 3; ++case_failure)
        {
            for (case_operation = 0; case_operation < 4; ++case_operation)
            {
                for (unsigned mode = 0; mode < 3; ++mode)
                {
                    if (mode && !fvm_jit_available())
                    {
                        continue;
                    }
                    FVM_Context *c = 0;
                    if (!(fvm_context_create(rt, &c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_load(c, version);
                    if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_construct(c, "Object", 0, 0, &case_owner) == FVM_OK &&
                          fvm_construct(c, "Object", 0, 0, &case_capture) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value value = class_value(case_owner), out;
                    if (!(fvm_set_global(c, "owner", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    Program returned;
                    if (case_operation == 3)
                    {
                        returned.variable("this");
                    }
                    else
                    {
                        returned.number(91);
                    }
                    returned.finish();
                    FVM_Object *callee = 0;
                    if (!(fvm_function_create_avm1(
                              c, returned.bytes.begin(), returned.bytes.size(), 0, 0, 0, &callee) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value = class_value(callee);
                    if (!(fvm_object_set(c, case_capture, "returned", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (case_operation == 3)
                    {
                        if (!(fvm_set_global(c, "cellFunction", &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    FVM_Object *getter = undefined_this_function(c, 0);
                    if (!(fvm_function_set_body(c, getter, case_getter, 0, case_capture) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value args[3];
                    fvm_value_string(args, "THIS");
                    args[1] = class_value(getter);
                    args[2] = class_value(undefined_this_function(c, 0));
                    if (!(fvm_object_call(c, case_owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    Program body;
                    if (case_operation == 1 || case_operation == 3)
                    {
                        body.number(0);
                        body.text("THIS");
                        body.op(0x3d);
                        body.op(0x3e);
                    }
                    else if (!case_operation)
                    {
                        body.variable("THIS");
                        body.op(0x3e);
                    }
                    else
                    {
                        Program raised, caught, header;
                        raised.number(77);
                        raised.op(0x2a);
                        caught.variable("THIS");
                        caught.op(0x3e);
                        header.op(1);
                        case_word(header, raised.bytes.size());
                        case_word(header, caught.bytes.size());
                        case_word(header, 0);
                        header.op('e');
                        header.op(0);
                        body.op(0x8f);
                        case_word(body, header.bytes.size());
                        for (size_t i = 0; i < header.bytes.size(); ++i)
                        {
                            body.op(header.bytes[i]);
                        }
                        for (size_t i = 0; i < raised.bytes.size(); ++i)
                        {
                            body.op(raised.bytes[i]);
                        }
                        for (size_t i = 0; i < caught.bytes.size(); ++i)
                        {
                            body.op(caught.bytes[i]);
                        }
                    }
                    Program outer;
                    outer.text("this");
                    if (case_operation == 3)
                    {
                        outer.variable("cellFunction");
                    }
                    else
                    {
                        outer.number(17);
                    }
                    outer.op(0x1d);
                    outer.variable("owner");
                    outer.op(0x94);
                    case_word(outer, 2);
                    case_word(outer, body.bytes.size());
                    for (size_t i = 0; i < body.bytes.size(); ++i)
                    {
                        outer.op(body.bytes[i]);
                    }
                    outer.number(99);
                    outer.finish();
                    FVM_Object *fn = 0;
                    if (!(fvm_function_create_avm1(c, outer.bytes.begin(), outer.bytes.size(), 0, 0, 0, &fn) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    case_calls = 0;
                    bool keyword = version <= 6;
                    FVM_Result status = keyword || !case_failure ? FVM_OK
                                        : case_failure == 1      ? FVM_EXCEPTION
                                                                 : FVM_ERROR;
                    value = with_case_invoke(c, fn, outer, mode, status);
                    if (!(case_calls == (keyword ? 0U : 1U)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (status == FVM_OK)
                    {
                        if (case_operation == 3)
                        {
                            if (!(value.type == FVM_VALUE_OBJECT && value.object == (keyword ? c->global : case_owner)))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else if (keyword && case_operation == 1)
                        {
                            if (!(value.type == FVM_VALUE_UNDEFINED))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(value.type == FVM_VALUE_NUMBER && value.number == (keyword ? 17 : 91)))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    if (!(!c->exception_pending && c->lexical_scopes.empty() && !c->script_this_cell))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
