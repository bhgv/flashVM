/* Host-version zero, invisible/stopped receivers, resolver lookup and protected lifecycle phases. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_input_methods_fixture.hpp"


/* Record a resolver result and the actual native method count. */
struct Policy
{
    FVM_Object *function; /* [GC:] Function rooted by the receiver's resolver entry. */
    unsigned resolves;    /* Missing-handler resolutions. */
    unsigned calls;       /* Actual zero-argument input notifications. */
};


/* Native host clips receive global input independently of visibility and playback. */
static FVM_Result receive(FVM_Context *c, FVM_Object *, unsigned count, const FVM_Value *, FVM_Value *out, void *raw)
{
    Policy *policy = static_cast<Policy *>(raw);
    if (!(!count && c->clip_input_dispatching))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++policy->calls;
    fvm_context_destroy(c);
    if (!(c->global))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fvm_call(c, "updateAfterEvent", 0, 0, out);
}


/* Resolve only the requested method and preserve the supplied member name. */
static FVM_Result resolve(FVM_Context *, FVM_Object *, unsigned count, const FVM_Value *args, FVM_Value *out, void *raw)
{
    Policy *policy = static_cast<Policy *>(raw);
    if (!(count == 1 && args[0].type == FVM_VALUE_STRING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::strcmp(args[0].string, "onMouseMove")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++policy->resolves;
    *out = method_value(policy->function);
    return FVM_OK;
}


/* Native roots require no SWF library, and invalid calls cannot consume any work. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned loaded = 0; loaded < 2; ++loaded)
    {
        FVM_Context *c = clip_context(runtime), *other = clip_context(runtime);
        FVM_Object *root = 0;
        if (loaded)
        {
            const unsigned char profile[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            if (!(fvm_load_memory(c, profile, sizeof(profile)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            root = method_root(c);
        }
        else
        {
            if (!(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        Policy policy = {0, 0, 0};
        if (!(fvm_function_create_native(c, receive, &policy, 0, &policy.function) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c,
                                              root,
                                              loaded ? "__resolve" : "onMouseMove",
                                              loaded ? resolve : receive,
                                              &policy,
                                              policy.function) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value, ignored;
        fvm_value_number(&value, 0);
        if (!(fvm_object_set(c, root, "_visible", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, root, "stop", 0, 0, &ignored) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_dispatch_input(c, root, 8) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(policy.resolves == loaded && policy.calls == 1 && c->stage_update_pending))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_dispatch_input(other, root, 8) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_dispatch_input(c, 0, 8) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_dispatch_input(0, root, 8) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned phase = 0; phase < 4; ++phase)
        {
            if (phase == 0)
            {
                c->destroying = true;
            }
            if (phase == 1)
            {
                c->gc_collecting = true;
            }
            if (phase == 2)
            {
                c->gc_releasing = true;
            }
            if (phase == 3)
            {
                c->movieclip_sound_busy = true;
            }
            c->avm1_steps = 77;
            if (!(fvm_movieclip_dispatch_input(c, root, 8) == FVM_INVALID_ARGUMENT && c->avm1_steps == 77))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            c->destroying = c->gc_collecting = c->gc_releasing = c->movieclip_sound_busy = false;
        }
        if (!(policy.calls == 1 && policy.resolves == loaded))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        input_clean(c);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime);
}
