/* Copyright (c) 2026 bhgv
 * EN: Native C caller for DisVM's recursive channel fixture on one bidirectional socket.
 * RU: Нативный C-клиент рекурсивных каналов DisVM на одном двустороннем сокете.
 */
#define _POSIX_C_SOURCE 200809L
#include "styxrpc/channel_client.h"
#include "styxrpc/channel_fid.h"
#include "styxrpc/program.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


static const char module[] = "mounted/channel-lifetime.dis"; /* Exact published Dis module. */
static const struct timespec pause_time = {0, 1000000};         /* One millisecond between confirmed polls. */
static const StyxRpcChannelGrant hub = {1, "(c[c[i]])"};         /* Receives delegated module channels. */
static const StyxRpcChannelGrant leaf = {2, "(i)"};             /* Final integer exchange. */

typedef struct Device
{
    StyxRpcPrograms programs;      /* Owns the two reverse publications. */
    StyxRpcRegistration nodes[20]; /* Stable registrations for the reverse tree. */
    StyxRpcServer *server;          /* Dispatcher-owned after thread start. */
    uint32_t hub_pid;               /* Temporary reverse hub identity. */
    uint32_t leaf_pid;              /* Temporary integer channel identity. */
    unsigned seen;                  /* Release/acquire count of matching module grants. */
    unsigned char received[64];    /* First exact grant frame, immutable after publication. */
    size_t size;                    /* Received frame extent, published by seen. */
    unsigned leaf_stage;            /* Owning dispatcher checks both integer directions. */
} Device;

typedef struct Open
{
    StyxRpcChannelFid fid;        /* Request and immutable completion of this open. */
    unsigned char request[128];   /* One admitted frame. */
    unsigned char reply[128];     /* One encoded completion. */
} Open;


/* EN: The published reverse hub accepts the bound peer's module-channel grants.
 * RU: Встречная шина принимает полномочия каналов модуля подключённого партнёра. */
static int module_policy(const void *context, const StyxRpcChannelRoute *route)
{
    (void)context;
    return route->kind == STYXRPC_ROUTE_MODULE && route->unbuffered &&
        !strcmp(route->channel.shape, "(c[i])");
}

static const StyxRpcChannelPolicy module_grants = {module_policy, NULL}; /* Scope of hub messages. */


/* EN: Admit only the same module address twice, then return the original bytes to Dis.
 * RU: Принять один адрес модуля дважды, затем вернуть Dis исходные байты. */
static int hub_operation(void *context, StyxRpcChannelFid *fid)
{
    Device *device = context;
    if (fid->selection.count != 1)
    {
        return 0;
    }
    const StyxRpcGuard *guard = fid->selection.guards;
    unsigned seen = __atomic_load_n(&device->seen, __ATOMIC_RELAXED);
    if (guard->receive)
    {
        return seen == 2 &&
            styxrpc_channel_fid_finish(fid, 1, device->received, device->size) == STYXRPC_OK;
    }
    if (guard->size > sizeof(device->received) || seen >= 2 ||
        (seen && (guard->size != device->size || memcmp(guard->value, device->received, device->size))) ||
        styxrpc_channel_fid_finish(fid, 1, NULL, 0) != STYXRPC_OK)
    {
        return 0;
    }
    if (!seen)
    {
        memcpy(device->received, guard->value, guard->size);
        device->size = guard->size;
    }
    __atomic_store_n(&device->seen, seen + 1, __ATOMIC_RELEASE);
    return 1;
}


/* EN: Accept the Dis worker's 43, then deliver 81 on its original integer channel.
 * RU: Принять 43 от потока Dis, затем передать 81 по исходному каналу int. */
static int leaf_operation(void *context, StyxRpcChannelFid *fid)
{
    Device *device = context;
    const StyxRpcGuard *guard = fid->selection.guards;
    unsigned char value[24];
    size_t size;
    if (fid->selection.count != 1 || device->leaf_stage >= 2 ||
        guard->receive != (int)device->leaf_stage ||
        styxrpc_value_integer(STYXRPC_VALUE_INT, guard->receive ? 81 : 43, value, sizeof(value), &size) != STYXRPC_OK ||
        (!guard->receive && (guard->size != size || memcmp(guard->value, value, size))) ||
        styxrpc_channel_fid_finish(fid, 1, guard->receive ? value : NULL, guard->receive ? size : 0) != STYXRPC_OK)
    {
        return 0;
    }
    device->leaf_stage++;
    return 1;
}


/* EN: Allocate operation buffers for one exact reverse bus and read/write fid.
 * RU: Выделить буферы операции для точной встречной шины и одного fid чтения и записи. */
