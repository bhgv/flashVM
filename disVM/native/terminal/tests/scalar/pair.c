/* Copyright (c) 2026 bhgv
 * EN: Two DisVM processes, each with its own original Styx file server.
 * RU: Два процесса DisVM, каждый со своим исходным файловым сервером Styx.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <unistd.h>
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include "mount.h"
#include "unibus_server.h"
#include "scalar_route.h"
#include <lib9.h>
#include "scalar_function.h"
#include "scalar_verify.h"
#include <string.h>
#include "guest_mount.h"

#define CALL_TAG 4294967295UL /* Maximum supported nonzero request tag. */
#define REPLY_TAG 64UL        /* First tag needing a leading magnitude group. */


/* EN/RU: Give the alternate module a distinct native result for the same input.
 * Выдать альтернативному модулю отдельный нативный результат для того же входа. */
static int adjust(void *context, int64_t input, int64_t *result)
{
    (void)context;
    *result = input - 7;
    return 1;
}


/* EN/RU: Run the owner service or mount its directory over the inherited TCP channel.
 * Запустить сервис владельца или смонтировать его каталог по унаследованному TCP. */
static int mounted_pair(int argc, char **argv, int fd, unsigned caps, int owner)
{
    int breaker = dup(fd);
    if (breaker < 0)
    {
        close(fd);
        return 2;
    }
    RemotePhy *channel = phyposixfd(fd, caps);
    int result = 2;
    if (owner)
    {
        if (getenv("DVM_TEST_SCALAR_MIXED_OWNER"))
        {
            const StyxMountedScalar entries[] = {
                {"wide.dis", "count", NULL, 0, 0, 0, 0}, {"wide.dis", "echo", NULL, 1, 0, 0, 0}
            };
            StyxUnibusServer *server = styxunibusserverstartmounted(channel, entries, 2);
            if (server)
            {
                DVM_Provider *requests[] = {
                    styxunibusservermailbox(server, 0, 0), styxunibusservermailbox(server, 1, 0)
                };
                DVM_Provider *replies[] = {
                    styxunibusservermailbox(server, 0, 1), styxunibusservermailbox(server, 1, 1)
                };
                result = scalar_runtime_shared(argc, argv, requests, replies);
                shutdown(breaker, SHUT_RDWR);
                if (!styxunibusserverjoin(server))
                {
                    result = 2;
                }
            }
            close(breaker);
            return result;
        }
        int tuple = getenv("DVM_TEST_SCALAR_TUPLE_OWNER") != NULL;
        int ref = getenv("DVM_TEST_SCALAR_REF_OWNER") != NULL;
        int list = getenv("DVM_TEST_SCALAR_LIST_OWNER") != NULL;
        if (list || ref || tuple || getenv("DVM_TEST_SCALAR_BIG_OWNER"))
        {
            const StyxMountedScalar entry = list ?
                (StyxMountedScalar){.module = "blend.dis", .name = "blend", .dis_call = 1} : ref ?
                (StyxMountedScalar){"ref.dis", "revise", NULL, 0, 0, 1, 0} : tuple ?
                (StyxMountedScalar){"tuple.dis", "transform", NULL, 0, 1, 0, 0} :
                (StyxMountedScalar){"wide.dis", "echo", NULL, 1, 0, 0, 0};
            StyxUnibusServer *server = styxunibusserverstartmounted(channel, &entry, 1);
            if (server)
            {
                result = scalar_runtime_run(argc, argv, styxunibusserverreply(server),
                    styxunibusserverinbox(server), NULL, NULL, 1);
                shutdown(breaker, SHUT_RDWR);
                if (!styxunibusserverjoin(server))
                {
                    result = 2;
                }
            }
            close(breaker);
            return result;
        }
        const StyxScalarFunction alternate = {
            STYX_FUNCTION_REQUEST_TAG, STYX_FUNCTION_REPLY_TAG, adjust, NULL, NULL, NULL, NULL, NULL, NULL, NULL
        };
        const StyxMountedScalar entries[] = {
            {"helper.dis", "offset", &alternate, 0, 0, 0, 0},
            {"compute.dis", "compute", NULL, 0, 0, 0, 0},
            {"compute.dis", "adjust", NULL, 0, 0, 0, 0}
        };
        StyxUnibusServer *server = styxunibusserverstartmounted(channel, entries,
            sizeof(entries) / sizeof(entries[0]));
        if (server)
        {
            DVM_Provider *requests[] = {
                styxunibusservermailbox(server, 2, 0), styxunibusservermailbox(server, 1, 0)
            };
            DVM_Provider *replies[] = {
                styxunibusservermailbox(server, 2, 1), styxunibusservermailbox(server, 1, 1)
            };
            result = scalar_runtime_shared(argc, argv, requests, replies);
            shutdown(breaker, SHUT_RDWR);
            if (!styxunibusserverjoin(server))
            {
                result = 2;
            }
        }
    }
    else
    {
        StyxClient *client = styxclientmountphy(channel, 256, "scalar", "", 3000, 4);
        DVM_Provider *mount = client ?
            styxmountprovider(client, FVM_ACCESS_READ | FVM_ACCESS_WRITE | FVM_ACCESS_EXEC) : NULL;
        if (mount)
        {
            result = scalar_runtime_mounted(argc, argv, mount);
            dvm_provider_release(mount);
        }
        shutdown(breaker, SHUT_RDWR);
    }
    close(breaker);
    return result;
}


