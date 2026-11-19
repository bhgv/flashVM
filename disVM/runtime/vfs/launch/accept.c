/* Copyright (c) 2026 bhgv
 * EN: One preparation attempt, independent native cancellation forwarding and provider-only handoff.
 * RU: Одна попытка подготовки, независимая native передача отмены и provider-only handoff.
 */
#include "ticket_internal.h"
#include "disvm/unibus_binding.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Forward only to a retained native cancellation source, no request/provider/guest pointers.
 * Передать сигнал только retained native source, без request/provider/guest pointers. */
static void forward(void *state)
{
    dvm_cancel_request(state);
}


/* EN/RU: Consume forwarding callback's independent native pin after all admitted signals retire.
 * Поглотить independent native pin после завершения всех admitted signals. */
static void release(void *state)
{
    dvm_cancel_release(state);
}


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
    DVM_CancelTap *tap = NULL;
    int connected = 1;
    if (parent)
    {
        connected = dvm_cancel_retain(shared->cancel);
        if (connected)
        {
            tap = dvm_cancel_subscribe(parent, forward, shared->cancel, release);
            if (!tap)
            {
                dvm_cancel_release(shared->cancel);
                connected = 0;
            }
        }
    }
    DVM_CancelScope scope = {0};
    DVM_Namespace *group = NULL;
    if (connected && dvm_cancel_enter(&scope, shared->cancel))
    {
        DVM_LaunchOptions options = {0};
        options.policy = shared->description.policy;
        options.terminal = shared->terminal;
        options.provider = provider;
        options.terminal_origin = shared->description.terminal_origin;
        options.provider_origin = accepted;
        group = dvm_launch_prepare(&options);
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
        else if (group)
        {
            shared->group = group;
            group = NULL;
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
    dvm_namespace_release(group);
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
    fvm_port_sync_lock(shared->lock);
    int taken = shared->state == DVM_LAUNCH_READY;
    if (taken)
    {
        handoff->group = shared->group;
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
        dvm_movie_offer_release(handoff->flashvm.offers);
        unibusrelease(handoff->unibus);
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