static int open_channel(void *context, unsigned access, void **cookie, const StyxRpcChannelGrant *grant)
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
    opened->fid = (StyxRpcChannelFid){.grants = grant, .grant_count = 1,
        .request = opened->request, .request_capacity = sizeof(opened->request),
        .reply = opened->reply, .reply_capacity = sizeof(opened->reply), .context = context,
        .admit = grant == &hub ? hub_operation : leaf_operation, .policy = grant == &hub ? &module_grants : NULL};
    *cookie = opened;
    return 1;
}


/* EN: Open the hub for module-grant requests.
 * RU: Открыть шину для запросов с полномочиями модуля. */
static int open_hub(void *context, unsigned access, void **cookie)
{
    return open_channel(context, access, cookie, &hub);
}


/* EN: Open the independent integer endpoint.
 * RU: Открыть независимый канал int. */
static int open_leaf(void *context, unsigned access, void **cookie)
{
    return open_channel(context, access, cookie, &leaf);
}


/* EN: The reverse bus returns results starting immediately after the written request.
 * RU: Встречная шина возвращает результат сразу после записанного запроса. */
static int read_bus(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    Open *opened = cookie;
    return offset >= opened->fid.staged && offset - opened->fid.staged <= SIZE_MAX &&
        styxrpc_channel_fid_read(&opened->fid, (size_t)(offset - opened->fid.staged),
            bytes, capacity, size) != STYXRPC_INVALID;
}


/* EN: Confirm the next contiguous prefix of one guarded request.
 * RU: Подтвердить следующий непрерывный фрагмент запроса. */
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


/* EN: Close the exact operation before freeing its storage.
 * RU: Закрыть точную операцию перед освобождением её памяти. */
static void close_bus(void *cookie)
{
    Open *opened = cookie;
    styxrpc_channel_fid_close(&opened->fid);
    free(opened);
}


/* EN: Serve the reverse publications until the connected DisVM closes its session.
 * RU: Обслуживать встречные публикации до закрытия сессии подключённой DisVM. */
static void *dispatch(void *context)
{
    Device *device = context;
    while (styxrpc_server_step(device->server))
    {
    }
    styxrpc_server_destroy(device->server);
    return NULL;
}


/* EN: Register the real call PID and both independent reverse capabilities.
 * RU: Зарегистрировать PID вызова и два независимых встречных полномочия. */
static int register_reverse(Device *device)
{
    const StyxRpcResource outer = {device, open_hub, read_bus, write_bus, close_bus};
    const StyxRpcResource inner = {device, open_leaf, read_bus, write_bus, close_bus};
    return styxrpc_programs_init(&device->programs, device->nodes, sizeof(device->nodes) / sizeof(*device->nodes)) &&
        styxrpc_program_add(&device->programs, 42) &&
        (device->hub_pid = styxrpc_program_unibus_grant(&device->programs, &outer)) &&
        (device->leaf_pid = styxrpc_program_unibus_grant(&device->programs, &inner));
}


/* EN: Authorize only the exact reverse address, key and rendezvous bit chosen for send.
 * RU: Разрешить только выбранный для отправки встречный адрес, ключ и признак rendezvous. */
static int same_grant(const void *context, const StyxRpcChannelRoute *route)
{
    const StyxRpcChannelRoute *expected = context;
    return route->kind == expected->kind && route->pid == expected->pid &&
        route->channel.key == expected->channel.key && route->unbuffered == expected->unbuffered &&
        !strcmp(route->channel.shape, expected->channel.shape);
}


/* EN: Send one selected reverse grant to a module capability, authorizing only this address.
 * RU: Передать выбранное встречное полномочие в канал модуля, разрешив только этот адрес. */
static int send_grant(StyxClient *client, const char *bus, const StyxRpcChannelRoute *target,
    const StyxRpcChannelRoute *offered)
{
    unsigned char child[32], value[48], request[96], reply[96];
    size_t length, size;
    if (styxrpc_channel_route_value(offered, child, sizeof(child), &length) != STYXRPC_OK ||
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, child, length, value, sizeof(value), &size) != FVM_S_OK)
    {
        return 0;
    }
    const StyxRpcChannelPolicy policy = {same_grant, offered};
    const StyxRpcSelection selection = {.count = 1, .guards = {{target->channel.key, 0, value, size}}, .policy = &policy};
    StyxRpcChannelResult outcome;
    return styxrpc_channel_route_exchange(client, bus, target, &selection, 2000, NULL, 0,
        request, sizeof(request), reply, sizeof(reply), &outcome) == STYXRPC_OK && outcome.selected == 1;
}


/* EN: Receive on the exact module route and distinguish refusal from timeout.
 * RU: Принять по точному маршруту модуля и отличить отказ от истечения срока. */
