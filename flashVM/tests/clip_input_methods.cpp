/* Input methods preserve source gates, defining owner, getter cardinality and old API behavior. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_input_methods_fixture.hpp"


/* Receiver/owner identities and lookup counts for one method route. */
struct Probe
{
    FVM_Object *self;      /* Borrowed receiver retained by the selected scene. */
    FVM_Object *owner;     /* Defining prototype or own receiver. */
    FVM_Object *function;  /* Getter result rooted by the accessor. */
    unsigned calls;        /* Actual method entries. */
    unsigned getter_calls; /* Getter entries. */
};


/* Verify inherited method identity and the event/collector/reentry boundaries. */
static FVM_Result
receive(FVM_Context *c, FVM_Object *self, unsigned count, const FVM_Value *, FVM_Value *out, void *raw)
{
    Probe *probe = static_cast<Probe *>(raw);
    if (!(self == probe->self && c->calling_owner == probe->owner && !count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->clip_input_dispatching && c->stage_update_allowed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long reclaimed = 77;
    if (!(fvm_gc_step(c, &reclaimed) == FVM_OK && !reclaimed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_dispatch_input(c, self, 8) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_dispatch_clip_input(c, self, 8) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++probe->calls;
    return fvm_call(c, "updateAfterEvent", 0, 0, out);
}


/* Return the single retained function from an inherited getter. */
static FVM_Result lookup(FVM_Context *, FVM_Object *self, unsigned count, const FVM_Value *, FVM_Value *out, void *raw)
{
    Probe *probe = static_cast<Probe *>(raw);
    if (!(self == probe->self && !count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++probe->getter_calls;
    *out = method_value(probe->function);
    return FVM_OK;
}


/* A missing member must never invoke a same-name global function. */
static FVM_Result forbidden(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    if (!(false))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_ERROR;
}


/* Exercise own/inherited/getter/native-method routes for all five source-version policies. */
int main()
{
    const char *names[] = {"onMouseMove", "onMouseDown", "onMouseUp", "onKeyDown", "onKeyUp"};
    const unsigned long events[] = {8, 16, 32, 64, 128};
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 5; version <= 8; ++version)
    {
        for (unsigned route = 0; route < 4; ++route)
        {
            FVM_Context *c = clip_context(runtime);
            FVM_Object *root = method_root(c, version), *proto = 0;
            Probe probe = {root, root, 0, 0, 0};
            if (!(fvm_object_create(c, &proto) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_set_prototype(c, root, proto) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            probe.owner = route == 3 ? 0 : route ? proto : root;
            if (!(fvm_function_create_native(c, receive, &probe, 0, &probe.function) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value fn = method_value(probe.function);
            for (unsigned i = 0; i < 5; ++i)
            {
                if (route == 2)
                {
                    if (!(fvm_object_define_accessor_rooted(c, proto, names[i], lookup, 0, &probe, probe.function) ==
                          FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (route == 3)
                {
                    if (!(fvm_object_define_method_rooted(c, proto, names[i], receive, &probe, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(fvm_object_set(c, probe.owner, names[i], &fn) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_movieclip_dispatch_clip_input(c, root, events[i]) == FVM_OK && !probe.calls))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            for (unsigned i = 0; i < 5; ++i)
            {
                if (!(fvm_movieclip_dispatch_input(c, root, events[i]) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(probe.calls == (version >= 6 ? 5U : 0U)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(probe.getter_calls == (version >= 6 && route == 2 ? 5U : 0U)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->stage_update_pending == (version >= 6)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            input_clean(c);
            fvm_context_destroy(c);
        }
    }
    FVM_Context *c = clip_context(runtime);
    FVM_Object *root = method_root(c);
    for (unsigned i = 0; i < 5; ++i)
    {
        if (!(fvm_register_function(c, names[i], forbidden, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_dispatch_input(c, root, events[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value scalar;
        fvm_value_number(&scalar, 17);
        if (!(fvm_object_set(c, root, names[i], &scalar) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_dispatch_input(c, root, events[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_movieclip_dispatch_input(c, root, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_dispatch_input(c, root, 24) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_dispatch_input(c, root, 1) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    input_clean(c);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
}
