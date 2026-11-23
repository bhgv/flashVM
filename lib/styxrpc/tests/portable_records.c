/* Copyright (c) 2026 bhgv
 * EN: Verify reusable channel records and positional nested reference updates.
 * RU: Проверить переиспользуемые кадры каналов и позиционные изменения вложенных ref.
 */
#include "styxrpc/channel_call.h"
#include "styxrpc/graph.h"
#include "call_record.h"
#include <stdio.h>
#include <string.h>


/* EN: Round trip mixed ALT, typed receives, idle and unauthorized capability refusal.
 * RU: Проверить смешанный ALT, типизированный приём, ожидание и отказ чужого ключа. */
static int channel_round_trip(void)
{
    const StyxRpcChannelGrant grants[] = {{1, "(i)"}, {2, "(s)"}};
    unsigned char child[24], message[32], request[96], reply[96], scratch[80];
    size_t field, extent, request_size, reply_size;
    StyxRpcSelection request_view;
    StyxRpcChannelResult received;
    if (styxrpc_value_integer(STYXRPC_VALUE_INT, -71, child, sizeof(child), &field) != STYXRPC_OK)
    {
        return 0;
    }
    const StyxRpcSelection selection = {.mode = STYXRPC_CHANNEL_TRY, .count = 2, .guards = {
        {1, 0, child, field}, {2, 1, NULL, 0}
    }};
    if (styxrpc_channel_request(grants, 2, &selection, scratch, sizeof(scratch),
            request, sizeof(request), &request_size) != STYXRPC_OK ||
        styxrpc_channel_read(grants, 1, request, request_size, &request_view) != STYXRPC_INVALID ||
        styxrpc_channel_read(grants, 2, request, request_size, &request_view) != STYXRPC_OK ||
        request_view.count != 2 || !request_view.guards[1].receive ||
        fvm_srecord_write(STYXRPC_VALUE_STRING, "hello", 5, child, sizeof(child), &field) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_GRAPH_TAG, child, field, message, sizeof(message), &extent) != FVM_S_OK ||
        styxrpc_channel_reply(grants, 2, &request_view, 2, message, extent,
            reply, sizeof(reply), &reply_size) != STYXRPC_OK ||
        styxrpc_channel_result(grants, 2, &request_view, reply, reply_size, &received) != STYXRPC_OK ||
        received.selected != 2 || received.size != extent ||
        styxrpc_channel_reply(grants, 2, &request_view, 0, NULL, 0,
            reply, sizeof(reply), &reply_size) != STYXRPC_OK ||
        styxrpc_channel_result(grants, 2, &request_view, reply, reply_size, &received) != STYXRPC_OK ||
        received.selected != 0)
    {
        return 0;
    }
    const StyxRpcSelection single = {.count = 1, .guards = {{1, 1, NULL, 0}}};
    return styxrpc_value_integer(STYXRPC_VALUE_INT, -71, child, sizeof(child), &field) == STYXRPC_OK &&
        styxrpc_channel_reply(grants, 2, &single, 1, child, field, reply, sizeof(reply), &reply_size) == STYXRPC_OK &&
        styxrpc_channel_result(grants, 2, &single, reply, reply_size, &received) == STYXRPC_OK &&
        received.selected == 1 && received.integer == -71;
}


/* EN: Accept a nested object update, then reject an index outside the original graph.
 * RU: Принять обновление вложенного объекта, затем отклонить отсутствующий индекс. */
