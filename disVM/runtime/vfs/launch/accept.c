/* Copyright (c) 2026 bhgv
 * EN: One preparation attempt, independent native cancellation forwarding and provider-only handoff.
 * RU: Одна попытка подготовки, независимая native передача отмены и provider-only handoff.
 */
#include "ticket_internal.h"
#include "disvm/unibus_binding.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: See ticket.h; source wrappers stay owned through preparation and notifications.
 * См. ticket.h; source wrappers удерживаются до завершения preparation/notices. */
int dvm_launch_request_prepare(DVM_LaunchRequest *request, DVM_Provider *provider, const DVM_Origin *origin)
{
    if (!request || !origin)
    {
        return 0;
    }
    DVM_Origin accepted = *origin;
    if (!dvm_launch_origin_valid(&accepted, DVM_ORIGIN_PROVIDER) ||
        !dvm_launch_root_cancellable(provider))
    {
        return 0;
    }
    LaunchShared *shared = request->shared;
    if (strcmp(shared->description.expected_provider, accepted.system) ||
        strcmp(shared->description.terminal_origin.session, accepted.session))
    {
        return 0;
    }
    fvm_port_sync_lock(shared->lock);
    int admitted = shared->state == DVM_LAUNCH_PENDING && shared->contract_checked;
    if (admitted)
    {
        shared->state = DVM_LAUNCH_PREPARING;
    }
    fvm_port_sync_unlock(shared->lock);
    if (!admitted)
    {
        return 0;
    }
    dvm_launch_shared_changed(shared);
    DVM_Cancel *parent = dvm_cancel_current();
    DVM_CancelTap *tap = parent ? dvm_cancel_forward(parent, shared->cancel) : NULL;
    int connected = !parent || tap;
    DVM_CancelScope scope = {0};
    DVM_LaunchHandoff context = {0};
    int prepared = 0;
    if (connected && dvm_cancel_enter(&scope, shared->cancel))
    {
        DVM_LaunchOptions options = {0};
        options.policy = shared->description.policy;
        options.terminal = shared->terminal;
        options.provider = provider;
        options.terminal_origin = shared->description.terminal_origin;
        options.provider_origin = accepted;
        context.group = dvm_launch_prepare(&options);
        const DVM_LaunchProgram *selected = &shared->selection;
        prepared = context.group && (!shared->program_root ||
            dvm_launch_handoff_program(&context, shared->program_root, selected->pid,
                selected->parent_pid, selected->hidden_fds));
        if (!dvm_cancel_leave(&scope))
        {
            abort();
        }
    }
    dvm_cancel_unsubscribe(tap);
    fvm_port_sync_lock(shared->lock);
    int ready = 0;
    if (shared->state == DVM_LAUNCH_PREPARING)
    {
        if (dvm_cancel_requested(shared->cancel) || dvm_cancel_requested(parent))
        {
            shared->state = DVM_LAUNCH_CANCELLED;
        }
        else if (prepared)
        {
            shared->group = context.group;
            shared->program = context.program;
            shared->program_parent = context.program_parent;
            shared->program_vfs = context.program_vfs;
            shared->program_var = context.program_var;
            context.group = NULL;
            context.program = NULL;
            context.program_parent = NULL;
            context.program_vfs = NULL;
            context.program_var = NULL;
            shared->provider_origin = accepted;
            shared->state = DVM_LAUNCH_READY;
            ready = 1;
        }
        else
        {
            shared->state = DVM_LAUNCH_DENIED;
        }
    }
    fvm_port_sync_unlock(shared->lock);
    dvm_namespace_release(context.group);
    dvm_provider_release(context.program);
    dvm_provider_release(context.program_parent);
    dvm_provider_release(context.program_vfs);
    dvm_provider_release(context.program_var);
    dvm_launch_shared_changed(shared);
    return ready;
}


/* EN/RU: See ticket.h; allocation/copy precedes ownership commit, so refusal preserves READY.
 * См. ticket.h; allocation/copy до ownership commit, refusal сохраняет READY. */
DVM_LaunchHandoff *dvm_launch_request_take(DVM_LaunchRequest *request)
{
    if (!request)
    {
        return NULL;
    }
    LaunchShared *shared = request->shared;
    DVM_LaunchHandoff *handoff = calloc(1, sizeof(*handoff));
    if (!handoff)
    {
        return NULL;
    }
    handoff->description = shared->description;
    if (shared->description.argument_bytes)
    {
        handoff->arguments = malloc(shared->description.argument_bytes);
        if (!handoff->arguments)
        {
            free(handoff);
            return NULL;
        }
        memcpy(handoff->arguments, shared->arguments, shared->description.argument_bytes);
    }
    if (shared->contract)
    {
        handoff->contract = dvm_launch_contract_clone(shared->contract);
        if (!handoff->contract)
        {
            dvm_launch_handoff_free(handoff);
            return NULL;
        }
    }
    fvm_port_sync_lock(shared->lock);
    int taken = shared->state == DVM_LAUNCH_READY;
    if (taken)
    {
        handoff->group = shared->group;
        handoff->program = shared->program;
        handoff->program_parent = shared->program_parent;
        handoff->program_vfs = shared->program_vfs;
        handoff->program_var = shared->program_var;
        handoff->program_hidden_fds = shared->selection.hidden_fds;
        shared->program = NULL;
        shared->program_parent = NULL;
        shared->program_vfs = NULL;
        shared->program_var = NULL;
        shared->group = NULL;
        handoff->provider_origin = shared->provider_origin;
        handoff->flashvm = shared->flashvm;
        memset(&shared->flashvm, 0, sizeof(shared->flashvm));
        shared->state = DVM_LAUNCH_TAKEN;
    }
    fvm_port_sync_unlock(shared->lock);
    if (!taken)
    {
        dvm_launch_handoff_free(handoff);
        return NULL;
    }
    dvm_launch_shared_changed(shared);
    return handoff;
}


/* EN/RU: See ticket.h; independent transferred namespace/data outlive both request wrappers.
 * См. ticket.h; transferred namespace/data живут независимо от request wrappers. */
void dvm_launch_handoff_free(DVM_LaunchHandoff *handoff)
{
    if (handoff)
    {
        dvm_namespace_release(handoff->group);
        dvm_provider_release(handoff->flashvm.provider);
        dvm_provider_release(handoff->program);
        dvm_provider_release(handoff->program_parent);
        dvm_provider_release(handoff->program_vfs);
        dvm_provider_release(handoff->program_var);
        dvm_movie_offer_release(handoff->flashvm.offers);
        unibusrelease(handoff->unibus);
        dvm_launch_contract_free(handoff->contract);
        free(handoff->arguments);
        free(handoff);
    }
}


/* EN/RU: See ticket.h; the binding is claimed only by the actual process adoption transaction.
 * См. ticket.h; claim выполняет только настоящая транзакция принятия process.
 */
int dvm_launch_handoff_unibus(DVM_LaunchHandoff *handoff, UnibusBinding *binding)
{
    if (!handoff || !handoff->group || handoff->unibus || !unibusretain(binding))
    {
        return 0;
    }
    handoff->unibus = binding;
    return 1;
}
