/* All ticks, descendants and nested native callbacks share one instruction/work budget. */
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_tick_fixture.hpp"

static FVM_Object *nested_root = 0;


static FVM_Result nested(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    return fvm_movieclip_advance(c, nested_root, 1);
}


int main()
{
    for (unsigned limit = 1; limit <= 8; ++limit)
    {
        FVM_Config cfg;
        FVM_Runtime *rt = 0;
        if (!(fvm_config_init(&cfg) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        cfg.instruction_limit = limit;
        if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Context *c = tick_context(rt);
        FVM_Object *root = tick_root(c);
        TickProbe p;
        if (!(fvm_object_define_method_rooted(c, root, "onEnterFrame", tick_notify, &p, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, root, 3) == (limit < 6 ? FVM_ERROR : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(p.calls == (limit < 6 ? limit / 2 : 3)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->avm1_steps == (limit < 6 ? limit : 6)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        unsigned before = p.calls;
        if (!(fvm_movieclip_advance(c, root, 0) == FVM_OK && p.calls == before && !c->avm1_steps))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
    }
    for (unsigned limit = 3; limit <= 4; ++limit)
    {
        FVM_Config cfg;
        FVM_Runtime *rt = 0;
        if (!(fvm_config_init(&cfg) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        cfg.instruction_limit = limit;
        if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Context *c = tick_context(rt);
        FVM_Object *root = tick_root(c);
        nested_root = tick_root(c);
        TickProbe p;
        if (!(fvm_object_define_method_rooted(c, nested_root, "onEnterFrame", tick_notify, &p, 0) == FVM_OK &&
              fvm_object_define_method_rooted(c, root, "onEnterFrame", nested, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, root, 1) == (limit == 4 ? FVM_OK : FVM_ERROR) && p.calls == (limit == 4)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->avm1_steps == limit))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        fvm_context_destroy(c);
        fvm_runtime_destroy(rt);
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cfg.instruction_limit = 6;
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Context *c = tick_context(rt);
    FVM_Object *root = tick_root(c);
    TickProbe p;
    for (unsigned i = 0; i < 3; ++i)
    {
        FVM_Object *child = tick_child(c, root, i == 0 ? "a" : i == 1 ? "b" : "c", i);
        if (!(fvm_object_define_method_rooted(c, child, "onEnterFrame", tick_notify, &p, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_ERROR && !p.calls && c->avm1_steps == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick_clean(c);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
