/* Getter/native errors, native C++ throws and script exceptions restore all delivery scopes. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_input_methods_fixture.hpp"


/* Select one callback result and count committed entries. */
struct Failure
{
    unsigned mode;  /* Error/exception/native-throw selector. */
    unsigned calls; /* Entries whose script-visible effects were committed. */
};


/* Request redraw before failing so cleanup cannot accidentally roll back committed effects. */
static FVM_Result fail(FVM_Context *c, FVM_Object *, unsigned count, const FVM_Value *, FVM_Value *out, void *raw)
{
    Failure *failure = static_cast<Failure *>(raw);
    if (!(!count && c->stage_update_allowed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++failure->calls;
    if (!(fvm_call(c, "updateAfterEvent", 0, 0, out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failure->mode == 1)
    {
        return FVM_ERROR;
    }
    if (failure->mode == 2)
    {
        return FVM_UNSUPPORTED;
    }
    if (failure->mode == 3)
    {
        throw std::bad_alloc();
    }
    if (failure->mode == 4)
    {
        throw 42;
    }
    if (failure->mode == 5)
    {
        FVM_Value exception;
        fvm_value_number(&exception, 99);
        return fvm_exception_raise(c, &exception);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* Exercise failure during both member resolution and the actual method call. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const FVM_Result expected[] = {FVM_OK, FVM_ERROR, FVM_UNSUPPORTED, FVM_OUT_OF_MEMORY, FVM_ERROR, FVM_EXCEPTION};
    for (unsigned getter = 0; getter < 2; ++getter)
    {
        for (unsigned mode = 1; mode <= 5; ++mode)
        {
            FVM_Context *c = clip_context(runtime);
            FVM_Object *root = method_root(c);
            Failure failure = {mode, 0};
            if (getter)
            {
                if (!(fvm_object_define_accessor_rooted(c, root, "onMouseMove", fail, 0, &failure, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(fvm_object_define_method_rooted(c, root, "onMouseMove", fail, &failure, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_movieclip_dispatch_input(c, root, 8) == expected[mode] && failure.calls == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->stage_update_pending))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            input_clean(c);
            if (mode == 5)
            {
                if (!(fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            failure.mode = 0;
            if (!(fvm_movieclip_dispatch_input(c, root, 8) == FVM_OK && failure.calls == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            input_clean(c);
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(runtime);
}
