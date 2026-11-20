/* Copyright (c) 2026 bhgv
 * EN: Two DisVM processes, each with its own original Styx file server.
 * RU: Два процесса DisVM, каждый со своим исходным файловым сервером Styx.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include "mount.h"
#include "unibus_server.h"
#include "guest_mount.h"

#define CALL_TAG 4294967295UL /* Maximum supported nonzero request tag. */
#define REPLY_TAG 64UL        /* First tag needing a leading magnitude group. */


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
    dvm_provider_release(original);
    dvm_provider_release(profile);
    dvm_provider_release(writer);
    if (!owner && status == 0)
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
