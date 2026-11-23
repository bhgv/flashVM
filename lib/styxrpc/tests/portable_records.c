/* Copyright (c) 2026 bhgv
 * EN: Verify reusable channel records and positional nested reference updates.
 * RU: Проверить переиспользуемые кадры каналов и позиционные изменения вложенных ref.
 */
#include "styxrpc/channel.h"
#include "styxrpc/graph.h"
#include "call_record.h"
#include <stdio.h>


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


/* EN: Check both independent protocol surfaces, failing with context.
 * RU: Проверить обе части протокола и указать причину отказа. */
int main(void)
{
    if (!channel_round_trip())
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