static int graph_round_trip(void)
{
    const StyxRpcSchema schema = {"(r[r[i]])", "()"};
    unsigned char value[32], inner[40], outer[48], request[64], nil[12], updates[64], reply[96];
    size_t val, ins, ots, req, ret, ups, length;
    const unsigned char first = 0, second = 1;
    StyxRpcGraphReply decoded;
    if (styxrpc_value_integer(STYXRPC_VALUE_INT, 41, value, sizeof(value), &val) != STYXRPC_OK ||
        fvm_srecord_write_pair(STYXRPC_VALUE_REF, &second, 1, value, val,
            inner, sizeof(inner), &ins) != FVM_S_OK ||
        fvm_srecord_write_pair(STYXRPC_VALUE_REF, &first, 1, inner, ins,
            outer, sizeof(outer), &ots) != FVM_S_OK ||
        styxrpc_graph_request(&schema, outer, ots, request, sizeof(request), &req) != STYXRPC_OK ||
        fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, nil, sizeof(nil), &ret) != FVM_S_OK ||
        styxrpc_value_integer(STYXRPC_VALUE_INT, 42, value, sizeof(value), &val) != STYXRPC_OK)
    {
        return 0;
    }
    for (unsigned char index = 1; index <= 2; index++)
    {
        unsigned char selector[] = {index, 1};
        if (fvm_srecord_write_pair(STYXRPC_VALUE_REF, selector, sizeof(selector), value, val,
                inner, sizeof(inner), &ins) != FVM_S_OK ||
            fvm_srecord_write(STYXRPC_GRAPH_UPDATES_TAG, inner, ins, updates, sizeof(updates), &ups) != FVM_S_OK)
        {
            return 0;
        }
        StyxRpcStatus status = styxrpc_graph_reply(&schema, request, req, nil, ret, updates, ups,
            reply, sizeof(reply), &length);
        if (status != (index == 1 ? STYXRPC_OK : STYXRPC_INVALID) ||
            (index == 1 && (styxrpc_graph_read(&schema, request, req, reply, length, &decoded) != STYXRPC_OK ||
                decoded.updates.size != ins)))
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Keep one identity for typed aliases and refuse aliasing incompatible channels.
 * RU: Сохранить один ID для псевдонимов и отклонить несовместимые типы одного канала. */
static int channel_grants(void)
{
    const StyxRpcSchema equal = {"(c[s]c[s])", "(i)"};
    const StyxRpcSchema conflicting = {"(c[s]c[i])", "(i)"};
    unsigned char children[64], bytes[96], token[STYXRPC_CHANNEL_TOKEN_SIZE] = {1};
    unsigned char key[] = {0, 0, 0, 7};
    size_t prefix, first, second, length;
    StyxRpcChannelCall call;
    return styxrpc_value_integer(STYXRPC_VALUE_BIG, 41, children, sizeof(children), &prefix) == STYXRPC_OK &&
        fvm_srecord_write(STYX_MANAGED_CHANNEL_TAG, key, sizeof(key), children + prefix,
            sizeof(children) - prefix, &first) == FVM_S_OK &&
        fvm_srecord_write(STYX_MANAGED_CHANNEL_TAG, key, sizeof(key), children + prefix + first,
            sizeof(children) - prefix - first, &second) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_GRAPH_REQUEST_TAG, children, prefix + first + second,
            bytes, sizeof(bytes), &length) == FVM_S_OK &&
        styxrpc_channel_call(&equal, bytes, length, &call) == STYXRPC_OK &&
        call.pid == 41 && call.count == 1 && call.grants[0].key == 7 &&
        styxrpc_channel_call(&conflicting, bytes, length, &call) == STYXRPC_INVALID &&
        styxrpc_channel_export_value(token, UINT32_MAX, 0, bytes, sizeof(bytes), &length) == STYXRPC_INVALID &&
        styxrpc_channel_export_value(token, 7, 1, bytes, sizeof(bytes), &length) == STYXRPC_OK;
}


/* EN: Require a confirmed same-route key and roll back a partially validated element.
 * RU: Требовать ключ текущего маршрута и отменять добавления частично верного элемента. */
static int nested_grants(void)
{
    StyxRpcChannelCall call = {.pid = 41, .count = 1, .grants = {{1, "(c[s])"}}};
    unsigned char key[] = {0, 0, 0, 2};
    unsigned char child[32], element[48], request[64];
    size_t length, extent, used;
    if (fvm_srecord_write(STYX_MANAGED_CHANNEL_TAG, key, sizeof(key), child, sizeof(child), &length) != FVM_S_OK ||
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, child, length, element, sizeof(element), &extent) != FVM_S_OK)
    {
        return 0;
    }
    const StyxRpcSelection send = {.count = 1, .guards = {{1, 0, element, extent}}};
    if (styxrpc_channel_request(call.grants, call.count, &send, NULL, 0,
            request, sizeof(request), &used) != STYXRPC_INVALID ||
        styxrpc_channel_import_element(&call, 1, element, extent) != STYXRPC_OK ||
        styxrpc_channel_import_element(&call, 1, element, extent) != STYXRPC_OK ||
        call.count != 2 || call.grants[1].key != 2 ||
        styxrpc_channel_request(call.grants, call.count, &send, NULL, 0,
            request, sizeof(request), &used) != STYXRPC_OK)
    {
        return 0;
    }

    call.grants[0] = (StyxRpcChannelGrant){1, "((c[i]c[i]))"};
    const unsigned char new_key[] = {0, 0, 0, 3};
    size_t second;
    if (fvm_srecord_write(STYX_MANAGED_CHANNEL_TAG, new_key, sizeof(new_key), child, sizeof(child), &length) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_CHANNEL_TAG, key, sizeof(key), child + length,
            sizeof(child) - length, &second) != FVM_S_OK ||
        fvm_srecord_write(STYXRPC_VALUE_TUPLE, child, length + second, request, sizeof(request), &used) != FVM_S_OK ||
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, request, used, element, sizeof(element), &extent) != FVM_S_OK)
    {
        return 0;
    }
    return styxrpc_channel_import_element(&call, 1, element, extent) == STYXRPC_INVALID && call.count == 2;
}


