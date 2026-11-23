/* Copyright (c) 2026 bhgv
 * EN: Check exact reuse and cancellation of channel argument scopes without a VM.
 * RU: Проверка повторного использования и отмены областей каналов без VM.
 */
#include "styxrpc/channel_arguments.h"
#include "styxrpc/channel_route.h"
#include <stdio.h>
#include <string.h>

#define TEST_PID UINT32_C(42) /* Caller PID registered for the borrowed reverse resource. */
#define TEST_BUFFER_BYTES 128U /* Space for one small framed request or reply. */


/* EN: Stage one queue and decode its assigned current-route identity through the public codec.
 * RU: Подготовить очередь и декодировать её ключ текущего маршрута открытым кодеком. */
static int stage(StyxRpcArgumentScope *scope, StyxRpcChannelQueue *queue, StyxRpcChannelGrant *grant)
{
    unsigned char first[TEST_BUFFER_BYTES], alias[TEST_BUFFER_BYTES], frame[TEST_BUFFER_BYTES];
    size_t size, extent;
    StyxRpcChannelRoutes routes;
    if (styxrpc_argument_scope_value(scope, queue, first, sizeof(first), &size) != STYXRPC_OK ||
        styxrpc_argument_scope_value(scope, queue, alias, sizeof(alias), &extent) != STYXRPC_OK ||
        extent != size || memcmp(first, alias, size) ||
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, first, size, frame, sizeof(frame), &extent) != FVM_S_OK ||
        styxrpc_channel_element_routes("(c[i])", frame, extent, &routes) != STYXRPC_OK || routes.count != 1 ||
        routes.entries[0].kind != STYXRPC_ROUTE_CURRENT)
    {
        return 0;
    }
    *grant = routes.entries[0].channel;
    return 1;
}


/* EN: Issue a valid single guard, preserving its exact request count for reverse reads.
 * RU: Выполнить допустимую ветку, запомнив длину запроса для встречного чтения. */
static int submit(const StyxRpcResource *bus, void *file, const StyxRpcChannelGrant *grant,
    const StyxRpcSelection *selection, size_t *size)
{
    unsigned char frame[TEST_BUFFER_BYTES];
    size_t n;
    return styxrpc_channel_request(grant, 1, selection, NULL, 0, frame, sizeof(frame), size) == STYXRPC_OK &&
        bus->write(file, 0, frame, *size, &n) && n == *size;
}


/* EN: Verify a queued cancellation and new-generation isolation, keeping an old fid open.
 * RU: Проверить отмену и изоляцию нового поколения, оставив старый fid открытым. */
static int exercise(StyxRpcArgumentBus *owner)
{
    const StyxRpcResource bus = styxrpc_argument_bus_resource(owner);
    const unsigned access = STYXRPC_READ | STYXRPC_WRITE;
    unsigned char arena[TEST_BUFFER_BYTES], reply[TEST_BUFFER_BYTES], integer[TEST_BUFFER_BYTES];
    size_t sizes[1], request_size, used, value_size;
    StyxRpcChannelQueue queue = {.grant = {1, "(i)"}, .bytes = arena, .sizes = sizes,
        .capacity = 1, .stride = sizeof(arena)};
    StyxRpcChannelGrant first, second;
    StyxRpcChannelResult result;
    StyxRpcArgumentScope *scope = styxrpc_argument_scope_open(owner, TEST_PID);
    void *old = NULL, *stale = NULL, *fresh = NULL;
    int valid = scope && !bus.open(bus.context, access, &old) &&
        stage(scope, &queue, &first) && styxrpc_argument_scope_activate(scope) && bus.open(bus.context, access, &old);
    if (valid)
    {
        const StyxRpcSelection receive = {.count = 1, .guards = {{first.key, 1, NULL, 0}}};
        valid = submit(&bus, old, &first, &receive, &request_size) &&
            bus.read(old, request_size, reply, sizeof(reply), &used) && !used;
    }
    styxrpc_argument_scope_close(scope);
    scope = NULL;
    if (valid)
    {
        scope = styxrpc_argument_scope_open(owner, TEST_PID);
        valid = !bus.read(old, request_size, reply, sizeof(reply), &used) && scope &&
            stage(scope, &queue, &second) && first.key != second.key && styxrpc_argument_scope_activate(scope) &&
            bus.open(bus.context, access, &stale) && bus.open(bus.context, access, &fresh);
    }
    if (valid)
    {
        const StyxRpcSelection dead = {.count = 1, .guards = {{first.key, 1, NULL, 0}}};
        valid = !submit(&bus, stale, &first, &dead, &request_size) &&
            styxrpc_value_integer(STYXRPC_VALUE_INT, -42, integer, sizeof(integer), &value_size) == STYXRPC_OK;
    }
    if (valid)
    {
        const StyxRpcSelection send = {.count = 1, .guards = {{second.key, 0, integer, value_size}}};
        valid = submit(&bus, fresh, &second, &send, &request_size) &&
            bus.read(fresh, request_size, reply, sizeof(reply), &used) && used &&
            styxrpc_channel_result(&second, 1, &send, reply, used, &result) == STYXRPC_OK && result.selected == 1;
    }
    styxrpc_argument_scope_close(scope);
    if (old)
    {
        bus.close(old);
    }
    if (stale)
    {
        bus.close(stale);
    }
    if (fresh)
    {
        bus.close(fresh);
    }
    int64_t stored;
    return valid && queue.count == 1 &&
        styxrpc_value_get_integer(STYXRPC_VALUE_INT, arena, sizes[0], &stored) == STYXRPC_OK && stored == -42;
}


