/* Copyright (c) 2026 bhgv
 * EN: Boot a local Dis program with a bidirectional Styx mount at /peer.
 * RU: Запуск локальной программы Dis с двусторонним маунтом Styx в /peer.
 */
#include "config.h"
#include "offer.h"
#include "unibus_server.h"
#include "peer_mount.h"
#include "launch_send.h"
#include "mount.h"
#include "vfs/cancel.h"
#include "vfs/launch/runtime.h"
#include "vfs/export.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include "vfs/program_view.h"
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>


/* EN: Install the mount as a capability in a fresh hosted namespace and run the guest.
 * RU: Установить маунт в новом пространстве имён и запустить программу. */
static int run_mounted(int argc, char **argv, DVM_Provider *mount, const char *path, const int *descriptors)
{
    char cwd[DVM_NS_PATH_LIMIT];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        return 1;
    }
    DVM_LaunchHandoff handoff = {.local_role = DVM_ORIGIN_TERMINAL};
    handoff.group = dvm_namespace_create(cwd, "/", NULL);
    strcpy(handoff.description.module, "styx-mounted");
    DVM_NamespacePlan target;
    int installed = handoff.group && dvm_namespace_pin(handoff.group) &&
        dvm_namespace_plan(handoff.group, path, &target) &&
        dvm_namespace_mount_provider(handoff.group, &target, mount, "/styx-peer") == 1;
    if (installed && descriptors)
    {
        int captured;
        DVM_Provider *fds = dvm_launch_host_fd_view(descriptors, &captured);
        installed = captured && fds && dvm_namespace_plan(handoff.group, "/.fd", &target) &&
            dvm_namespace_mount_provider(handoff.group, &target, fds, "/launch-descriptors") == 1;
        dvm_provider_release(fds);
    }
    int result = installed ? dvm_runtime_main_with_handoff(argc, argv, &handoff) : 1;
    if (!installed)
    {
        fprintf(stderr, "disvm: cannot mount Styx at %s\n", path);
    }
    dvm_namespace_release(handoff.group);
    return result;
}


typedef struct MountedLaunch
{
    DVM_Provider *request; /* Owned exact remote request file. */
    unsigned long tag; /* Record type selected at mount setup. */
    StyxClient *client; /* Borrowed while the routed mount is retained. */
    int wait; /* Use the read/write launch fid when advertised. */
    int scoped; /* Keep the callback tree alive through owner shutdown. */
} MountedLaunch;


/* EN: Drop an owned launch configuration after its last captured mount retires.
 * RU: Освободить конфигурацию запуска после последнего захвата маунта. */
static void release_launch(void *state)
{
    MountedLaunch *launch = state;
    dvm_provider_release(launch->request);
    free(launch);
}


typedef struct ScopedAddress
{
    StyxClient *client; /* Owned connection pin. */
    DVM_LaunchAddress *request; /* Owned process-framed sender. */
    DVM_Provider *completion; /* Optional one-shot status file. */
} ScopedAddress;


/* EN: Retire the sender and the carrier pin.
 * RU: Освободить отправителя и ссылку на транспорт. */
static void release_scoped(void *state)
{
    ScopedAddress *scope = state;
    dvm_provider_release(scope->completion);
    dvm_launch_address_free(scope->request);
    styxclientfree(scope->client);
    free(scope);
}


/* EN: Send once and keep the caller scope available until the one-shot peer exits.
 * RU: Отправить один раз и удерживать контекст до выхода одноразового провайдера. */
static DVM_LaunchDelivery send_scoped(void *state, uint32_t pid, const void *bytes, size_t size)
{
    ScopedAddress *scope = state;
    DVM_LaunchDelivery result = dvm_launch_address_send_process(scope->request, pid, bytes, size);
    if (result != DVM_LAUNCH_NOT_SENT)
    {
        DVM_VfsHandle reader = {0};
        int opened = scope->completion && dvm_provider_open(scope->completion, ".", "r", &reader) == FVM_OK;
        const struct timespec pause = {0, 10000000};
        while (styxclientalive(scope->client) && !dvm_cancel_requested(dvm_cancel_current()))
        {
            if (opened)
            {
                char bytes[8];
                DVM_IoResult status = dvm_vfs_read_chunk(&reader, bytes, sizeof(bytes));
                if (status.status != FVM_OK || status.count)
                {
                    break;
                }
            }
            nanosleep(&pause, NULL);
        }
        dvm_vfs_close(&reader);
    }
    return result;
}


/* EN: Return a fresh address for the same pinned request file.
 * RU: Выдать адрес для того же закреплённого файла запросов. */
