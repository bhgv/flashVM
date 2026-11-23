/* Copyright (c) 2026 bhgv
 * EN: Exchange typed UniBus selection on registered process fids over Styx.
 * RU: Обмен типизированным выбором UniBus через fid процесса в Styx.
 */
#include "styxrpc/channel_fid.h"
#include "styxrpc/channel_route.h"
#include "styxrpc/program.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include <pthread.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define REVOKE_ROUNDS 132U /* Reuse the same three slots across one live session. */

static const StyxRpcChannelGrant grants[] = {{1, "(i)"}, {2, "(s)"}}; /* Explicit local capabilities. */


typedef struct Endpoint
{
    StyxRpcServer *server; /* Owned by server thread after pthread_create. */
    StyxRpcPrograms *programs; /* Reverse tree owned by the same dispatcher. */
    const StyxRpcResource *stream; /* Borrowed callbacks for each issued route. */
    uint32_t pid;                 /* Current grant, mutated on the dispatcher. */
    unsigned cancelled;    /* Checked only after joining dispatcher. */
} Endpoint;

typedef struct Open
{
    StyxRpcChannelFid fid;     /* Portable per-fid state. */
    unsigned char request[128]; /* One operation. */
    unsigned char reply[128];   /* One completion. */
    int stream;                /* Reply begins after the request for a routed PID. */
} Open;


/* EN: Complete a TRY with a ready string; retain the blocking receive until cancellation.
 * RU: Завершить TRY готовой строкой; блокирующий приём оставить до отмены. */
static int schedule(void *context, StyxRpcChannelFid *fid)
{
    Endpoint *endpoint = context;
    if (fid->selection.mode == STYXRPC_CHANNEL_TRY && fid->selection.count == 1 &&
        fid->selection.guards[0].key == 1 && fid->selection.guards[0].receive)
    {
        unsigned before = endpoint->cancelled;
        uint32_t retired = endpoint->pid;
        if (!styxrpc_program_unibus_revoke(endpoint->programs, retired) ||
            endpoint->cancelled != before + 2 || styxrpc_program_unibus_revoke(endpoint->programs, retired))
        {
            return 0;
        }
        endpoint->pid = styxrpc_program_unibus_grant(endpoint->programs, endpoint->stream);
        return endpoint->pid == retired + 1 && styxrpc_channel_fid_finish(fid, 0, NULL, 0) == STYXRPC_OK;
    }
    unsigned selected;
    if (fid->selection.count == 1 && fid->selection.guards[0].key == 2 && fid->selection.guards[0].receive)
    {
        selected = 1;
    }
    else if (fid->selection.mode == STYXRPC_CHANNEL_TRY && fid->selection.count == 2 &&
        fid->selection.guards[1].key == 2 && fid->selection.guards[1].receive)
    {
        selected = 2;
    }
    else
    {
        return fid->selection.mode != STYXRPC_CHANNEL_TRY;
    }
    unsigned char field[16], value[24];
    size_t length, used;
    return fvm_srecord_write(STYXRPC_VALUE_STRING, "ready", 5, field, sizeof(field), &length) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, field, length, value, sizeof(value), &used) == FVM_S_OK &&
        styxrpc_channel_fid_finish(fid, selected, value, used) == STYXRPC_OK;
}


/* EN: Confirm exactly one unresolved admission is retired.
 * RU: Зафиксировать завершение ровно одной ожидающей операции. */
static void cancel(void *context, StyxRpcChannelFid *fid)
{
    Endpoint *endpoint = context;
    if (fid->selection.count == 1)
    {
        endpoint->cancelled++;
    }
}


/* EN: Install an independently owned channel operation per read/write fid.
 * RU: Создать независимую операцию канала на каждом fid чтения и записи. */
static int open_bus(void *context, unsigned access, void **cookie)
{
    if (access != (STYXRPC_READ | STYXRPC_WRITE))
    {
        return 0;
    }
    Open *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        return 0;
    }
    opened->fid = (StyxRpcChannelFid){.grants = grants, .grant_count = 2,
        .request = opened->request, .request_capacity = sizeof(opened->request),
        .reply = opened->reply, .reply_capacity = sizeof(opened->reply),
        .admit = schedule, .cancel = cancel, .context = context};
    *cookie = opened;
    return 1;
}


/* EN: Open a routed PID using its stream reply offset.
 * RU: Открыть маршрут PID со смещением ответа в потоке. */
static int open_stream(void *context, unsigned access, void **cookie)
{
    if (!open_bus(context, access, cookie))
    {
        return 0;
    }
    Open *opened = *cookie;
    opened->stream = 1;
    return 1;
}


/* EN: Return a completed reply or an empty pending prefix on the selected fid.
 * RU: Вернуть полный ответ либо пустой префикс ожидания на выбранном fid. */
static int read_bus(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    Open *opened = cookie;
    uint64_t base = opened->stream ? opened->fid.staged : 0;
    return offset >= base && offset - base <= SIZE_MAX && styxrpc_channel_fid_read(&opened->fid,
        (size_t)(offset - base), bytes, capacity, size) != STYXRPC_INVALID;
}


