/* Actual OS-worker cancellation interrupts KeyPress capture or delivery without a numeric limit. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_keypress_fixture.hpp"
#include "flashvm/worker.h"

static unsigned calls; /* One joined worker uses this native fixture counter at a time. */


/* Request cooperative stop from inside a retained AVM1-to-native handler. */
static FVM_Result stop(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++calls;
    FVM_Worker *worker = 0;
    if (!(fvm_worker_current(c, &worker) == FVM_OK && fvm_worker_stop(worker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* Construct and execute the graph entirely on its worker owner thread. */
static FVM_Result run(FVM_Context *c, const FVM_Value *input, FVM_Value *out, void *)
{
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *root = keypress_root(c, 8, 1, keypress_call("stopKey")), *actor = get(c, root, "actor").object;
    for (unsigned i = 0; i < 70; ++i)
    {
        char name[32];
        std::sprintf(name, "key%u", i);
        input_duplicate(c, actor, name, i + 3);
    }
    if (!(fvm_register_function(c, "stopKey", stop, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    calls = 0;
    if (!input->boolean)
    {
        FVM_Worker *worker = 0;
        if (!(fvm_worker_current(c, &worker) == FVM_OK && fvm_worker_stop(worker) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_movieclip_dispatch_key_press(c, root, 1) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(input->boolean ? (calls > 0 && calls < 71) : !calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    input_clean(c);
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


/* A pre-capture stop and a stop among callbacks both restore the dispatch guards. */
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
        FVM_Value input, out;
        fvm_value_boolean(&input, i);
        if (!(fvm_worker_run(&cfg, run, &input, 0, &worker) == FVM_OK && fvm_worker_join(worker, &out) == FVM_OK &&
              out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_worker_destroy(worker);
    }
}