/* EN: Borrow ordinary runtime arguments and consume the inherited test carrier.
 * Serve incoming records and mount the peer's outgoing file before guest init.
 * RU: Заимствовать параметры runtime и принять унаследованный носитель теста.
 * Опубликовать входящие записи и смонтировать файл другой стороны до init. */
int __wrap_main(int argc, char **argv)
{
    const char *descriptor = getenv("DVM_TEST_SCALAR_FD");
    int owner = getenv("DVM_TEST_SCALAR_OWNER") != NULL;
    int fd = descriptor ? atoi(descriptor) : -1;
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    if (fd >= 3 && getenv("DVM_TEST_SCALAR_MOUNTED_OWNER"))
    {
        return mounted_pair(argc, argv, fd, caps, owner);
    }
    PhyBidir *mux = fd >= 3 ? phybidircreate(phyposixfd(fd, caps), 1024) : NULL;
    if (!mux)
    {
        fprintf(stderr, "scalar pair: carrier initialization failed\n");
        return 2;
    }
    DVM_Provider *image = owner ? scalar_local_image("implementation.dis") : NULL;
    DVM_Provider *owner_profile = owner ? scalar_local_profile() : NULL;
    StyxUnibusServer *receiver = !owner || (image && owner_profile) ?
        styxunibusserverstartsession(phybidirchannel(mux, owner ? 0 : 1),
            owner ? CALL_TAG : REPLY_TAG, image, owner_profile) : NULL;
    dvm_provider_release(owner_profile);
    dvm_provider_release(image);
    StyxClient *client = receiver ? styxclientmountphy(phybidirchannel(mux, owner ? 1 : 0),
        256, "scalar", "", 3000, 4) : NULL;
    DVM_Provider *original = !owner && client && styxclientretain(client) ?
        styxmountproviderat(client, "implementation.dis", FVM_ACCESS_READ | FVM_ACCESS_EXEC) : NULL;
    DVM_Provider *profile = !owner && client && styxclientretain(client) ?
        styxmountproviderat(client, ".interface", FVM_ACCESS_READ) : NULL;
    DVM_Provider *writer = client ? styxmountproviderat(client, ".unibus", FVM_ACCESS_WRITE) : NULL;
    if (!writer || (!owner && (!original || !profile)))
    {
        fprintf(stderr, "scalar pair: file mount failed\n");
        dvm_provider_release(writer);
        dvm_provider_release(original);
        dvm_provider_release(profile);
        phybidirfree(mux);
        styxunibusserverjoin(receiver);
        return 2;
    }
    if (owner && getenv("DVM_TEST_SCALAR_DELAY_OWNER"))
    {
        const struct timespec delay = {1, 0};
        nanosleep(&delay, NULL);
    }
    int status = scalar_runtime_run(argc, argv, writer, styxunibusserverinbox(receiver), original, profile, owner);
    const char *calls = getenv("DVM_TEST_SCALAR_CALL_COUNT");
    int disconnect = getenv("DVM_TEST_SCALAR_DISCONNECT") != NULL;
    if (!owner && !disconnect && status == 0 && calls && !strcmp(calls, "0") &&
        styxscalarclosefile(writer, CALL_TAG) != DVM_LAUNCH_DELIVERED)
    {
        fprintf(stderr, "scalar pair: session close not acknowledged\n");
        status = 2;
    }
    dvm_provider_release(original);
    dvm_provider_release(profile);
    dvm_provider_release(writer);
    if (!owner && !disconnect && status == 0)
    {
        /* Keep the reverse server alive until the owner's Rwrite drains. */
        if (!styxunibusserverjoin(receiver))
        {
            status = 2;
        }
        receiver = NULL;
    }
    phybidirfree(mux);
    return styxunibusserverjoin(receiver) ? status : 2;
}