/* EN: Acknowledge each accepted prefix once, even when the request is fragmented.
 * RU: Подтвердить каждый принятый фрагмент один раз, включая разбитый запрос. */
static int write_bus(void *cookie, uint64_t offset, const void *bytes, size_t size, size_t *written)
{
    Open *opened = cookie;
    if (offset > SIZE_MAX || styxrpc_channel_fid_write(&opened->fid, (size_t)offset, bytes, size) == STYXRPC_INVALID)
    {
        return 0;
    }
    *written = size;
    return 1;
}


/* EN: Retire the exact operation before returning its native buffers.
 * RU: Закрыть точную операцию до освобождения нативных буферов. */
static void close_bus(void *cookie)
{
    Open *opened = cookie;
    styxrpc_channel_fid_close(&opened->fid);
    free(opened);
}


/* EN: Serve an owned reverse endpoint until the client disconnects.
 * RU: Обслуживать встречный endpoint до отключения клиента. */
static void *dispatch(void *context)
{
    Endpoint *endpoint = context;
    while (styxrpc_server_step(endpoint->server))
    {
    }
    styxrpc_server_destroy(endpoint->server);
    return NULL;
}


/* EN: Submit a delayed guard and an immediate mixed ALT on separate fids.
 * RU: Подать ожидающую ветку и немедленный смешанный ALT на разных fid. */
static int exchange(StyxClient *client)
{
    StyxFid *park = styxclientwalk(client, ".prog/42/.unibus");
    StyxFid *ready = styxclientwalk(client, ".prog/42/.unibus");
    const StyxRpcSelection receive = {.count = 1, .guards = {{1, 1, NULL, 0}}};
    const StyxRpcSelection selection = {.mode = STYXRPC_CHANNEL_TRY, .count = 2,
        .guards = {{1, 1, NULL, 0}, {2, 1, NULL, 0}}};
    unsigned char request[128], scratch[96], reply[96];
    size_t length;
    StyxTransfer sent, prefix, received;
    StyxRpcChannelResult result;
    int good = park && ready && styxclientopen(park, 2) && styxclientopen(ready, 2) &&
        styxrpc_channel_request(grants, 2, &receive, NULL, 0,
            request, sizeof(request), &length) == STYXRPC_OK;
    if (good)
    {
        styxclientwrite(park, 0, request, length, &sent);
        styxclientread(park, 0, reply, sizeof(reply), &prefix);
        good = sent.success && sent.count == length && prefix.success && !prefix.count &&
            styxrpc_channel_request(grants, 2, &selection, scratch, sizeof(scratch),
                request, sizeof(request), &length) == STYXRPC_OK;
    }
    if (good)
    {
        styxclientwrite(ready, 0, request, 1, &prefix);
        styxclientwrite(ready, 1, request + 1, length - 1, &sent);
        styxclientread(ready, 0, reply, sizeof(reply), &received);
        good = prefix.success && prefix.count == 1 && sent.success && sent.count == length - 1 &&
            received.success && styxrpc_channel_result(grants, 2, &selection, reply, received.count,
                &result) == STYXRPC_OK && result.selected == 2 && result.size;
    }
    styxclientclunk(park);
    styxclientclunk(ready);
    return good;
}


/* EN: Follow the published PID and reject an otherwise identical unpublished route.
 * RU: Открыть выданный PID и отклонить такой же адрес без публикации. */
static int exchange_routed(StyxClient *client, uint32_t pid)
{
    StyxRpcChannelRoute route = {.kind = STYXRPC_ROUTE_PID, .pid = pid, .channel = grants[1]};
    unsigned char request[96], reply[96];
    const StyxRpcSelection receive = {.count = 1, .guards = {{2, 1, NULL, 0}}};
    StyxRpcChannelResult result;
    if (styxrpc_channel_route_exchange(client, NULL, &route, &receive, 500, NULL, 0,
            request, sizeof(request), reply, sizeof(reply), &result) != STYXRPC_OK ||
        result.selected != 1 || !result.size)
    {
        return 0;
    }
    route.pid++;
    return styxrpc_channel_route_exchange(client, NULL, &route, &receive, 500, NULL, 0,
        request, sizeof(request), reply, sizeof(reply), &result) == STYXRPC_INVALID;
}


/* EN: Admit a live PID and refuse its replacement address without changing routes.
 * RU: Принять живой PID и отклонить другой адрес без изменения routes. */
static int import_pid(StyxClient *client, uint32_t pid, StyxRpcStatus expected, StyxRpcChannelRoutes *routes)
{
    const StyxRpcChannelRoute route = {.kind = STYXRPC_ROUTE_PID, .pid = pid, .channel = grants[1]};
    unsigned char child[96], frame[96];
    size_t leaf, extent;
    return styxrpc_channel_route_value(&route, child, sizeof(child), &leaf) == STYXRPC_OK &&
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, child, leaf, frame, sizeof(frame), &extent) == FVM_S_OK &&
        styxrpc_channel_pid_import(client, "(c[s])", frame, extent, routes) == expected;
}


