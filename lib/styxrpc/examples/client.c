/* Copyright (c) 2026 bhgv
 * EN: Call the native device twice via one Styx connection.
 * RU: Дважды вызвать нативное устройство по одному соединению Styx.
 */
#include "client.h"
#include "styxrpc/rpc.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>


/* EN: Borrow client, send argument on a fresh function fid and compare the reply.
 * Return zero on an uncertain transfer without replaying the call.
 * RU: Через заимствованный client передать argument на новом fid и проверить ответ.
 * При неподтверждённой операции вернуть ноль, не повторяя вызов. */
static int invoke(StyxClient *client, int argument)
{
    unsigned char request[STYXRPC_FRAME_SIZE];
    size_t request_size;
    if (styxrpc_request(STYXRPC_INT_SIGNATURE, argument, request, sizeof(request), &request_size) != STYXRPC_OK)
    {
        return 0;
    }
    StyxFid *fid = styxclientwalk(client, "mounted/device.dis/sample");
    if (!fid || !styxclientopen(fid, 2))
    {
        styxclientclunk(fid);
        return 0;
    }
    StyxTransfer sent;
    styxclientwrite(fid, 0, request, request_size, &sent);
    int good = sent.success && sent.confirmed && !sent.uncertain && sent.count == request_size;
    unsigned char reply[STYXRPC_FRAME_SIZE + 1];
    size_t offset = 0;
    while (good && offset < sizeof(reply))
    {
        StyxTransfer received;
        styxclientread(fid, offset, reply + offset, sizeof(reply) - offset, &received);
        good = received.success && received.confirmed && !received.uncertain;
        if (!good || !received.count)
        {
            break;
        }
        offset += received.count;
    }
    styxclientclunk(fid);
    int64_t result;
    return good && styxrpc_reply(STYXRPC_INT_SIGNATURE, reply, offset, &result) == STYXRPC_OK &&
        result == (int64_t)argument + 1;
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
    int good = invoke(client, 41) && invoke(client, -10);
    styxclientshutdown(client);
    styxclientfree(client);
    if (good)
    {
        puts("PASS native C calls standalone C device");
    }
    return good ? 0 : 1;
}
