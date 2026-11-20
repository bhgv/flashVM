/* Copyright (c) 2026 bhgv
 * EN: Native request capability lifetime, copied startup data, cancellation and status notices.
 * RU: Lifetime native request capabilities, copied startup data, отмена и status notices.
 */
#include "ticket_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Release unpublished or exclusively final shared storage; no callers may remain.
 * Освободить unpublished/final shared storage, callers отсутствуют. */
static void destroy(LaunchShared *shared)
{
    if (shared)
    {
        if (shared->retired)
        {
            shared->retired(shared->context);
        }
        dvm_namespace_release(shared->group);
        dvm_provider_release(shared->terminal);
        dvm_provider_release(shared->program_root);
        dvm_provider_release(shared->program);
        dvm_provider_release(shared->program_parent);
        dvm_provider_release(shared->program_vfs);
        dvm_provider_release(shared->program_var);
        dvm_provider_release(shared->flashvm.provider);
        dvm_movie_offer_release(shared->flashvm.offers);
        dvm_cancel_release(shared->cancel);
        dvm_changes_destroy(shared->changes);
        if (shared->lock)
        {
            fvm_port_sync_destroy(shared->lock);
        }
        dvm_launch_contract_free(shared->contract);
        free(shared->arguments);
        free(shared);
    }
}


/* EN/RU: See internal header; finalizing records cannot regain references.
 * См. internal header; finalizing records не получают новые references. */
int dvm_launch_shared_retain(LaunchShared *shared)
{
    fvm_port_sync_lock(shared->lock);
    int live = shared->references && shared->references != (unsigned)-1;
    if (live)
    {
        shared->references++;
    }
    fvm_port_sync_unlock(shared->lock);
    return live;
}


/* EN/RU: See internal header; no native destructor executes under shared mutex.
 * См. internal header; native destructors не выполняются под shared mutex. */
void dvm_launch_shared_drop(LaunchShared *shared)
{
    fvm_port_sync_lock(shared->lock);
    int last = --shared->references == 0;
    fvm_port_sync_unlock(shared->lock);
    if (last)
    {
        destroy(shared);
    }
}


/* EN/RU: See internal header; notices may coalesce/reorder, state remains authoritative.
 * См. internal header; notices могут объединяться/переставляться, state authoritative. */
void dvm_launch_shared_changed(LaunchShared *shared)
{
    changespost(shared->changes, DVM_CHANGE_CONNECTION, 0, 0);
    if (shared->changed)
    {
        shared->changed(shared->context);
    }
}


/* EN/RU: See ticket.h; allocation and root retention finish before capability publication.
 * См. ticket.h; allocation/root retention завершаются до выдачи capabilities. */
DVM_LaunchTicket *dvm_launch_request_create(const DVM_LaunchSpec *spec, DVM_LaunchRequest **out)
{
    if (out)
    {
        *out = NULL;
    }
    if (!out || !spec || (spec->policy != DVM_LAUNCH_TERMINAL_FIRST && spec->policy != DVM_LAUNCH_OWNER_FIRST) || !dvm_launch_root_cancellable(spec->terminal) ||
        !dvm_launch_origin_valid(&spec->terminal_origin, DVM_ORIGIN_TERMINAL) ||
        !dvm_origin_id_valid(spec->expected_provider) || !spec->module[0] ||
        !memchr(spec->module, 0, sizeof(spec->module)) || spec->argument_bytes > DVM_LAUNCH_ARGUMENT_LIMIT ||
        (spec->argument_bytes && !spec->arguments) ||
        (spec->program_root ? (!dvm_launch_root_cancellable(spec->program_root) ||
            !spec->program.pid || spec->program.pid == spec->program.parent_pid) :
            (spec->program.pid || spec->program.parent_pid || spec->program.hidden_fds)) ||
        (spec->flashvm && !dvm_terminal_channels_valid(spec->flashvm)))
    {
        return NULL;
    }
    LaunchShared *shared = calloc(1, sizeof(*shared));
    DVM_LaunchTicket *ticket = calloc(1, sizeof(*ticket));
    DVM_LaunchRequest *request = calloc(1, sizeof(*request));
    if (shared)
    {
        shared->lock = fvm_port_sync_create();
        shared->cancel = dvm_cancel_create();
        shared->changes = dvm_changes_create();
        shared->contract = dvm_launch_contract_clone(spec->contract);
        shared->contract_checked = !spec->contract;
        shared->description.typed = spec->contract != NULL;
        if (spec->argument_bytes)
        {
            shared->arguments = malloc(spec->argument_bytes);
        }
    }
    if (!shared || !ticket || !request || !shared->lock || !shared->cancel || !shared->changes ||
        (spec->argument_bytes && !shared->arguments) || (spec->contract && !shared->contract) ||
        !dvm_provider_retain(spec->terminal))
    {
        destroy(shared);
        free(ticket);
        free(request);
        return NULL;
    }
    shared->terminal = spec->terminal;
    if (spec->program_root && !dvm_provider_retain(spec->program_root))
    {
        destroy(shared);
        free(ticket);
        free(request);
        return NULL;
    }
    shared->program_root = spec->program_root;
    shared->selection = spec->program;
    if (spec->flashvm)
    {
        if (!dvm_provider_retain(spec->flashvm->provider))
        {
            destroy(shared);
            free(ticket);
            free(request);
            return NULL;
        }
        if (!dvm_movie_offer_retain(spec->flashvm->offers))
        {
            dvm_provider_release(spec->flashvm->provider);
            destroy(shared);
            free(ticket);
            free(request);
            return NULL;
        }
        shared->flashvm = *spec->flashvm;
        shared->description.terminal_channels = 1;
    }
    shared->description.policy = spec->policy;
    shared->description.terminal_origin = spec->terminal_origin;
    strcpy(shared->description.expected_provider, spec->expected_provider);
    strcpy(shared->description.module, spec->module);
    shared->description.argument_bytes = spec->argument_bytes;
    if (spec->argument_bytes)
    {
        memcpy(shared->arguments, spec->arguments, spec->argument_bytes);
    }
    shared->references = 2;
    ticket->shared = request->shared = shared;
    *out = request;
    return ticket;
}