/* EN: Authorize a single exact PID for the route-policy regression.
 * RU: Разрешить один точный PID для проверки политики маршрутов. */
static int allow_pid(const void *context, const StyxRpcChannelRoute *route)
{
    const uint32_t *pid = context;
    return route->kind == STYXRPC_ROUTE_PID && route->pid == *pid &&
        route->channel.key == 2 && route->unbuffered && !strcmp(route->channel.shape, "(s)");
}


/* EN: Test routed framing, exact policy boundaries and transactional alias validation.
 * RU: Проверить кадры маршрутов, границы политики и атомарную проверку алиасов. */
static int routed_grants(void)
{
    uint32_t pid = UINT32_C(0x80001000);
    const StyxRpcChannelRoute reverse = {.kind = STYXRPC_ROUTE_PID, .pid = pid,
        .channel = {2, "(s)"}, .unbuffered = 1};
    const StyxRpcChannelRoute module = {.kind = STYXRPC_ROUTE_MODULE, .token = {13, 42}, .channel = {2, "(s)"}};
    const StyxRpcChannelGrant outer = {1, "(c[s])"};
    const StyxRpcChannelPolicy policy = {allow_pid, &pid};
    unsigned char child[64], frame[80], request[96];
    size_t first, second, size, length;
    StyxRpcChannelRoutes routes = {0};
    if (styxrpc_channel_route_value(&reverse, child, sizeof(child), &first) != STYXRPC_OK ||
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, child, first, frame, sizeof(frame), &size) != FVM_S_OK)
    {
        return 0;
    }
    const StyxRpcSelection send = {.count = 1, .guards = {{1, 0, frame, size}}, .policy = &policy};
    StyxRpcSelection read;
    if (styxrpc_channel_request(&outer, 1, &send, NULL, 0, request, sizeof(request), &length) != STYXRPC_OK ||
        styxrpc_channel_read(&outer, 1, request, length, &read) != STYXRPC_INVALID ||
        styxrpc_channel_read_routed(&outer, 1, &policy, request, length, &read) != STYXRPC_OK)
    {
        return 0;
    }
    ++pid;
    if (styxrpc_channel_read_routed(&outer, 1, &policy, request, length, &read) != STYXRPC_INVALID ||
        styxrpc_channel_route_value(&module, child + first, sizeof(child) - first, &second) != STYXRPC_OK ||
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, child, first + second, frame, sizeof(frame), &size) != FVM_S_OK ||
        styxrpc_channel_element_routes("(c[s]c[s])", frame, size, &routes) != STYXRPC_OK || routes.count != 2 ||
        routes.entries[1].kind != STYXRPC_ROUTE_MODULE ||
        memcmp(routes.entries[1].token, module.token, sizeof(module.token)))
    {
        return 0;
    }
    memcpy(child + first, child, first);
    if (fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, child, 2 * first, frame, sizeof(frame), &size) != FVM_S_OK ||
        styxrpc_channel_element_routes("(c[s]c[i])", frame, size, &routes) != STYXRPC_INVALID || routes.count != 2 ||
        styxrpc_channel_element_routes("(c[s]c[s])", frame, size, &routes) != STYXRPC_OK || routes.count != 1)
    {
        return 0;
    }
    const unsigned char empty_module[STYXRPC_CHANNEL_TOKEN_SIZE + 4] = {[STYXRPC_CHANNEL_TOKEN_SIZE + 3] = 1};
    return fvm_srecord_write(STYX_MANAGED_CHANNEL_MODULE_GRANT_TAG, empty_module, sizeof(empty_module),
            child, sizeof(child), &first) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, child, first, frame, sizeof(frame), &size) == FVM_S_OK &&
        styxrpc_channel_element_routes("(c[s])", frame, size, &routes) == STYXRPC_INVALID && routes.count == 1;
}


/* EN: Check both independent protocol surfaces, failing with context.
 * RU: Проверить обе части протокола и указать причину отказа. */
int main(void)
{
    if (!channel_round_trip() || !channel_grants() || !nested_grants() || !routed_grants())
    {
        fputs("native channel admission or result failed\n", stderr);
        return 1;
    }
    if (!graph_round_trip())
    {
        fputs("managed nested reference update failed\n", stderr);
        return 1;
    }
    return 0;
}
