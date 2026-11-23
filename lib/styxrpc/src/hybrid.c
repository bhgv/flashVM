/* Copyright (c) 2026 bhgv
 * EN: Expose native Dis table semantics over owner-local C storage.
 * RU: Семантика нативных таблиц Dis на локальном хранилище C.
 */
#include "styxrpc/hybrid.h"
#include "call_record.h"
#include "scalar_record.h"
#include "flashvm/utf8.h"
#include "table.h"
#include <stdlib.h>
#include <string.h>

#include "managed_table_state.inc"


/* EN/RU: See styxrpc/hybrid.h. / См. styxrpc/hybrid.h. */
StyxRpcHybrid *styxrpc_hybrid_open(const void *snapshot, size_t size)
{
    if (!snapshot && !size)
    {
        return fht_create(NULL);
    }
    return snapshot && size && size <= STYXRPC_MANAGED_LIMIT ? wire_table_decode(snapshot, size) : NULL;
}


/* EN/RU: See styxrpc/hybrid.h. / См. styxrpc/hybrid.h. */
void styxrpc_hybrid_close(StyxRpcHybrid *table)
{
    fht_destroy(table);
}


/* EN/RU: See styxrpc/hybrid.h. / См. styxrpc/hybrid.h. */
int styxrpc_hybrid_apply(StyxRpcHybrid *table, StyxRpcHybridRequest *request)
{
    if (!table || !request)
    {
        return 0;
    }
    switch (request->operation)
    {
    case STYXRPC_HYBRID_LENGTH:
        request->count = fht_length(table);
        return 1;
    case STYXRPC_HYBRID_COUNT:
        request->count = fht_count(table);
        return 1;
    case STYXRPC_HYBRID_GET:
    case STYXRPC_HYBRID_SET:
    case STYXRPC_HYBRID_DELETE:
        break;
    default:
        return 0;
    }
    const StyxRpcHybridKey *source = &request->key;
    if ((source->text != 0 && source->text != 1) ||
        (source->text && (source->size > STYXRPC_MANAGED_LIMIT ||
        (!source->bytes && source->size) ||
        fvm_utf8_validate(source->bytes, source->size, NULL) != FVM_UTF8_OK)))
    {
        return 0;
    }
    FhtKey key = {source->text ? FHT_TEXT : FHT_INDEX, source->index, source->bytes, source->size};
    switch (request->operation)
    {
    case STYXRPC_HYBRID_GET:
        return fht_get(table, key, &request->value) == FHT_OK;
    case STYXRPC_HYBRID_SET:
        return fht_set(table, key, request->value) == FHT_OK;
    default:
        return fht_delete(table, key) == FHT_OK;
    }
}


/* EN/RU: See styxrpc/hybrid.h. / См. styxrpc/hybrid.h. */
int styxrpc_hybrid_snapshot(const StyxRpcHybrid *table, void *bytes, size_t capacity, size_t *used)
{
    if (!table || !bytes || !capacity || capacity > STYXRPC_MANAGED_LIMIT || !used)
    {
        return 0;
    }
    unsigned char *scratch = malloc(capacity * 2);
    TableWire wire = {scratch, scratch ? scratch + capacity : NULL, 0, capacity};
    int valid = scratch && table_snapshot(table, &wire) &&
        fvm_srecord_write(STYX_MANAGED_TABLE_TAG, wire.rows, wire.size, bytes, capacity, used) == FVM_S_OK;
    free(scratch);
    return valid;
}


/* EN/RU: See styxrpc/hybrid.h. / См. styxrpc/hybrid.h. */
int styxrpc_hybrid_diff(const StyxRpcHybrid *table, const void *baseline, size_t size,
    void *bytes, size_t capacity, size_t *used)
{
    if (!table || !baseline || !size || size > STYXRPC_MANAGED_LIMIT ||
        !bytes || !capacity || capacity > STYXRPC_MANAGED_LIMIT || !used)
    {
        return 0;
    }
    unsigned char *scratch = malloc(capacity * 2);
    TableDiff diff = {.baseline = wire_table_decode(baseline, size),
        .wire = {scratch, scratch ? scratch + capacity : NULL, 0, capacity}};
    int valid = scratch && diff.baseline && table_diff(table, &diff);
    *used = 0;
    if (valid && diff.wire.size)
    {
        valid = fvm_srecord_write(STYX_MANAGED_TABLE_PATCH_TAG, diff.wire.rows, diff.wire.size,
            bytes, capacity, used) == FVM_S_OK;
    }
    fht_destroy(diff.baseline);
    free(scratch);
    return valid;
}


/* EN/RU: See styxrpc/hybrid.h. / См. styxrpc/hybrid.h. */
StyxRpcHybrid *styxrpc_hybrid_stage(const StyxRpcHybrid *table, const void *patch, size_t size)
{
    return table && patch && size && size <= STYXRPC_MANAGED_LIMIT ? wire_table_stage(table, patch, size) : NULL;
}


/* EN/RU: See styxrpc/hybrid.h. / См. styxrpc/hybrid.h. */
int styxrpc_hybrid_big_parse(const char *bytes, size_t size, int64_t *value)
{
    return fht_big_parse(bytes, size, value) == FHT_OK;
}


/* EN/RU: See styxrpc/hybrid.h. / См. styxrpc/hybrid.h. */
size_t styxrpc_hybrid_big_format(int64_t value, char *bytes, size_t capacity)
{
    return fht_big_format(value, bytes, capacity);
}
