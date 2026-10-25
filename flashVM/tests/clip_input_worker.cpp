/* Real OS workers cancel a tree before capture or between script/native handlers with no numeric limit. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_input_fixture.hpp"
#include "flashvm/worker.h"

static unsigned delivered;


static FVM_Result stop_input(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
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


static FVM_Result run(FVM_Context *c, const FVM_Value *input, FVM_Value *out, void *)
{
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes code;
    integer(code, 0);
    text(code, "stopInput");
    code.push_back(0x3d);
    code.push_back(0x17);
    code.push_back(0);
    FVM_Object *root = clip_load(c, clip_movie(8, clip_event_table(8, 8, code)));
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *actor = get(c, root, "actor").object;
    for (unsigned i = 0; i < 70; ++i)
    {
        char name[32];
        std::sprintf(name, "clip%u", i);
        input_duplicate(c, actor, name, i + 3);
    }
    if (!(fvm_register_function(c, "stopInput", stop_input, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
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
    if (!(fvm_movieclip_dispatch_clip_input(c, root, 8) == FVM_ERROR &&
          (input->boolean ? (delivered > 0 && delivered < 71) : !delivered)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    input_clean(c);
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


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
        FVM_Worker *w = 0;
        FVM_Value in, out;
        fvm_value_boolean(&in, i);
        if (!(fvm_worker_run(&cfg, run, &in, 0, &w) == FVM_OK && fvm_worker_join(w, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_worker_destroy(w);
    }
}
