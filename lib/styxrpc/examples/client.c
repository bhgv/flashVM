/* Copyright (c) 2026 bhgv
 * EN: Call the native device twice via one Styx connection.
 * RU: Дважды вызвать нативное устройство по одному соединению Styx.
 */
#include "client.h"
#include "styxrpc/scalar_client.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>


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


/* EN: Take the connected fd in argv[1]. The mount consumes it.
 * Return 2 for bad arguments, 1 for setup/RPC failure, or 0 for two confirmed replies.
 * RU: Принять подключённый fd из argv[1]. Монтирование забирает fd.
 * Вернуть 2 при ошибке аргумента, 1 при ошибке соединения/вызова, 0 при двух ответах. */
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
    int good = exercise(client);
    styxclientshutdown(client);
    styxclientfree(client);
    if (good)
    {
        puts("PASS native C calls standalone C device");
    }
    return good ? 0 : 1;
}
