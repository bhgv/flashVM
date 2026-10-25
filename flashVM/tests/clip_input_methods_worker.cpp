/* Actual OS workers stop before optional-method capture and between native receiver callbacks. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_input_methods_fixture.hpp"
#include "flashvm/worker.h"

static unsigned delivered; /* Worker-owned counter, used by one joined worker at a time. */


/* Request cooperative stop from a real native MovieClip method. */
static FVM_Result stop(FVM_Context *c, FVM_Object *, unsigned count, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(!count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++delivered;
    FVM_Worker *self = 0;
    if (!(fvm_worker_current(c, &self) == FVM_OK && fvm_worker_stop(self) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* Construct and dispatch inside the worker's owner thread with no numeric work limit. */
static FVM_Result run(FVM_Context *c, const FVM_Value *input, FVM_Value *out, void *)
{
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *root = method_root(c), *actor = get(c, root, "actor").object;
    if (!(fvm_object_define_method_rooted(c, actor, "onMouseMove", stop, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 70; ++i)
    {
        char name[32];
        std::sprintf(name, "receiver%u", i);
        FVM_Object *child = input_duplicate(c, actor, name, i + 3);
        if (!(fvm_object_define_method_rooted(c, child, "onMouseMove", stop, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    delivered = 0;
    if (!input->boolean)
    {
        FVM_Worker *self = 0;
        if (!(fvm_worker_current(c, &self) == FVM_OK && fvm_worker_stop(self) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_movieclip_dispatch_input(c, root, 8) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(input->boolean ? (delivered > 0 && delivered < 71) : !delivered))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    input_clean(c);
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


/* Join each worker before reusing the shared native fixture counter. */
int main()
{
    FVM_Config cfg;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cfg.instruction_limit = 0;
    for (unsigned i = 0; i < 2; ++i)
    {
        FVM_Worker *worker = 0;
        FVM_Value input, output;
        fvm_value_boolean(&input, i);
        if (!(fvm_worker_run(&cfg, run, &input, 0, &worker) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_worker_join(worker, &output) == FVM_OK && output.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_worker_destroy(worker);
    }
}
