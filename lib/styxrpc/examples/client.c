/* Copyright (c) 2026 bhgv
 * EN: Call the native scalar and managed exports via one Styx connection.
 * RU: Вызвать скалярные и управляемые экспорты по одному соединению Styx.
 */
#include "client.h"
#include "styxrpc/managed_client.h"
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
    int good = exercise(client) && exercise_managed(client);
    styxclientshutdown(client);
    styxclientfree(client);
    if (good)
    {
        puts("PASS native C calls standalone C device");
    }
    return good ? 0 : 1;
}