static StyxRpcStatus receive_one(StyxClient *client, const char *bus, const StyxRpcChannelRoute *target,
    StyxRpcChannelResult *outcome)
{
    unsigned char request[96], reply[96];
    const StyxRpcSelection selection = {.count = 1, .guards = {{target->channel.key, 1, NULL, 0}}};
    return styxrpc_channel_route_exchange(client, bus, target, &selection, 2000, NULL, 0,
        request, sizeof(request), reply, sizeof(reply), outcome);
}


/* EN: Poll the borrowed relay marker on the calling thread. With NULL, stop
 * immediately. The relay writes '1' only after DisVM acknowledged the token.
 * RU: Проверить заимствованный маркер в вызывающем потоке. При NULL отменить
 * сразу. Ретранслятор пишет '1' после подтверждения токена от DisVM. */
static int stop_import(void *state)
{
    const char *marker = state;
    if (!marker)
    {
        return 1;
    }
    FILE *file = fopen(marker, "rb");
    if (!file)
    {
        return 0;
    }
    int first = fgetc(file);
    fclose(file);
    return first == '1';
}


/* EN: Import a batch of two different tokens after proving cancellation leaves
 * the two original indices and no index for child. The fixture owns marker.
 * RU: Принять два разных токена после проверки, что отмена сохраняет два
 * исходных индекса без добавления child. Маркер принадлежит тесту. */
static int admit_delegated(StyxRpcChannelLease *lease, const StyxRpcChannelRoute *child,
    const StyxRpcChannelRoute *base, char *marker)
{
    static const char shape[] = "((c[c[i]]c[i]))"; /* Tuple with new channel and existing integer alias. */
    unsigned char leaves[96], tuple[112], frame[128];
    size_t first, second, extent, size;
    if (!memcmp(child->token, base->token, STYXRPC_CHANNEL_TOKEN_SIZE) ||
        styxrpc_channel_route_value(child, leaves, sizeof(leaves), &first) != STYXRPC_OK ||
        styxrpc_channel_route_value(base, leaves + first, sizeof(leaves) - first, &second) != STYXRPC_OK ||
        fvm_srecord_write(STYXRPC_VALUE_TUPLE, leaves, first + second, tuple, sizeof(tuple), &extent) != FVM_S_OK ||
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, tuple, extent, frame, sizeof(frame), &size) != FVM_S_OK)
    {
        return 0;
    }
    if (marker)
    {
        FILE *arm = fopen("arm-import", "wb");
        if (!arm || fclose(arm) != 0)
        {
            return 0;
        }
    }
    if (styxrpc_channel_lease_import_message_until(lease, shape, frame, size, stop_import, marker) != STYXRPC_CANCELLED ||
        styxrpc_channel_lease_route(lease, 2))
    {
        return 0;
    }
    const StyxRpcChannelRoute *existing = styxrpc_channel_lease_route(lease, 1);
    if (!existing || memcmp(existing->token, base->token, STYXRPC_CHANNEL_TOKEN_SIZE) ||
        existing->channel.key != base->channel.key || strcmp(existing->channel.shape, base->channel.shape) ||
        styxrpc_channel_lease_import_message_until(lease, shape, frame, size, NULL, NULL) != STYXRPC_OK)
    {
        return 0;
    }
    const StyxRpcChannelRoute *imported = styxrpc_channel_lease_route(lease, 2);
    return imported && !memcmp(imported->token, child->token, STYXRPC_CHANNEL_TOKEN_SIZE) &&
        imported->channel.key == child->channel.key && !strcmp(imported->channel.shape, child->channel.shape);
}


/* EN: Exchange a reverse grant, echo the stable module alias, use it, and check clunk revocation.
 * RU: Передать встречное полномочие, вернуть алиас модуля, использовать и проверить отзыв по clunk. */
