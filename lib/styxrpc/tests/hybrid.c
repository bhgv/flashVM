/* Copyright (c) 2026 bhgv
 * EN: Exercise the canonical table graph API with independent C owners and bounded records.
 * RU: Проверка API графов таблиц с независимыми владельцами C и ограниченными записями.
 */
#include "styxrpc/graph.h"
#include "styxrpc/hybrid.h"
#include <stdio.h>
#include <string.h>

#define TABLE_CHANGE_SIGNATURE UINT32_C(0x32bb77a4) /* Compiled hybrid signature used by the VM tests. */
#define TABLE_FIXTURE_BYTES 512U /* Bounded wire space for a small fixture. */


/* EN: Update a numeric or counted text key through the standalone SDK.
 * RU: Изменить числовой ключ или строку с длиной через самостоятельный SDK. */
static int update(StyxRpcHybrid *table, StyxRpcHybridOperation operation, StyxRpcHybridKey key, int64_t value)
{
    StyxRpcHybridRequest request = {operation, key, value, 0};
    return styxrpc_hybrid_apply(table, &request);
}


/* EN: Read one exact value from the selected namespace.
 * RU: Прочитать точное значение из выбранного пространства ключей. */
static int get(StyxRpcHybrid *table, StyxRpcHybridKey key, int64_t expected)
{
    StyxRpcHybridRequest request = {STYXRPC_HYBRID_GET, key, 0, 0};
    return styxrpc_hybrid_apply(table, &request) && request.value == expected;
}


/* EN: Stage a provider patch against the caller's current table and preserve unrelated local keys.
 * RU: Подготовить патч провайдера на текущей таблице клиента и сохранить остальные ключи. */
static int exchange(StyxRpcHybrid *caller, const StyxRpcHybridKey *text)
{
    const StyxRpcHybridKey one = {.index = 1}, far = {.index = INT64_MAX};
    const StyxRpcHybridKey local = {.text = 1, .bytes = "local", .size = 5};
    const StyxRpcHybridKey counted = {.text = 1, .bytes = "a\0b", .size = 3};
    unsigned char snapshot[TABLE_FIXTURE_BYTES], patch[TABLE_FIXTURE_BYTES];
    size_t baseline, changes, no_changes;
    if (!update(caller, STYXRPC_HYBRID_SET, one, 11) ||
        !update(caller, STYXRPC_HYBRID_SET, *text, 22) ||
        !update(caller, STYXRPC_HYBRID_SET, counted, INT64_MIN) ||
        !styxrpc_hybrid_snapshot(caller, snapshot, sizeof(snapshot), &baseline))
    {
        return 0;
    }
    StyxRpcHybrid *provider = styxrpc_hybrid_open(snapshot, baseline);
    StyxRpcHybrid *staged = NULL;
    int ok = provider && get(provider, one, 11) && get(provider, *text, 22) &&
        styxrpc_hybrid_diff(provider, snapshot, baseline, patch, sizeof(patch), &no_changes) && !no_changes &&
        update(provider, STYXRPC_HYBRID_DELETE, *text, 0) &&
        update(provider, STYXRPC_HYBRID_SET, *text, 22) &&
        update(provider, STYXRPC_HYBRID_SET, far, INT64_MAX) &&
        update(provider, STYXRPC_HYBRID_DELETE, one, 0) &&
        styxrpc_hybrid_diff(provider, snapshot, baseline, patch, sizeof(patch), &changes) && changes &&
        update(caller, STYXRPC_HYBRID_SET, local, 33);
    if (ok)
    {
        StyxRpcHybrid *invalid = styxrpc_hybrid_stage(caller, patch, changes - 1);
        StyxRpcHybridRequest removed = {STYXRPC_HYBRID_GET, one, 0, 0};
        staged = styxrpc_hybrid_stage(caller, patch, changes);
        ok = !invalid && staged && get(caller, one, 11) &&
            !styxrpc_hybrid_apply(staged, &removed) && get(staged, *text, 22) && get(staged, counted, INT64_MIN) &&
            get(staged, local, 33) && get(staged, far, INT64_MAX);
        styxrpc_hybrid_close(invalid);
    }
    styxrpc_hybrid_close(staged);
    styxrpc_hybrid_close(provider);
    return ok;
}


/* EN: Verify that a snapshot and a table patch travel as a standard positional graph update.
 * RU: Проверить перенос снимка и патча таблицы в штатном позиционном графе. */