static DVM_LaunchAddress *resolve_launch(void *state)
{
    MountedLaunch *launch = state;
    if (launch->wait)
    {
        return styxterminallaunchwaitaddress(launch->request, launch->tag);
    }
    if (!launch->scoped)
    {
        return styxterminallaunchaddress(launch->request, launch->tag);
    }
    ScopedAddress *scope = calloc(1, sizeof(*scope));
    if (!scope || !styxclientretain(launch->client))
    {
        free(scope);
        return NULL;
    }
    scope->client = launch->client;
    if (styxclientretain(launch->client))
    {
        scope->completion = styxmountproviderat(launch->client, ".exit", FVM_ACCESS_READ);
    }
    scope->request = styxterminallaunchcalleraddress(launch->request, launch->tag);
    DVM_LaunchAddress *address = scope->request ?
        dvm_launch_address_process_transport(send_scoped, scope, release_scoped) : NULL;
    if (!address)
    {
        release_scoped(scope);
    }
    return address;
}


/* EN: Read a bounded exact declaration without interpreting untrusted paths or tags.
 * RU: Прочитать точное объявление без разбора недоверенных путей и тегов. */
static const char *discover_launch(StyxClient *client)
{
    if (!styxclientretain(client))
    {
        return NULL;
    }
    DVM_Provider *offer = styxmountproviderat(client, ".interface", FVM_ACCESS_READ);
    DVM_VfsHandle reader = {0};
    if (!offer || dvm_provider_open(offer, ".", "r", &reader) != FVM_OK)
    {
        dvm_provider_release(offer);
        return NULL;
    }
    unsigned char bytes[sizeof(DVM_STYX_LAUNCH_CONTEXT)];
    size_t size = 0;
    const char *profile = NULL;
    while (size < sizeof(bytes))
    {
        DVM_IoResult read = dvm_vfs_read_chunk(&reader, bytes + size, sizeof(bytes) - size);
        if (read.status != FVM_OK || !(read.flags & DVM_IO_PREFIX) || (read.flags & DVM_IO_UNCERTAIN))
        {
            break;
        }
        size += read.count;
        if (!read.count || read.stop == DVM_IO_EOF)
        {
            if (size == sizeof(DVM_STYX_LAUNCH_CONTEXT) - 1 &&
                !memcmp(bytes, DVM_STYX_LAUNCH_CONTEXT, size))
            {
                profile = DVM_STYX_LAUNCH_CONTEXT;
            }
            else if (size == sizeof(DVM_STYX_LAUNCH_RAW) - 1 &&
                !memcmp(bytes, DVM_STYX_LAUNCH_RAW, size))
            {
                profile = DVM_STYX_LAUNCH_RAW;
            }
            break;
        }
    }
    dvm_vfs_close(&reader);
    dvm_provider_release(offer);
    return profile;
}


/* EN: Pin the server's exact request file and determine its completion mode.
 * RU: Закрепить точный файл запроса и определить режим завершения. */
static MountedLaunch *open_launch_route(StyxClient *client, const char *request, unsigned long tag, int scoped)
{
    MountedLaunch *launch = calloc(1, sizeof(*launch));
    if (!launch || !styxclientretain(client))
    {
        free(launch);
        return NULL;
    }
    launch->tag = tag;
    launch->client = client;
    launch->scoped = scoped;
    unsigned access = FVM_ACCESS_WRITE | (scoped ? FVM_ACCESS_READ : 0);
    launch->request = styxmountproviderat(client, request, access);
    if (!launch->request)
    {
        release_launch(launch);
        return NULL;
    }
    FVM_FileInfo info;
    launch->wait = scoped && dvm_provider_info(launch->request, ".", &info) == FVM_OK &&
        !info.directory && (info.mode & 0444);
    return launch;
}


#include "mount_programs.inc"


/* EN: Consume an attached client and configure an exact optional launch route.
 * RU: Принять подключённый клиент и настроить точный маршрут запуска. */
DVM_Provider *dvm_terminal_mount_peer(StyxCallerServer *reverse, StyxClient *client,
    const char *entry, const char *request, unsigned long tag, int scoped)
{
    int automatic = !entry;
    if (automatic)
    {
        const char *profile = discover_launch(client);
        if (!profile)
        {
            return styxcallermount(reverse, client, FVM_ACCESS_MASK);
        }
        entry = DVM_STYX_LAUNCH_ENTRY;
        request = DVM_STYX_LAUNCH_REQUEST;
        tag = DVM_STYX_LAUNCH_TAG;
        scoped = !strcmp(profile, DVM_STYX_LAUNCH_CONTEXT);
    }
    DVM_Provider *mount = mount_launch_programs(client, entry, request, tag, scoped, automatic && scoped);
    if (!mount || !styxcallerbind(reverse, mount))
    {
        dvm_provider_release(mount);
        return NULL;
    }
    return mount;
}