static int exercise(StyxClient *client, Device *device, char *marker)
{
    char bus[sizeof(module) + sizeof("/.unibus")];
    snprintf(bus, sizeof(bus), "%s/.unibus", module);
    const StyxRpcManagedTarget call = {
        client, module, "open", UINT32_C(0x1303311e), {"()", "((c[c[c[c[i]]]]c[i]))"}, 42, 2000
    }; /* Exact Limbo offer and the reverse PID published by register_reverse. */
    unsigned char reply[128]; /* Borrowed by accepted.result while inspecting the returned channels. */
    StyxRpcLeasedReply accepted = {0}; /* Owns the forward function and exact module on success. */
    StyxRpcChannelRoutes delegated; /* Holds the child's independently decoded route. */
    const StyxRpcChannelRoute outer = {.kind = STYXRPC_ROUTE_PID, .pid = device->hub_pid, .channel = hub, .unbuffered = 1};
    const StyxRpcChannelRoute inner = {.kind = STYXRPC_ROUTE_PID, .pid = device->leaf_pid, .channel = leaf, .unbuffered = 1};
    int step = 1;
    int valid = styxrpc_channel_invoke(&call, NULL, 0, reply, sizeof(reply), &accepted) == STYXRPC_CALL_OK &&
        accepted.routes.count == 2 &&
        !memcmp(accepted.routes.entries[0].token, accepted.routes.entries[1].token, STYXRPC_CHANNEL_TOKEN_SIZE);
    if (valid)
    {
        step = 2;
        valid = send_grant(client, bus, &accepted.routes.entries[0], &outer);
    }
    if (valid)
    {
        step = 3;
        for (unsigned attempt = 0; attempt < 2000 && __atomic_load_n(&device->seen, __ATOMIC_ACQUIRE) < 2; ++attempt)
        {
            nanosleep(&pause_time, NULL);
        }
        valid = __atomic_load_n(&device->seen, __ATOMIC_ACQUIRE) == 2 &&
            styxrpc_channel_element_routes(hub.shape, device->received, device->size, &delegated) == STYXRPC_OK &&
            delegated.count == 1 && admit_delegated(accepted.lease, &delegated.entries[0], &accepted.routes.entries[1], marker) &&
            send_grant(client, bus, styxrpc_channel_lease_route(accepted.lease, 2), &inner);
    }
    StyxRpcChannelResult result;
    if (valid)
    {
        step = 4;
        StyxRpcChannelRoute wrong = accepted.routes.entries[1];
        wrong.token[0] ^= 0x80;
        StyxRpcStatus denied = receive_one(client, bus, &wrong, &result);
        valid = denied == STYXRPC_INVALID || denied == STYXRPC_REMOTE_ERROR;
    }
    if (valid)
    {
        step = 5;
        const StyxRpcSelection selection = {
            .count = 1, .guards = {{accepted.routes.entries[1].channel.key, 1, NULL, 0}}
        }; /* Receive through the captured module, preserving the caller's exact token. */
        unsigned char request[96]; /* Single guard, so no ALT scratch is required. */
        valid = styxrpc_channel_lease_exchange(accepted.lease, 1, &selection, 2000, NULL, 0,
            request, sizeof(request), reply, sizeof(reply), &result) == STYXRPC_OK &&
            result.selected == 1 && result.integer == 81;
    }
    styxrpc_channel_lease_release(accepted.lease, 0); /* Revoke before the negative checks. */
    if (valid)
    {
        step = 6;
        StyxRpcStatus revoked = receive_one(client, bus, &accepted.routes.entries[1], &result);
        StyxRpcStatus child = receive_one(client, bus, &delegated.entries[0], &result);
        valid = (revoked == STYXRPC_INVALID || revoked == STYXRPC_REMOTE_ERROR) &&
            (child == STYXRPC_INVALID || child == STYXRPC_REMOTE_ERROR);
    }
    if (!valid)
    {
        fprintf(stderr, "recursive C/Dis grant exchange failed at stage %d\n", step);
    }
    return valid;
}


/* EN: Split an already connected fd, run the reverse dispatcher and own both shutdown paths.
 * RU: Разделить подключённый fd, запустить встречный диспетчер и завершить оба направления. */
int main(int argc, char **argv)
{
    char *end;
    long fd = argc == 2 || argc == 3 ? strtol(argv[1], &end, 10) : -1;
    char *marker = argc == 3 ? argv[2] : NULL;
    Device device = {0};
    if (fd < 3 || fd > INT_MAX || *end || !register_reverse(&device))
    {
        return 2;
    }
    PhyBidir *mux = phybidircreate(phyposixfd((int)fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    if (!mux)
    {
        return 1;
    }
    device.server = styxrpc_server_create(phybidirchannel(mux, 1), &styxrpc_program_tree, &device.programs);
    pthread_t owner;
    if (!device.server || pthread_create(&owner, NULL, dispatch, &device))
    {
        styxrpc_server_destroy(device.server);
        phybidirfree(mux);
        return 1;
    }
    StyxClient *client = styxclientmountphy(phybidirchannel(mux, 0), 4096, "caller", "", 1000, 8);
    int valid = client && exercise(client, &device, marker);
    styxclientshutdown(client);
    styxclientfree(client);
    phybidirfree(mux);
    pthread_join(owner, NULL);
    if (!valid || device.leaf_stage != 2)
    {
        return 1;
    }
    puts("PASS delegated PID and module channels between C and DisVM");
    return 0;
}
