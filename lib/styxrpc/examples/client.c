/* Copyright (c) 2026 bhgv
 * EN: Call scalar, managed and graph exports via one Styx connection.
 * RU: Вызвать скалярные, управляемые и графовые экспорты по одному соединению Styx.
 */
#include "client.h"
#include "graph_device.h"
#include "styxrpc/graph_client.h"
#include "styxrpc/managed_client.h"
#include "styxrpc/vfs_client.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN: Check two fresh calls and two refused outcomes on the borrowed client.
 * RU: Проверить два новых вызова и два отказа через заимствованный клиент. */
static int exercise(StyxClient *client)
{
    const char *module = "mounted/device.dis";
    int64_t result = 0;
    return styxrpc_scalar_invoke(client, module, "sample", STYXRPC_BIG_SIGNATURE, 41, 1000, &result) ==
            STYXRPC_CALL_NOT_SENT && result == 0 &&
        styxrpc_scalar_invoke(client, module, "sample", STYXRPC_INT_SIGNATURE, 41, 1000, &result) ==
            STYXRPC_CALL_OK && result == 42 &&
        styxrpc_scalar_invoke(client, module, "sample", STYXRPC_INT_SIGNATURE, INT32_MAX, 1000, &result) ==
            STYXRPC_CALL_REMOTE_ERROR && result == 42 &&
        styxrpc_scalar_invoke(client, module, "sample", STYXRPC_INT_SIGNATURE, -10, 1000, &result) ==
            STYXRPC_CALL_OK && result == -9;
}


/* EN: Encode one managed call using the standalone SDK and verify a flat ref update.
 * RU: Закодировать управляемый вызов независимым SDK и проверить обновление плоского ref. */
static int exercise_managed(StyxClient *client)
{
    const StyxRpcManagedTarget target = {
        client, "mounted/device.dis", "adjust", UINT32_C(0xe6d83cc8), {"(sa[i]r[i])", "((si))"}, 0, 1000
    };
    unsigned char args[128], child[STYXRPC_FRAME_SIZE], reply[256];
    unsigned char index = 0;
    size_t text_size, array_size, ref_size, child_size;
    StyxRpcManagedReply decoded;
    if (fvm_srecord_write(STYXRPC_VALUE_STRING, "sensor", 6, args, sizeof(args), &text_size) != FVM_S_OK ||
        styxrpc_value_integer(STYXRPC_VALUE_INT, 5, child, sizeof(child), &child_size) != STYXRPC_OK ||
        fvm_srecord_write(STYXRPC_VALUE_ARRAY, child, child_size, args + text_size,
            sizeof(args) - text_size, &array_size) != FVM_S_OK ||
        styxrpc_value_integer(STYXRPC_VALUE_INT, 37, child, sizeof(child), &child_size) != STYXRPC_OK ||
        fvm_srecord_write_pair(STYXRPC_VALUE_REF, &index, 1, child, child_size, args + text_size + array_size,
            sizeof(args) - text_size - array_size, &ref_size) != FVM_S_OK ||
        styxrpc_managed_invoke(&target, args, text_size + array_size + ref_size, reply, sizeof(reply), &decoded) !=
            STYXRPC_CALL_OK || decoded.void_result || decoded.changed != 1)
    {
        return 0;
    }
    FVM_SRecord tuple, text, update;
    size_t used;
    int64_t result;
    return fvm_srecord_read(decoded.result.payload, decoded.result.size, sizeof(reply), &tuple, &used) == FVM_S_OK &&
        tuple.tag == STYXRPC_VALUE_TUPLE && used == decoded.result.size &&
        fvm_srecord_read(tuple.payload, tuple.size, sizeof(reply), &text, &used) == FVM_S_OK &&
        text.tag == STYXRPC_VALUE_STRING && text.size == 6 && !memcmp(text.payload, "sensor", 6) &&
        styxrpc_value_get_integer(STYXRPC_VALUE_INT, tuple.payload + used, tuple.size - used, &result) == STYXRPC_OK &&
        result == 42 &&
        fvm_srecord_read(decoded.updates.payload, decoded.updates.size, sizeof(reply), &update, &used) == FVM_S_OK &&
        update.tag == STYXRPC_VALUE_REF && used == decoded.updates.size && update.size > 1 &&
        update.payload[0] == index && styxrpc_value_get_integer(STYXRPC_VALUE_INT, update.payload + 1,
            update.size - 1, &result) == STYXRPC_OK && result == 42;
}


