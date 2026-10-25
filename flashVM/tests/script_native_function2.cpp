#include <stdio.h>
#include <stdlib.h>
#include "../src/standard/function_state.hpp"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/table.h"


struct Probe
{
    unsigned flags, calls;
    FVM_Object *receiver;
};


static FVM_Result native(FVM_Context *c, const FVM_ScriptFrame *f, FVM_Value *out, void *raw)
{
    Probe *p = (Probe *)raw;
    if (!(f->receiver == p->receiver && f->register_count == 8 && f->argc == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->script_this == f->implicit_this && c->script_depth == 1 && c->script_callee == f->function))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->lexical_scopes.back() == f->activation && c->avm1_target_active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->avm1_execution_depth == 1 && !c->pending_finalizer && p->receiver->finalized))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(f->registers[7].number == 19))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(f->registers[1].type == ((p->flags & 2) ? FVM_VALUE_UNDEFINED : FVM_VALUE_OBJECT)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(f->registers[2].type == FVM_VALUE_OBJECT && f->registers[2].object == f->arguments))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (p->flags & 32)
    {
        if (!(f->super_value.type == FVM_VALUE_UNDEFINED && f->registers[3].type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(f->registers[5].type == FVM_VALUE_OBJECT && f->registers[5].object == c->global))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    if (!(fvm_table_get_index(c, f->arguments, 0, &value) == FVM_OK && value.number == 19))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *temporary = 0;
    if (!(fvm_object_create(c, &temporary) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&f->registers[7]);
    f->registers[7].type = FVM_VALUE_OBJECT;
    f->registers[7].object = temporary;
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 41);
    if (!(fvm_object_set(c, temporary, "alive", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++p->calls;
    *out = value;
    return FVM_OK;
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
    const unsigned paired[] = {0, 2, 8, 32, 42};
    for (unsigned version = 5; version <= 8; ++version)
    {
        for (unsigned i = 0; i < 5; ++i)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_ScriptFunction body;
            body.body.push_back(0);
            body.function2 = true;
            body.register_count = 8;
            body.flags = 469 | paired[i];
            body.parameters.push_back(fvm_detail::string("x"));
            body.parameter_registers.push_back(7);
            FVM_Object *fn = 0, *receiver = 0;
            if (!(fvm_function_make_script(c, body, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_create(c, &receiver) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Probe probe = {body.flags, 0, receiver};
            if (!(fvm_function_set_body(c, fn, native, &probe, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value arg, out;
            fvm_value_number(&arg, 19);
            c->pending_finalizer = receiver;
            FVM_GCRootFrame *before = c->gc_frames;
            if (!(fvm_function_invoke(c, fn, receiver, 1, &arg, &out) == FVM_OK && out.number == 41))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(probe.calls == 1 && c->gc_frames == before && c->script_depth == 0 && !c->script_this &&
                  !c->avm1_target_active && c->lexical_scopes.empty()))
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
