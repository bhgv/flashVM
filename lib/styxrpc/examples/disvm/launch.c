/* Copyright (c) 2026 bhgv
 * EN: Submit one Dis launch from a native C process and read its exit status.
 * RU: Запустить Dis из C-процесса и прочитать статус завершения.
 */
#define _POSIX_C_SOURCE 200809L
#include "styxrpc/launch.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN: Consume the connected descriptor and submit the given startup argument.
 * Return zero for Dis success, 3 for a confirmed Dis failure, or 1 for uncertainty.
 * RU: Принять подключённый fd и подать указанный аргумент запуска.
 * Вернуть ноль при успехе Dis, 3 при подтверждённой ошибке либо 1 при неопределённости. */
int main(int argc, char **argv)
{
    char *end;
    long fd = argc == 3 ? strtol(argv[1], &end, 10) : -1;
    if (fd < 3 || fd > INT_MAX || *end)
    {
        return 2;
    }
    PhyBidir *mux = phybidircreate(phyposixfd((int)fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    StyxClient *client = mux ? styxclientmountphy(phybidirchannel(mux, 0), 4096, "caller", "", 1000, 8) : NULL;
    if (!client)
    {
        phybidirfree(mux);
        return 1;
    }
    unsigned char encoded[256];
    StyxRpcLaunchStatus status = styxrpc_launch_start(client, argv[2], strlen(argv[2]), encoded, sizeof(encoded));
    const struct timespec pause = {0, 10000000};
    for (unsigned tries = 0; status == STYXRPC_LAUNCH_PENDING && tries < 200; ++tries)
    {
        status = styxrpc_launch_status(client);
        if (status == STYXRPC_LAUNCH_PENDING)
        {
            nanosleep(&pause, NULL);
        }
    }
    styxclientshutdown(client);
    styxclientfree(client);
    phybidirfree(mux);
    printf("Dis launch status %d\n", status);
    return status == STYXRPC_LAUNCH_OK ? 0 : status == STYXRPC_LAUNCH_FAILED ? 3 : 1;
}
