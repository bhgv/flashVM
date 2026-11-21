/* Copyright (c) 2026 bhgv
 * [GC:] EN: Exception-safe initial frame construction before private launch adoption/publication.
 * RU: Exception-safe начальный frame до private launch adoption/publication.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "disvm/refcount.h"
#include "startup_internal.h"
#include "module_counts.h"
#include <stdlib.h>
#include <string.h>


/* [GC:] EN/RU: Unpublished zero-argument frame owns no guest arguments; heap storage survives longjmp.
 * Private frame не владеет guest arguments; heap storage сохраняется через longjmp. */
typedef struct InitialLaunch
{
    Prog initial; /* Temporary register/extent owner until process publication. */
    Modlink *link; /* One caller-owned managed module link reference. */
    Frame *frame; /* Native staging frame, separate from the actual stack extent. */
    void *source; /* Controller's saved native REG source scratch. */
} InitialLaunch;


/* [GC:] EN/RU: See launch/runtime.h; reserve entry frame, adopt handoff, then publish process.
 * См. launch/runtime.h; entry frame и adoption до публикации process. */
extern Prog *dvm_runtime_create_hosted_child(Modlink *, const REG *, DVM_LaunchHandoff *, const DVM_LaunchCompletion *);

static Prog *launch_module(Module *module, DVM_LaunchHandoff *handoff, const DVM_LaunchCompletion *completion)
{
    Import empty;
    Prog *process;
    memset(&empty, 0, sizeof(empty));
    if (!module || module == H || !module->entryt || module->entryt->size < sizeof(Frame))
    {
        error("incompatible legacy module frame; native-cell module required");
    }
    InitialLaunch *state = calloc(1, sizeof(*state));
    if (!state)
    {
        error(exNomem);
    }
    state->source = DVM_R.s;
    if (waserror())
    {
        DVM_R.s = state->source;
        free(state->initial.R.EX);
        free(state->frame);
        if (state->link)
        {
            destroy(state->link);
        }
        free(state);
        nexterror();
    }
    dvm_runtime_module_retain(&module->ref);
    state->link = linkmod(module, &empty, 1);
    if (state->link == H)
    {
        error("cannot link bootstrap module");
    }
    state->frame = calloc(1, module->entryt->size);
    if (!state->frame)
    {
        error(exNomem);
    }
    state->frame->t = module->entryt;
    state->initial.R.M = state->link;
    state->initial.R.MP = state->link->MP;
    state->initial.R.PC = module->entry;
    DVM_R.s = &state->frame;
    newstack(&state->initial);
    initmem(module->entryt, state->initial.R.FP);
    process = completion ? dvm_runtime_create_hosted_child(state->link, &state->initial.R, handoff, completion) :
        dvm_runtime_create_initial(state->link, &state->initial.R, handoff);
    state->initial.R.EX = NULL;
    Modlink *link = state->link;
    state->link = NULL;
    if (dvm_refcount_release_u(&D2H(link)->ref) != DVM_REFCOUNT_LIVE)
    {
        error(exHeap);
    }
    poperror();
    DVM_R.s = state->source;
    free(state->frame);
    free(state);
    return process;
}


/* EN/RU: See runtime.h. / См. runtime.h. */
Prog *dvm_runtime_launch_module(Module *module, DVM_LaunchHandoff *handoff)
{
    return launch_module(module, handoff, NULL);
}


/* EN/RU: See runtime.h. / См. runtime.h. */
Prog *dvm_runtime_launch_child(Module *module, DVM_LaunchHandoff *handoff, const DVM_LaunchCompletion *completion)
{
    return launch_module(module, handoff, completion);
}