static int graph_wire(StyxRpcHybrid *table, StyxRpcHybridKey key)
{
    const StyxRpcSchema schema = {"(h)", "(h)"};
    const unsigned char identity = 0; /* Original argument ID. */
    unsigned char blob[TABLE_FIXTURE_BYTES], arguments[TABLE_FIXTURE_BYTES], request[TABLE_FIXTURE_BYTES];
    unsigned char after[TABLE_FIXTURE_BYTES], scratch[TABLE_FIXTURE_BYTES], edit[TABLE_FIXTURE_BYTES];
    unsigned char result[TABLE_FIXTURE_BYTES], updates[TABLE_FIXTURE_BYTES], reply[TABLE_FIXTURE_BYTES];
    size_t size, extent, written, index, request_size, update_size, result_size, reply_size;
    FVM_SRecord original, before, changed, record;
    StyxRpcGraphReply decoded;
    if (!styxrpc_hybrid_snapshot(table, blob, sizeof(blob), &size) ||
        fvm_srecord_write_pair(STYXRPC_VALUE_REF, &identity, 1, blob, size, arguments,
            sizeof(arguments), &extent) != FVM_S_OK ||
        styxrpc_graph_request(&schema, arguments, extent, request, sizeof(request), &request_size) != STYXRPC_OK ||
        styxrpc_graph_arguments(&schema, request, request_size, &original) != STYXRPC_OK ||
        fvm_srecord_read(original.payload, original.size, sizeof(arguments), &before, &size) != FVM_S_OK ||
        !update(table, STYXRPC_HYBRID_SET, key, INT64_MAX) ||
        !styxrpc_hybrid_snapshot(table, after + 1, sizeof(after) - 1, &size))
    {
        return 0;
    }
    after[0] = identity;
    changed = before;
    changed.payload = after;
    changed.size = size + 1;
    return styxrpc_graph_diff(&schema, &original, identity, "h", &before, &changed, scratch, sizeof(scratch),
            edit, sizeof(edit), &written) && written > 0 &&
        fvm_srecord_read(edit, written, sizeof(edit), &record, &extent) == FVM_S_OK && extent == written &&
        record.tag == STYXRPC_GRAPH_ARGUMENT_TAG &&
        styxrpc_graph_update_index(&schema, &original, &record, 1, &index) && index == identity &&
        fvm_srecord_write(STYXRPC_VALUE_REF, &identity, 1, blob, sizeof(blob), &size) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_GRAPH_RESULT_TAG, blob, size, result, sizeof(result), &result_size) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_GRAPH_UPDATES_TAG, edit, written, updates, sizeof(updates), &update_size) == FVM_S_OK &&
        styxrpc_graph_reply(&schema, request, request_size, result, result_size, updates, update_size,
            reply, sizeof(reply), &reply_size) == STYXRPC_OK &&
        styxrpc_graph_read(&schema, request, request_size, reply, reply_size, &decoded) == STYXRPC_OK;
}


/* EN: Run table and graph checks without a hosted Styx or VM dependency.
 * RU: Выполнить проверки таблиц и графов без хостового Styx и VM. */
int main(void)
{
    const StyxRpcHybridKey text = {.text = 1, .bytes = "1", .size = 1};
    unsigned char offer[128];
    size_t size;
    int64_t parsed;
    char decimal[STYXRPC_HYBRID_BIG_TEXT];
    StyxRpcHybrid *table = styxrpc_hybrid_open(NULL, 0);
    const StyxRpcSchema schema = {"(ha[h]r[hhi])", "((hh))"};
    int ok = table && exchange(table, &text) && graph_wire(table, text) &&
        styxrpc_graph_profile("change", TABLE_CHANGE_SIGNATURE, &schema, offer, sizeof(offer), &size) == STYXRPC_OK &&
        styxrpc_hybrid_big_parse("-9223372036854775808", 20, &parsed) && parsed == INT64_MIN &&
        styxrpc_hybrid_big_format(parsed, decimal, sizeof(decimal)) == 20 &&
        !strcmp(decimal, "-9223372036854775808") &&
        !styxrpc_hybrid_big_parse("9223372036854775808", 19, &parsed) && parsed == INT64_MIN;
    styxrpc_hybrid_close(table);
    if (!ok)
    {
        fputs("Hybrid C table or graph checks failed\n", stderr);
    }
    return ok ? 0 : 1;
}