/* EN: Encode a child as a typed message using the local or staged key.
 * RU: Закодировать дочерний канал как типизированное сообщение по одному ключу. */
static int seed_child(uint32_t key, unsigned char *arena, size_t *size)
{
    StyxRpcChannelRoute route = {.kind = STYXRPC_ROUTE_CURRENT, .channel = {key, "(i)"}};
    unsigned char leaf[TEST_BUFFER_BYTES];
    size_t extent;
    return styxrpc_channel_route_value(&route, leaf, sizeof(leaf), &extent) == STYXRPC_OK &&
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, leaf, extent, arena, TEST_BUFFER_BYTES, size) == FVM_S_OK;
}


/* EN: Admit a nested-only queue, revoke it and reject the identical old key in the next scope.
 * RU: Допустить вложенную очередь, отозвать её и отклонить старый ключ в следующей области. */
static int nested_scope(StyxRpcArgumentBus *owner)
{
    const StyxRpcResource bus = styxrpc_argument_bus_resource(owner);
    const unsigned access = STYXRPC_READ | STYXRPC_WRITE;
    const uint32_t local_key = INT32_MAX;
    unsigned char arena[TEST_BUFFER_BYTES], staged[TEST_BUFFER_BYTES], reply[TEST_BUFFER_BYTES];
    size_t size, staged_size, length, used;
    StyxRpcChannelQueue child = {.grant = {1, "(i)"}};
    StyxRpcChannelQueue parent = {.grant = {1, "(c[i])"}, .bytes = staged, .sizes = &staged_size,
        .capacity = 1, .stride = sizeof(staged), .count = 1};
    StyxRpcChannelGrant outer, before, after;
    StyxRpcArgumentScope *scope = styxrpc_argument_scope_open(owner, TEST_PID);
    void *pending = NULL, *stale = NULL;
    int ok = scope && stage(scope, &parent, &outer) &&
        styxrpc_argument_scope_nested(scope, &child, &before) == STYXRPC_OK &&
        seed_child(local_key, arena, &size) && seed_child(local_key, staged, &staged_size) &&
        !styxrpc_argument_scope_activate(scope);
    const StyxRpcChannelKeyMap keys = {local_key, ok ? before.key : 0};
    if (ok)
    {
        ok = styxrpc_channel_element_remap(parent.grant.shape, arena, size, &keys, 1,
            staged, sizeof(staged), &staged_size) == STYXRPC_OK && styxrpc_argument_scope_activate(scope) &&
            bus.open(bus.context, access, &pending);
    }
    if (ok)
    {
        const StyxRpcSelection receive = {.count = 1, .guards = {{before.key, 1, NULL, 0}}};
        ok = submit(&bus, pending, &before, &receive, &length) &&
            bus.read(pending, length, reply, sizeof(reply), &used) && !used;
    }
    styxrpc_argument_scope_close(scope);
    scope = NULL;
    if (ok)
    {
        scope = styxrpc_argument_scope_open(owner, TEST_PID);
        ok = !bus.read(pending, length, reply, sizeof(reply), &used) && scope &&
            styxrpc_argument_scope_nested(scope, &child, &after) == STYXRPC_OK &&
            before.key != after.key && styxrpc_argument_scope_activate(scope) &&
            bus.open(bus.context, access, &stale);
    }
    if (ok)
    {
        const StyxRpcSelection dead = {.count = 1, .guards = {{before.key, 1, NULL, 0}}};
        ok = !submit(&bus, stale, &before, &dead, &length);
    }
    styxrpc_argument_scope_close(scope);
    if (pending)
    {
        bus.close(pending);
    }
    if (stale)
    {
        bus.close(stale);
    }
    return ok;
}


/* EN: Validate the standalone reverse owner without an application runtime.
 * RU: Проверить независимую встречную шину без исполняющей среды приложения. */
int main(void)
{
    StyxRpcArgumentBus *owner = styxrpc_argument_bus_create(TEST_PID);
    int valid = owner && !styxrpc_argument_scope_open(owner, TEST_PID + 1) && exercise(owner) && nested_scope(owner);
    styxrpc_argument_bus_destroy(owner);
    if (!valid)
    {
        fputs("channel argument scope lifetime failed\n", stderr);
    }
    return valid ? 0 : 1;
}
