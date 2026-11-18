/* Copyright (c) 2026 bhgv
 * EN: Stage and move launch namespace/channel ownership before process publication.
 * RU: Подготовить и перенести namespace/channel ownership до публикации process.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runqueue.h"
#include "runtime.h"
#include "context.h"
#include "../sysevents.h"
#include "../../../native/movie/service.h"
#include "disvm/unibus_binding.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: See runtime.h; immutable parent metadata is pinned by the spawning owner.
 * См. runtime.h; immutable parent metadata удерживается spawning owner. */
int dvm_launch_process_inherit(Prog *process, Prog *parent)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
    DVM_TerminalChannels *source = parent ? ((DVM_RuntimeProcess *)parent)->terminal_channels : NULL;
    DVM_LaunchContext *context = parent ? ((DVM_RuntimeProcess *)parent)->launch_context : NULL;
    if (context && !dvm_launch_context_retain(context))
    {
        return 0;
    }
    owner->launch_context = context;
    if (!source)
    {
        return 1;
    }
    DVM_TerminalChannels *copy = malloc(sizeof(*copy));
    if (!copy || !dvm_provider_retain(source->provider))
    {
        free(copy);
        dvm_launch_context_release(owner->launch_context);
        owner->launch_context = NULL;
        return 0;
    }
    *copy = *source;
    copy->offers = NULL; /* A child must not replace the parent job's advertised scope. */
    DVM_Provider *movie = styxmovieservice(owner->namespace_group);
    if (!movie)
    {
        dvm_provider_release(copy->provider);
        free(copy);
        dvm_launch_context_release(owner->launch_context);
        owner->launch_context = NULL;
        return 0;
    }
    owner->movie_provider = movie;
    owner->terminal_channels = copy;
    return 1;
}


/* EN/RU: See runtime.h; provider destruction stays native and outside process/namespace locks.
 * См. runtime.h; provider destruction native и вне process/namespace locks. */
void dvm_launch_process_release(Prog *process)
{
    if (process)
    {
        DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
        DVM_TerminalChannels *channels = owner->terminal_channels;
        DVM_Provider *movie = owner->movie_provider;
        dvm_launch_context_release(owner->launch_context);
        owner->launch_context = NULL;
        owner->movie_provider = NULL;
        owner->terminal_channels = NULL;
        if (channels && channels->offers)
        {
            if (owner->movie_offer_staged)
            {
                dvm_movie_offer_abort(channels->offers);
            }
            else if (owner->movie_offer_generation)
            {
                dvm_movie_offer_close(channels->offers, owner->movie_offer_generation);
            }
            dvm_movie_offer_release(channels->offers);
        }
        owner->movie_offer_generation = 0;
        owner->movie_offer_staged = 0;
        dvm_provider_release(movie);
        if (channels)
        {
            dvm_provider_release(channels->provider);
            free(channels);
        }
    }
}


/* EN/RU: See runtime.h; no fallible setup remains after committing the two ownership moves.
 * См. runtime.h; после commit двух ownership moves fallible setup больше нет. */
int dvm_launch_process_adopt(Prog *process, DVM_LaunchHandoff *handoff)
{
    if (!process || !handoff || !handoff->group ||
        (handoff->local_role && handoff->local_role != DVM_ORIGIN_TERMINAL && handoff->local_role != DVM_ORIGIN_PROVIDER))
    {
        return 0;
    }
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
    if (owner->launch_adopted || owner->messages || owner->dedicated || owner->dispatch_owner ||
        owner->native_owner || process->osenv != &owner->environment || !owner->namespace_group ||
        (!!handoff->description.terminal_channels != !!handoff->flashvm.provider) ||
        (handoff->flashvm.provider && !dvm_terminal_channels_valid(&handoff->flashvm)))
    {
        return 0;
    }
    DVM_TerminalChannels *channels = NULL;
    if (handoff->flashvm.provider)
    {
        channels = malloc(sizeof(*channels));
        if (!channels)
        {
            return 0;
        }
        *channels = handoff->flashvm;
    }
    DVM_SysEvents *events = dvm_sysevents_create(handoff->group);
    if (!events)
    {
        free(channels);
        return 0;
    }
    DVM_Provider *movie = channels ? styxmovieservice(handoff->group) : NULL;
    if (channels && !movie)
    {
        dvm_sysevents_retire(events);
        dvm_sysevents_release(events);
        free(channels);
        return 0;
    }
    int staged = channels && channels->offers ? dvm_movie_offer_stage(channels->offers, movie) : 0;
    if (channels && channels->offers && !staged)
    {
        dvm_provider_release(movie);
        dvm_sysevents_retire(events);
        dvm_sysevents_release(events);
        free(channels);
        return 0;
    }
    DVM_LaunchContext *context = dvm_launch_context_create(handoff);
    if (!context || (handoff->unibus && !unibusclaim(handoff->unibus, (uintptr_t)process)))
    {
        dvm_launch_context_release(context);
        if (staged)
        {
            dvm_movie_offer_abort(channels->offers);
        }
        dvm_provider_release(movie);
        dvm_sysevents_retire(events);
        dvm_sysevents_release(events);
        free(channels);
        return 0;
    }
    DVM_Namespace *previous = owner->namespace_group;
    DVM_SysEvents *previous_events = owner->system_events;
    dvm_launch_process_release(process);
    unibusownerretired(owner->unibus, (uintptr_t)process);
    unibusrelease(owner->unibus);
    owner->unibus = handoff->unibus;
    owner->unibus_isolated = 1;
    handoff->unibus = NULL;
    owner->namespace_group = handoff->group;
    owner->system_events = events;
    owner->terminal_channels = channels;
    owner->movie_provider = movie;
    owner->movie_offer_staged = staged;
    owner->launch_context = context;
    owner->module_local_role = handoff->local_role ? handoff->local_role : DVM_ORIGIN_PROVIDER;
    owner->provider_execution = 1; /* Trusted handoff establishes this runtime as the execution provider before publication. */
    owner->launch_adopted = 1;
    handoff->group = NULL;
    memset(&handoff->flashvm, 0, sizeof(handoff->flashvm));
    dvm_sysevents_retire(previous_events);
    dvm_sysevents_release(previous_events);
    dvm_namespace_release(previous);
    return 1;
}


/* EN/RU: Publish only after resident identity/mailbox checks, before its first guest dispatch.
 * Объявить после проверки resident identity/mailbox, до первого guest dispatch. */
int dvm_launch_process_announce(Prog *process)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)process;
    if (!owner->movie_offer_staged)
    {
        return 1;
    }
    uint64_t generation = dvm_movie_offer_commit(owner->terminal_channels->offers);
    if (!generation)
    {
        return 0;
    }
    owner->movie_offer_staged = 0;
    owner->movie_offer_generation = generation;
    return 1;
}
