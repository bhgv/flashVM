/* Copyright (c) 2026 bhgv
 * EN: Increment a nested ref using the portable positional graph codec.
 * RU: Изменение вложенного ref переносимым позиционным кодеком графов.
 */
#include "graph_device.h"
#include "scalar_record.h"
#include <string.h>

const StyxRpcSchema bump_schema = {"(r[r[i]])", "()"}; /* Box.inner.counter. */


/* EN: Check the complete request, then stage one sparse update for the existing nested identity.
 * RU: Проверить полный запрос, затем подготовить изменение исходного вложенного ID. */
static int finish(BumpCall *call)
{
    FVM_SRecord arguments, outer, inner;
    size_t used;
    if (styxrpc_graph_arguments(&bump_schema, call->request, call->staged, &arguments) != STYXRPC_OK ||
        fvm_srecord_read(arguments.payload, arguments.size, sizeof(call->request), &outer, &used) != FVM_S_OK ||
        outer.tag != STYXRPC_VALUE_REF || outer.size < 2 ||
        fvm_srecord_read(outer.payload + 1, outer.size - 1, sizeof(call->request), &inner, &used) != FVM_S_OK ||
        inner.tag != STYXRPC_VALUE_REF || inner.size < 2)
    {
        return 0;
    }
    int64_t counter;
    StyxScalarKind kind;
    if (!styxscalarread(STYXRPC_VALUE_INT, inner.payload + 1, inner.size - 1, &kind, &counter) ||
        kind != STYX_SCALAR_VALUE || counter == INT32_MAX)
    {
        return 0;
    }
    const unsigned char selector[] = {inner.payload[0], 1};
    unsigned char value[24], change[40], updates[48], nil[12];
    size_t length, changed, update_size, nil_size;
    return styxrpc_value_integer(STYXRPC_VALUE_INT, counter + 1, value, sizeof(value), &length) == STYXRPC_OK &&
        fvm_srecord_write_pair(STYXRPC_VALUE_REF, selector, sizeof(selector), value, length,
            change, sizeof(change), &changed) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_GRAPH_UPDATES_TAG, change, changed, updates, sizeof(updates), &update_size) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, nil, sizeof(nil), &nil_size) == FVM_S_OK &&
        styxrpc_graph_reply(&bump_schema, call->request, call->staged,
            nil, nil_size, updates, update_size, call->reply, sizeof(call->reply), &call->ready) == STYXRPC_OK;
}


/* EN/RU: See graph_device.h. / См. graph_device.h. */
int bump_write(BumpCall *call, size_t offset, const void *bytes, size_t size)
{
    if (!call || call->refused || call->ready || !size || offset != call->staged ||
        size > sizeof(call->request) - call->staged)
    {
        return 0;
    }
    memcpy(call->request + offset, bytes, size);
    call->staged += size;
    FVM_SRecord record;
    size_t used;
    FVM_SResult status = fvm_srecord_read(call->request, call->staged, sizeof(call->request), &record, &used);
    if (status == FVM_S_MORE && call->staged < sizeof(call->request))
    {
        return 1;
    }
    if (status != FVM_S_OK || used != call->staged || !finish(call))
    {
        call->refused = 1;
        return 0;
    }
    return 1;
}


/* EN/RU: See graph_device.h. / См. graph_device.h. */
int bump_read(const BumpCall *call, size_t offset, void *bytes, size_t capacity, size_t *size)
{
    if (!call || call->refused)
    {
        return 0;
    }
    size_t remaining = offset < call->ready ? call->ready - offset : 0;
    *size = capacity < remaining ? capacity : remaining;
    if (*size)
    {
        memcpy(bytes, call->reply + offset, *size);
    }
    return 1;
}
