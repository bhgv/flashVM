/* Copyright (c) 2026 bhgv
 * EN: Publish the exact file selected by the Limbo fixture, then make a Styx round trip.
 * RU: Опубликовать точный файл примера Limbo и выполнить обмен через Styx.
 */
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lib9.h>
#include <styx.h>
#include "variable_server.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "resource.h"

extern int __real_main(int argc, char **argv); /* Actual hosted DisVM entry. */
extern int __real_dvm_stack_bind(DVM_ResourcePlan *, DVM_ResourcePlan *, unsigned, int64_t); /* Actual mount. */

static pthread_t reader; /* Joined before the fixture exits. */
static StyxVariableServer *server; /* Owns the exported native provider pin. */
static int remote_fd = -1; /* Client carrier end transferred to its thread. */
static int result; /* Set by the remote reader, consumed after join. */


/* EN/RU: Read the owner's typed update, then write a value back on the same fid.
 * Прочитать обновление владельца и записать ответ через тот же fid. */
static void *exchange(void *unused)
{
    (void)unused;
    int breaker = dup(remote_fd);
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    StyxClient *client = styxclientmountphy(phyposixfd(remote_fd, caps), 256, "fixture", "", 1500, 4);
    StyxFid *file = client ? styxclientwalk(client, ".var/count") : NULL;
    if (file && styxclientopen(file, ORDWR))
    {
        const unsigned char expected[] = {255, 255, 255, 254};
        unsigned char bytes[4];
        StyxTransfer transfer;
        for (unsigned attempt = 0; attempt < 300; attempt++)
        {
            styxclientread(file, 0, bytes, sizeof(bytes), &transfer);
            if (!transfer.success || transfer.count != sizeof(bytes))
            {
                break;
            }
            if (!memcmp(bytes, expected, sizeof(bytes)))
            {
                const unsigned char update[] = {0, 1, 0, 3};
                styxclientwrite(file, 0, update, sizeof(update), &transfer);
                result = transfer.success && transfer.confirmed && transfer.count == sizeof(update);
                break;
            }
            const struct timespec delay = {0, 10000000};
            nanosleep(&delay, NULL);
        }
    }
    styxclientclunk(file);
    if (breaker >= 0)
    {
        shutdown(breaker, SHUT_RDWR);
        close(breaker);
    }
    styxclientfree(client);
    return NULL;
}


/* EN/RU: Observe a successful test mount and hand its same exact provider to Styx.
 * После успешного монтажа стенда передать Styx тот же точный провайдер. */
int __wrap_dvm_stack_bind(DVM_ResourcePlan *source, DVM_ResourcePlan *target, unsigned flags, int64_t index)
{
    int mounted = __real_dvm_stack_bind(source, target, flags, index);
    if (mounted != 1 || server || strcmp(target->names.path, "/cell"))
    {
        return mounted;
    }
    int pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair))
    {
        exit(2);
    }
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    server = styxvariableserverstart(phyposixfd(pair[0], caps), "count", source->provider);
    remote_fd = pair[1];
    if (!server || pthread_create(&reader, NULL, exchange, NULL))
    {
        exit(2);
    }
    return mounted;
}


/* EN/RU: Run the original VM and verify completion of both ends.
 * Запустить исходную VM и проверить завершение обеих сторон. */
int __wrap_main(int argc, char **argv)
{
    int status = __real_main(argc, argv);
    if (!server || pthread_join(reader, NULL) || !styxvariableserverjoin(server) || !result)
    {
        fputs("Limbo variable Styx exchange failed\n", stderr);
        return 2;
    }
    return status;
}