/* EN: Park two operations, revoke through the owner and reject old and fresh access.
 * RU: Поставить две операции в ожидание, отозвать через владельца и проверить отказ доступа. */
static int revoke_routed(StyxClient *client, uint32_t pid)
{
    char directory[sizeof(".prog/4294967295")];
    char path[sizeof(".prog/4294967295/.unibus")];
    snprintf(directory, sizeof(directory), ".prog/%" PRIu32, pid);
    snprintf(path, sizeof(path), "%s/.unibus", directory);
    StyxFid *old_directory = styxclientwalk(client, directory);
    StyxFid *first = styxclientwalk(client, path);
    StyxFid *second = styxclientwalk(client, path);
    StyxFid *walked = styxclientwalk(client, path);
    StyxFid *control = styxclientwalk(client, ".prog/42/.unibus");
    const StyxRpcSelection park = {.count = 1, .guards = {{1, 1, NULL, 0}}};
    const StyxRpcSelection revoke = {.mode = STYXRPC_CHANNEL_TRY, .count = 1, .guards = {{1, 1, NULL, 0}}};
    unsigned char scratch[96], request[96], reply[96];
    size_t length;
    StyxTransfer sent, received;
    int good = old_directory && first && second && walked && control && styxclientopen(first, 2) &&
        styxclientopen(second, 2) && styxclientopen(control, 2) &&
        styxrpc_channel_request(grants, 2, &park, NULL, 0, request, sizeof(request), &length) == STYXRPC_OK;
    if (good)
    {
        styxclientwrite(first, 0, request, length, &sent);
        styxclientwrite(second, 0, request, length, &received);
        good = sent.success && sent.count == length && received.success && received.count == length &&
            styxrpc_channel_request(grants, 2, &revoke, scratch, sizeof(scratch),
                request, sizeof(request), &length) == STYXRPC_OK;
    }
    if (good)
    {
        styxclientwrite(control, 0, request, length, &sent);
        styxclientread(first, 0, reply, sizeof(reply), &received);
        good = sent.success && sent.count == length && !received.success && !styxclientopen(walked, 2);
        styxclientwrite(second, 0, request, length, &sent);
        StyxFid *fresh = styxclientwalk(client, path);
        StyxFid *from_old = styxclientwalkfrom(old_directory, ".unibus");
        good = good && !sent.success && !fresh && !from_old;
        styxclientclunk(from_old);
        styxclientclunk(fresh);
    }
    styxclientclunk(old_directory);
    styxclientclunk(walked);
    styxclientclunk(second);
    styxclientclunk(first);
    styxclientclunk(control);
    return good;
}


/* EN: Run the exchange on a connected socket and verify that clunk cancelled the pending fid.
 * RU: Выполнить обмен по сокету и проверить, что clunk отменил ожидающий fid. */
int main(void)
{
    StyxRpcPrograms programs;
    Endpoint endpoint = {.programs = &programs};
    StyxRpcRegistration nodes[11];
    const StyxRpcResource resource = {&endpoint, open_bus, read_bus, write_bus, close_bus};
    const StyxRpcResource stream = {&endpoint, open_stream, read_bus, write_bus, close_bus};
    int sockets[2];
    if (!styxrpc_programs_init(&programs, nodes, sizeof(nodes) / sizeof(*nodes)) || !styxrpc_program_add(&programs, 42) ||
        !styxrpc_program_unibus(&programs, 42, &resource) ||
        !(endpoint.pid = styxrpc_program_unibus_grant(&programs, &stream)) ||
        styxrpc_program_unibus_grant(&programs, &stream) || socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        return 1;
    }
    uint32_t pid = endpoint.pid;
    endpoint.stream = &stream;
    endpoint.server = styxrpc_server_create(phyposixfd(sockets[1], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE),
        &styxrpc_program_tree, &programs);
    pthread_t owner;
    if (!endpoint.server || pthread_create(&owner, NULL, dispatch, &endpoint))
    {
        styxrpc_server_destroy(endpoint.server);
        close(sockets[0]);
        return 1;
    }
    StyxClient *client = styxclientmountfd(sockets[0], 4096, "test", "", 1000, 8);
    StyxRpcChannelRoutes imported = {0};
    int good = client && exchange(client) && import_pid(client, pid, STYXRPC_OK, &imported) &&
        import_pid(client, pid + 1, STYXRPC_INVALID, &imported) && imported.count == 1 &&
        imported.entries[0].pid == pid;
    for (unsigned i = 0; good && i < REVOKE_ROUNDS; ++i)
    {
        good = exchange_routed(client, pid + i) && revoke_routed(client, pid + i);
    }
    good = good && import_pid(client, pid, STYXRPC_INVALID, &imported) && imported.count == 1 &&
        imported.entries[0].pid == pid && exchange_routed(client, pid + REVOKE_ROUNDS) && exchange(client);
    styxclientshutdown(client);
    styxclientfree(client);
    pthread_join(owner, NULL);
    if (!good || endpoint.cancelled != 2 + 2 * REVOKE_ROUNDS)
    {
        fputs("UniBus selection or pending fid retirement failed\n", stderr);
        return 1;
    }
    return 0;
}