/* EN/RU: See ticket.h; copy state under its mutex, without provider I/O.
 * См. ticket.h; скопировать state под mutex без provider I/O. */
DVM_LaunchState dvm_launch_ticket_status(DVM_LaunchTicket *ticket)
{
    if (!ticket)
    {
        return DVM_LAUNCH_INVALID;
    }
    fvm_port_sync_lock(ticket->shared->lock);
    DVM_LaunchState state = ticket->shared->state;
    fvm_port_sync_unlock(ticket->shared->lock);
    return state;
}


/* EN/RU: See ticket.h; description is immutable while provider wrapper is alive.
 * См. ticket.h; description immutable до завершения provider wrapper. */
int dvm_launch_describe(DVM_LaunchRequest *request, DVM_LaunchDescription *out)
{
    if (!request || !out)
    {
        return 0;
    }
    *out = request->shared->description;
    return 1;
}


/* EN/RU: See ticket.h; argument bytes are immutable, slice offsets cannot wrap.
 * См. ticket.h; bytes immutable, slice offsets не переполняются. */
int dvm_launch_arguments(DVM_LaunchRequest *request, size_t offset, void *bytes, size_t capacity, size_t *copied)
{
    if (!request || !copied || (capacity && !bytes) || offset > request->shared->description.argument_bytes)
    {
        return 0;
    }
    *copied = request->shared->description.argument_bytes-offset;
    if (*copied > capacity)
    {
        *copied = capacity;
    }
    if (*copied)
    {
        memcpy(bytes, request->shared->arguments+offset, *copied);
    }
    return 1;
}


/* EN/RU: See ticket.h; returned journal owns no request, provider or guest pointers.
 * См. ticket.h; returned journal не владеет request/provider/guest pointers. */
DVM_Changes *dvm_launch_observe(DVM_LaunchTicket *ticket)
{
    return ticket && changesretain(ticket->shared->changes) ? ticket->shared->changes : NULL;
}


/* EN/RU: Terminate a borrowed shared request before take; clear prepared group atomically.
 * Terminal state is CANCELLED or DENIED; signals/destruction/notifications run unlocked.
 * Завершить shared request до take, убрать group атомарно; signals/cleanup/notices вне lock. */
int dvm_launch_shared_terminate(LaunchShared *shared, DVM_LaunchState state)
{
    DVM_Namespace *group = NULL;
    DVM_Provider *program = NULL;
    DVM_Provider *parent = NULL;
    DVM_Provider *vfs = NULL;
    DVM_Provider *vars = NULL;
    fvm_port_sync_lock(shared->lock);
    int changed = shared->state == DVM_LAUNCH_PENDING || shared->state == DVM_LAUNCH_PREPARING ||
        shared->state == DVM_LAUNCH_READY;
    if (changed)
    {
        shared->state = state;
        group = shared->group;
        program = shared->program;
        parent = shared->program_parent;
        vfs = shared->program_vfs;
        vars = shared->program_var;
        shared->group = NULL;
        shared->program = NULL;
        shared->program_parent = NULL;
        shared->program_vfs = NULL;
        shared->program_var = NULL;
    }
    fvm_port_sync_unlock(shared->lock);
    if (changed)
    {
        dvm_cancel_request(shared->cancel);
        dvm_namespace_release(group);
        dvm_provider_release(program);
        dvm_provider_release(parent);
        dvm_provider_release(vfs);
        dvm_provider_release(vars);
        dvm_launch_shared_changed(shared);
    }
    return changed;
}


/* EN/RU: See ticket.h; requester cannot take or prepare the provider environment.
 * См. ticket.h; requester не получает prepare/take authority. */
int dvm_launch_ticket_cancel(DVM_LaunchTicket *ticket)
{
    return ticket ? dvm_launch_shared_terminate(ticket->shared, DVM_LAUNCH_CANCELLED) : 0;
}


/* EN/RU: See ticket.h; provider can refuse unfinished work without executing guest code.
 * См. ticket.h; provider отклоняет unfinished work без guest execution. */
int dvm_launch_request_refuse(DVM_LaunchRequest *request)
{
    return request ? dvm_launch_shared_terminate(request->shared, DVM_LAUNCH_DENIED) : 0;
}


/* EN/RU: See ticket.h; free after requester callers join, preserving TAKEN owners.
 * См. ticket.h; free после join requester callers, TAKEN owners сохраняются. */
void dvm_launch_ticket_free(DVM_LaunchTicket *ticket)
{
    if (ticket)
    {
        dvm_launch_ticket_cancel(ticket);
        dvm_launch_shared_drop(ticket->shared);
        free(ticket);
    }
}


/* EN/RU: See ticket.h; provider abandonment refuses unfinished work, not transferred ownership.
 * См. ticket.h; abandonment отклоняет unfinished work, не transferred ownership. */
void dvm_launch_request_free(DVM_LaunchRequest *request)
{
    if (request)
    {
        dvm_launch_request_refuse(request);
        dvm_launch_shared_drop(request->shared);
        free(request);
    }
}
