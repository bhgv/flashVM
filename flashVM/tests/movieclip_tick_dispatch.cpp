/* Optional tick methods preserve defining owner, getter cardinality and committed effects. */
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_tick_fixture.hpp"


struct OwnerProbe
{
    FVM_Object *self, *owner, *function;
    unsigned calls, getter_calls;


    OwnerProbe() : self(0), owner(0), function(0), calls(0), getter_calls(0)
    {
    }
};


static FVM_Result owned(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *, FVM_Value *out, void *raw)
{
    OwnerProbe *p = (OwnerProbe *)raw;
    if (!(self == p->self && n == 0 && c->calling_owner == p->owner))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++p->calls;
    unsigned long reclaimed = 77;
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT && fvm_gc_step(c, &reclaimed) == FVM_OK && !reclaimed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fvm_call(c, "updateAfterEvent", 0, 0, out);
}


static FVM_Result getter(FVM_Context *, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    OwnerProbe *p = (OwnerProbe *)raw;
    if (!(self == p->self))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++p->getter_calls;
    *out = tick_object(p->function);
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
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 5)
        {
            continue;
        }
        for (unsigned route = 0; route < 3; ++route)
        {
            FVM_Context *c = tick_context(rt, version);
            FVM_Object *root = tick_root(c), *proto = 0;
            OwnerProbe p;
            p.self = root;
            if (!(fvm_object_create(c, &proto) == FVM_OK && fvm_object_set_prototype(c, root, proto) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            p.owner = route ? proto : root;
            if (!(fvm_function_create_native(c, owned, &p, 0, &p.function) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value fn = tick_object(p.function);
            if (route == 2)
            {
                if (!(fvm_object_define_accessor_rooted(c, proto, "onEnterFrame", getter, 0, &p, p.function) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(fvm_object_set(c, p.owner, "onEnterFrame", &fn) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_movieclip_advance(c, root, 2) == FVM_OK && p.calls == 2 &&
                  p.getter_calls == (route == 2 ? 2 : 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!c->stage_update_pending))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            tick_clean(c);
            fvm_context_destroy(c);
        }
        FVM_Context *c = tick_context(rt, version);
        FVM_Object *root = tick_root(c);
        TickProbe p;
        if (!(fvm_object_define_method_rooted(c, root, "onEnterFrame", tick_notify, &p, 0) == FVM_OK &&
              fvm_movieclip_advance(c, root, 2) == FVM_OK && p.calls == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        fvm_context_destroy(c);
    }
    for (unsigned fail = 0; fail < 2; ++fail)
    {
        FVM_Context *c = tick_context(rt);
        FVM_Object *root = tick_root(c, 3);
        TickProbe p;
        p.status = fail ? FVM_EXCEPTION : FVM_ERROR;
        if (!(fvm_object_define_method_rooted(c, root, "onEnterFrame", tick_notify, &p, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, root, 3) == p.status && p.calls == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        if (fail)
        {
            if (!(fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_Value frame;
        if (!(fvm_object_get(c, root, "_currentframe", &frame) == FVM_OK && frame.number == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        p.status = FVM_OK;
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && p.calls == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, root, "_currentframe", &frame) == FVM_OK && frame.number == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    FVM_Context *c = tick_context(rt);
    FVM_Object *root = tick_root(c);
    TickProbe p;
    if (!(fvm_register_function(c, "onEnterFrame", tick_notify, &p) == FVM_OK &&
          fvm_movieclip_advance(c, root, 1) == FVM_OK && !p.calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value noncallable;
    fvm_value_number(&noncallable, 17);
    if (!(fvm_object_set(c, root, "onEnterFrame", &noncallable) == FVM_OK &&
          fvm_movieclip_advance(c, root, 1) == FVM_OK && !p.calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned phase = 0; phase < 3; ++phase)
    {
        if (!phase)
        {
            c->destroying = true;
        }
        else if (phase == 1)
        {
            c->gc_collecting = true;
        }
        else
        {
            c->gc_releasing = true;
        }
        c->avm1_steps = 77;
        if (!(fvm_movieclip_advance(c, root, 0) == FVM_INVALID_ARGUMENT && c->avm1_steps == 77))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        c->destroying = c->gc_collecting = c->gc_releasing = false;
        tick_clean(c);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