/* EN: Use the independent byte API on the same mounted provider.
 * RU: Использовать независимый байтовый API на том же смонтированном провайдере. */
static int exercise_vfs(StyxClient *client)
{
    const char *path = "mounted/device.dis/value";
    StyxRpcFileInfo info;
    unsigned char bytes[2];
    size_t count = 0;
    return styxrpc_vfs_stat(client, path, &info) == STYXRPC_IO_OK && !info.directory &&
        styxrpc_vfs_read_at(client, path, 0, bytes, sizeof(bytes), &count) == STYXRPC_IO_OK &&
        count == 1 && bytes[0] == 'A' &&
        styxrpc_vfs_write_at(client, path, 0, "Z", 1, &count) == STYXRPC_IO_OK && count == 1 &&
        styxrpc_vfs_read_at(client, path, 0, bytes, sizeof(bytes), &count) == STYXRPC_IO_OK &&
        count == 1 && bytes[0] == 'Z' &&
        styxrpc_vfs_read_at(client, "mounted/device.dis/../value", 0, bytes, sizeof(bytes), &count) ==
            STYXRPC_IO_NOT_OPEN && count == 0;
}


/* EN: Invoke the nested device ref and decode its sparse change using only C.
 * RU: Вызвать устройство с вложенным ref и декодировать изменение средствами C. */
static int exercise_graph(StyxClient *client)
{
    const StyxRpcManagedTarget target = {
        client, "mounted/device.dis", "bump", BUMP_SIGNATURE, {"(r[r[i]])", "()"}, 0, 1000
    };
    unsigned char value[24], leaf[48], arguments[72], request[96], reply[256];
    const unsigned char parent_id = 0, child_id = 1;
    size_t value_size, leaf_size, size, request_size, used, index;
    FVM_SRecord original, update;
    StyxRpcGraphReply decoded;
    StyxRpcGraphFields fields;
    int64_t counter;
    return styxrpc_value_integer(STYXRPC_VALUE_INT, 41, value, sizeof(value), &value_size) == STYXRPC_OK &&
        fvm_srecord_write_pair(STYXRPC_VALUE_REF, &child_id, 1, value, value_size,
            leaf, sizeof(leaf), &leaf_size) == FVM_S_OK &&
        fvm_srecord_write_pair(STYXRPC_VALUE_REF, &parent_id, 1, leaf, leaf_size,
            arguments, sizeof(arguments), &size) == FVM_S_OK &&
        styxrpc_graph_invoke(&target, arguments, size, reply, sizeof(reply), &decoded) == STYXRPC_CALL_OK &&
        decoded.result.tag == STYXRPC_VALUE_NIL && decoded.result.size == 0 &&
        styxrpc_graph_request(&target.schema, arguments, size, request, sizeof(request), &request_size) == STYXRPC_OK &&
        styxrpc_graph_arguments(&target.schema, request, request_size, &original) == STYXRPC_OK &&
        fvm_srecord_read(decoded.updates.payload, decoded.updates.size, sizeof(reply), &update, &used) == FVM_S_OK &&
        used == decoded.updates.size &&
        styxrpc_graph_update_index(&target.schema, &original, &update, 2, &index) && index == child_id &&
        styxrpc_graph_update_fields("r[i]", &update, &fields) && fields.mask == 1 &&
        styxrpc_value_get_integer(STYXRPC_VALUE_INT, fields.children.payload, fields.children.size,
            &counter) == STYXRPC_OK && counter == 42;
}


/* EN: Take the connected fd in argv[1]. The mount consumes it.
 * Return 2 for bad arguments, 1 for setup/RPC failure, or 0 for checked outcomes.
 * RU: Принять подключённый fd из argv[1]. Монтирование забирает fd.
 * Вернуть 2 при ошибке аргумента, 1 при ошибке соединения/вызова, 0 при проверенных результатах. */
int main(int argc, char **argv)
{
    char *end;
    long fd = argc == 2 ? strtol(argv[1], &end, 10) : -1;
    if (fd < 3 || fd > INT_MAX || *end)
    {
        return 2;
    }
    StyxClient *client = styxclientmountfd((int)fd, 8192, "device", "", 5000, 8);
    if (!client)
    {
        return 1;
    }
    int good = exercise(client) && exercise_managed(client) && exercise_graph(client) && exercise_vfs(client);
    styxclientshutdown(client);
    styxclientfree(client);
    if (good)
    {
        puts("PASS native C calls standalone C device");
    }
    return good ? 0 : 1;
}
