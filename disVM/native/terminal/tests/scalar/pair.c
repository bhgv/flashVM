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
#include "caller_styx.h"
#include "unibus_server.h"
#include "scalar_route.h"
#include <lib9.h>
#include "scalar_function.h"
#include "load_profile.h"
#include "scalar_verify.h"
#include <string.h>
#include "guest_mount.h"

extern DVM_Provider *scalar_reverse_mount(RemotePhy *transport); /* Reverse provider for call contexts. */

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


/* EN: Inspect the live compiled profile independently of ordinary load.
 * RU: Проверить опубликованную схему отдельно от обычного load. */
static int verify_compiled_profile(DVM_Provider *mount)
{
    DVM_VfsHandle handle = {0};
    unsigned char bytes[STYX_SHAPED_OFFER_MAX + 1];
    if (dvm_provider_open(mount, "mounted/blend.dis/.interface", "r", &handle) != FVM_OK)
    {
        return 0;
    }
    size_t size = 0;
    int complete = 0;
    while (size < sizeof(bytes))
    {
        DVM_IoResult got = dvm_vfs_read_chunk(&handle, bytes + size, sizeof(bytes) - size);
        if (got.status != FVM_OK || !(got.flags & DVM_IO_PREFIX) ||
            (got.flags & DVM_IO_UNCERTAIN) || got.count > sizeof(bytes) - size)
        {
            break;
        }
        size += got.count;
        if (!got.count || got.stop == DVM_IO_EOF)
        {
            complete = 1;
            break;
        }
    }
    dvm_vfs_close(&handle);
    StyxCallShape shape;
    int scoped;
    return complete && styxscalar_profile_match_shape(bytes, size, "blend", DVM_CALL_LIST_SIGNATURE, &scoped, &shape) &&
        !scoped && !strcmp(shape.arguments, "(i(ib)b)") && !strcmp(shape.result, "((i(ib)))");
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
    int scoped = getenv("DVM_TEST_SCALAR_SCOPED_SET") != NULL;
    PhyBidir *mux = scoped ? phybidircreate(channel, 1024) : NULL;
    DVM_Provider *reverse = NULL;
    StyxCallerServer *caller = NULL;
    if (scoped)
    {
        channel = mux ? phybidirchannel(mux, 0) : NULL;
        if (owner)
        {
            reverse = mux ? scalar_reverse_mount(phybidirchannel(mux, 1)) : NULL;
        }
        else
        {
            caller = mux ? styxcallerserver(phybidirchannel(mux, 1)) : NULL;
        }
        if (!channel || (!reverse && !caller))
        {
            shutdown(breaker, SHUT_RDWR);
            phyfree(channel);
            dvm_provider_release(reverse);
            styxcallerjoin(caller);
            phybidirfree(mux);
            close(breaker);
            return 2;
        }
    }
    int result = 2;
    if (owner)
    {
        const char *set = getenv("DVM_TEST_SCALAR_LOADED_SET");
        if (set)
        {
            int rejected = !strcmp(set, "reject");
            int dynamic = !strcmp(set, "dynamic");
            const char *concrete_names[] = {"tally", "wide", "pair", "revise", "blend", rejected ? "missing" : "exchange"};
            const char *dynamic_names[] = {"status", "adjust", "project", "fold", "quad", "fractional", "clear", "tally"};
            const char *ref_names[] = {"bump", "revise", "inspect"};
            const char *const *names = dynamic ? dynamic_names : concrete_names;
            size_t count = dynamic ? sizeof(dynamic_names) / sizeof(dynamic_names[0]) :
                sizeof(concrete_names) / sizeof(concrete_names[0]);
            const char *module = dynamic ? "dynamic.dis" : "set.dis";
            const char *implementation = dynamic ? "dynamic-set-owner.dis" : "loaded-set-owner.dis";
            if (!strcmp(set, "compiled-ref"))
            {
                names = ref_names;
                count = sizeof(ref_names) / sizeof(ref_names[0]);
                module = "compiled-ref.dis";
                implementation = "compiled-ref-owner.dis";
            }
            const char *scoped_names[] = {"inspect", "update", "updatepair", "baseline",
                "inspect", "update", "updatepair", "baseline"};
            const char *second = NULL;
            if (scoped)
            {
                names = scoped_names;
                count = sizeof(scoped_names) / sizeof(scoped_names[0]);
                module = "scoped-calls.dis";
                implementation = "scoped-calls-owner.dis";
                second = "scoped-extra-owner.dis";
            }
            StyxUnibusServer *server;
            if (scoped)
            {
                StyxMountedScalar entries[STYX_MOUNTED_EXPORT_LIMIT] = {0};
                for (size_t i = 0; i < count; i++)
                {
                    entries[i].module = i < count / 2 ? module : "scoped-extra.dis";
                    entries[i].name = names[i];
                }
                server = styxunibusserverstartscopedtable(channel, entries, count, reverse);
            }
            else
            {
                server = styxunibusserverstartloadedset(channel, module, names, count);
            }
            if (server)
            {
                DVM_Provider *requests[STYX_MOUNTED_EXPORT_LIMIT];
                DVM_Provider *replies[STYX_MOUNTED_EXPORT_LIMIT];
                DVM_Provider *offers[STYX_MOUNTED_EXPORT_LIMIT];
                for (size_t i = 0; i < count; i++)
                {
                    requests[i] = styxunibusservermailbox(server, i, 0);
                    replies[i] = styxunibusservermailbox(server, i, 1);
                    offers[i] = styxunibusserverloadprofileat(server, i);
                }
                result = scalar_runtime_loaded_set(argc, argv, implementation, second, names, count, requests, replies, offers);
                if (!rejected)
                {
                    shutdown(breaker, SHUT_RDWR);
                }
                if (!styxunibusserverjoin(server))
                {
                    result = 2;
                }
            }
            dvm_provider_release(reverse);
            phybidirfree(mux);
            close(breaker);
            return result;
        }
        if (getenv("DVM_TEST_SCALAR_MIXED_OWNER"))
        {
            const StyxMountedScalar entries[] = {
                {"wide.dis", "count", NULL, 0, 0, 0, 0, 0}, {"wide.dis", "echo", NULL, 1, 0, 0, 0, 0}
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
        const char *published = getenv("DVM_TEST_SCALAR_LOAD_NAME");
        const char *export_name = getenv("DVM_TEST_SCALAR_LOAD_EXPORT");
        if (published && export_name)
        {
            StyxUnibusServer *server = styxunibusserverstartloaded(channel, published, export_name);
            if (server)
            {
                result = scalar_runtime_run(argc, argv, styxunibusserverreply(server),
                    styxunibusserverinbox(server), NULL, styxunibusserverloadprofile(server), 1);
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
            .request_tag = STYX_FUNCTION_REQUEST_TAG, .reply_tag = STYX_FUNCTION_REPLY_TAG, .method = adjust
        };
        const StyxMountedScalar entries[] = {
            {"helper.dis", "offset", &alternate, 0, 0, 0, 0, 0},
            {"compute.dis", "compute", NULL, 0, 0, 0, 0, 0},
            {"compute.dis", "adjust", NULL, 0, 0, 0, 0, 0}
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
        unsigned access = FVM_ACCESS_READ | FVM_ACCESS_WRITE | FVM_ACCESS_EXEC;
        DVM_Provider *mount = !client ? NULL : caller && !getenv("DVM_TEST_SCALAR_UNBOUND_SET") ?
            styxcallermount(caller, client, access) : styxmountprovider(client, access);
        if (mount)
        {
            if (!getenv("DVM_TEST_SCALAR_COMPILED_TYPES") || verify_compiled_profile(mount))
            {
                result = scalar_runtime_mounted(argc, argv, mount);
            }
            else
            {
                fputs("could not verify the exported compiled call types\n", stderr);
            }
            dvm_provider_release(mount);
        }
        shutdown(breaker, SHUT_RDWR);
        styxcallerjoin(caller);
        phybidirfree(mux);
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
