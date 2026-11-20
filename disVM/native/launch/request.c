/* Copyright (c) 2026 bhgv
 * EN: Preflight launch description, capture granted terminal root and publish two native authorities.
 * RU: Preflight launch description, capture granted terminal root и публикация двух native authorities.
 */
#include "request.h"
#include "program_record.h"
#include "mount.h"
#include "cancel.h"
#include <string.h>


/* EN/RU: Borrow spec/access; validate local immutable inputs before any remote request,1/0.
 * Проверить spec/access до remote I/O,1/0, identity strings не credentials. */
static int valid(const DVM_LaunchSpec *spec, unsigned access)
{
    return spec && !spec->terminal && (spec->policy == DVM_LAUNCH_TERMINAL_FIRST || spec->policy == DVM_LAUNCH_OWNER_FIRST) &&
        dvm_launch_origin_valid(&spec->terminal_origin, DVM_ORIGIN_TERMINAL) &&
        dvm_origin_id_valid(spec->expected_provider) && spec->module[0] &&
        memchr(spec->module, 0, sizeof(spec->module)) && spec->argument_bytes <= DVM_LAUNCH_ARGUMENT_LIMIT &&
        (!spec->argument_bytes || spec->arguments) && !spec->flashvm &&
        !(access & ~FVM_ACCESS_MASK) && (access & (FVM_ACCESS_READ|FVM_ACCESS_EXEC)) ==
            (FVM_ACCESS_READ|FVM_ACCESS_EXEC);
}


/* EN/RU: Build ordinary or explicit bidir terminal request; inputs borrowed, terminal ref consumed.
 * Создать ordinary либо bidir terminal request; inputs borrowed, terminal ref consumed.
 * No ticket/request is exposed before all cancellation/ownership checks finish.
 * См. request.h; ticket/request не выдаются до завершения checks и передачи ownership. */
static DVM_LaunchTicket *create(StyxClient *terminal, const DVM_LaunchSpec *spec,
    unsigned access, const StyxTerminalPaths *paths, DVM_LaunchRequest **request, DVM_LaunchEndpoint *endpoint)
{
    if (request)
    {
        *request = NULL;
    }
    if ((!request && !endpoint) || !terminal || !valid(spec, access) || !styxclientcancellable(terminal) ||
        (paths && (!styxclientbidir(terminal) || !(access & FVM_ACCESS_WRITE) ||
            !dvm_terminal_channel_paths_valid(paths->receive, paths->send))) ||
        dvm_cancel_requested(dvm_cancel_current()))
    {
        styxclientfree(terminal);
        return NULL;
    }
    DVM_Provider *root = styxmountproviderat(terminal, ".", access);
    if (!root)
    {
        return NULL;
    }
    FVM_FileInfo attributes;
    DVM_LaunchRequest *pending = NULL;
    DVM_LaunchTicket *ticket = NULL;
    if (!dvm_cancel_requested(dvm_cancel_current()) && dvm_provider_info(root, ".", &attributes) == FVM_OK &&
        attributes.directory && !dvm_cancel_requested(dvm_cancel_current()))
    {
        DVM_LaunchSpec prepared = *spec;
        prepared.terminal = root;
        DVM_TerminalChannels channels = {0};
        if (paths)
        {
            channels.provider = root;
            strcpy(channels.receive, paths->receive);
            strcpy(channels.send, paths->send);
            channels.offers = paths->offers;
            prepared.flashvm = &channels;
        }
        if (endpoint)
        {
            /* EN/RU: A queued ticket stays observable after publication.
             * После публикации ticket остаётся доступным для наблюдения. */
            ticket = dvm_launch_service_submit(endpoint, &prepared);
        }
        else
        {
            ticket = dvm_launch_request_create(&prepared, &pending);
        }
    }
    dvm_provider_release(root);
    if (endpoint)
    {
        return ticket;
    }
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        dvm_launch_ticket_free(ticket);
        dvm_launch_request_free(pending);
        return NULL;
    }
    *request = pending;
    return ticket;
}


/* EN/RU: See request.h; ordinary constructor cannot attach FlashVM channels.
 * См. request.h; ordinary constructor не подключает FlashVM channels. */
DVM_LaunchTicket *styxlaunchrequest(StyxClient *terminal, const DVM_LaunchSpec *spec,
    unsigned access, DVM_LaunchRequest **request)
{
    return create(terminal, spec, access, NULL, request, NULL);
}


/* EN/RU: See request.h; missing explicit terminal paths refuse and consume the input ref.
 * См. request.h; отсутствие terminal paths означает refusal и consume input ref. */
DVM_LaunchTicket *styxbidirlaunchrequest(StyxClient *terminal, const DVM_LaunchSpec *spec,
    unsigned access, const StyxTerminalPaths *paths, DVM_LaunchRequest **request)
{
    if (!paths)
    {
        if (request)
        {
            *request = NULL;
        }
        styxclientfree(terminal);
        return NULL;
    }
    return create(terminal, spec, access, paths, request, NULL);
}


// EN/RU: See request.h; the queue owns provider review after publication.
// См. request.h; после публикации право review принадлежит очереди.
DVM_LaunchTicket *styxbidirlaunchsubmit(StyxClient *terminal, const DVM_LaunchSpec *spec,
    unsigned access, const StyxTerminalPaths *paths, DVM_LaunchEndpoint *endpoint)
{
    if (!paths || !endpoint)
    {
        styxclientfree(terminal);
        return NULL;
    }
    return create(terminal, spec, access, paths, NULL, endpoint);
}


/* EN/RU: Apply the host's route before touching the captured remote root.
 * Проверить выданный host маршрут до обращения к удалённому корню. */
DVM_LaunchTicket *styxbidirlaunchrecord(StyxClient *terminal, const DVM_LaunchSpec *spec,
    unsigned access, const StyxTerminalPaths *paths, DVM_LaunchEndpoint *endpoint,
    unsigned long tag, const FVM_SRecord *record)
{
    if (!spec || spec->arguments || spec->argument_bytes || !tag || tag > 0xffffffffUL ||
        !record || record->tag != tag || record->size > DVM_LAUNCH_ARGUMENT_LIMIT ||
        (record->size && !record->payload))
    {
        styxclientfree(terminal);
        return NULL;
    }
    DVM_LaunchSpec submitted = *spec; /* The peer can only select within a granted module. */
    submitted.arguments = record->payload;
    submitted.argument_bytes = record->size;
    if (spec->program_root)
    {
        if (!styxlaunchprogramdecode(record->payload, record->size, &submitted.program))
        {
            styxclientfree(terminal);
            return NULL;
        }
        submitted.arguments = (const unsigned char *)record->payload + STYX_LAUNCH_PROGRAM_HEADER;
        submitted.argument_bytes -= STYX_LAUNCH_PROGRAM_HEADER;
    }
    return styxbidirlaunchsubmit(terminal, &submitted, access, paths, endpoint);
}