/* EN: Consume a delegated stream duplicate and run with forward channel 0 and reverse channel 1.
 * RU: Принять дубликат выданного потока и запустить с прямым каналом 0 и встречным 1. */
int dvm_styx_mount_cli(int argc, char **argv)
{
    int separator = 3;
    const char *entry = NULL;
    const char *request = NULL;
    unsigned long tag = 0;
    int scoped = argc > 3 && !strcmp(argv[3], "--launch-context");
    if (argc >= 8 && (scoped || !strcmp(argv[3], "--launch")))
    {
        entry = argv[4];
        request = argv[5];
        char *end;
        errno = 0;
        tag = strtoul(argv[6], &end, 10);
        if (errno || argv[6][0] < '0' || argv[6][0] > '9' || *end || !tag || tag > UINT32_MAX)
        {
            fprintf(stderr, "disvm: invalid launch record tag\n");
            return 2;
        }
        separator = 7;
    }
    const char *path = "/peer";
    if (argc > separator + 1 && !strcmp(argv[separator], "--at"))
    {
        path = argv[separator + 1];
        separator += 2;
        if (!dvm_terminal_mount_path_valid(path))
        {
            fprintf(stderr, "disvm: --at requires a canonical absolute Styx mount path\n");
            return 2;
        }
    }
    int descriptors[DVM_LAUNCH_FD_COUNT] = {0};
    int granted = 0;
    while (argc > separator + 1 && (!strcmp(argv[separator], "--fd3") || !strcmp(argv[separator], "--fd")))
    {
        int legacy = !strcmp(argv[separator], "--fd3");
        char *end;
        long number = 3;
        if (!legacy)
        {
            const char *number_text = argv[++separator];
            number = strtol(number_text, &end, 10);
            if (!*number_text || *end || number < 3 || number >= DVM_LAUNCH_FD_COUNT || argc <= separator + 1)
            {
                fprintf(stderr, "disvm: --fd requires a guest FD 3 through %u and a host FD\n",
                    DVM_LAUNCH_FD_COUNT - 1);
                return 2;
            }
        }
        const char *descriptor = argv[separator + 1];
        long supplied = strtol(descriptor, &end, 10);
        struct stat metadata;
        if (!*descriptor || *end || supplied < 3 || supplied > INT_MAX || descriptors[number] ||
            fstat((int)supplied, &metadata) || !S_ISREG(metadata.st_mode))
        {
            fprintf(stderr, "disvm: --fd/--fd3 requires a unique guest number and a delegated regular file FD >= 3\n");
            return 2;
        }
        descriptors[number] = (int)supplied;
        granted = 1;
        separator += 2;
    }
    if (argc <= separator + 1 || strcmp(argv[separator], "--"))
    {
        fprintf(stderr, "usage: disvm --styx-mount-fd FD [--launch|--launch-context ENTRY REQUEST TAG] [--at PATH] [--fd3 FD] [--fd N FD]... -- [VM OPTIONS] BOOTSTRAP.dis\n");
        return 2;
    }
    int fd = dvm_terminal_connected(argv[2]);
    int breaker = fd >= 0 ? dup(fd) : -1;
    if (breaker < 0)
    {
        if (fd >= 0)
        {
            close(fd);
        }
        fprintf(stderr, "disvm: expected a delegated connected stream FD >= 3\n");
        return 2;
    }
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    StyxCallerServer *reverse = mux ? styxcallerserver(phybidirchannel(mux, 1)) : NULL;
    StyxClient *client = reverse ? styxclientmountphy(phybidirchannel(mux, 0), 4096,
        "caller", "", 5000, 8) : NULL;
    DVM_Provider *mount = client ? dvm_terminal_mount_peer(reverse, client, entry, request, tag, scoped) : NULL;
    int result = 1;
    if (mount)
    {
        result = run_mounted(argc - separator, argv + separator, mount, path, granted ? descriptors : NULL);
    }
    else
    {
        fprintf(stderr, "disvm: cannot initialize bidirectional Styx mount\n");
    }
    shutdown(breaker, SHUT_RDWR);
    close(breaker);
    styxcallerjoin(reverse);
    dvm_provider_release(mount);
    phybidirfree(mux);
    return result;
}
