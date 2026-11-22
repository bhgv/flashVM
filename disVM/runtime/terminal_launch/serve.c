/* Copyright (c) 2026 bhgv
 * EN: Serve one provider-selected Dis launch over a delegated Styx carrier.
 * RU: Обслужить один запуск Dis, выбранный владельцем выданного транспорта Styx.
 */
#include "config.h"
#include "offer.h"
#include "vfs/module_adapter.h"
#include "vfs/cancel.h"
#include "unibus_server.h"
#include "launch_receive.h"
#include "program_record.h"
#include "mount.h"
#include "vfs/launch/runtime.h"
#include "vfs/export.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define OWNER_WAIT_SECONDS 30 /* Bound initial admission even when the peer stays connected. */

typedef struct Startup
{
    unsigned char bytes[64]; /* The one-shot inbox bounds the complete record to 64 bytes. */
    size_t size; /* Exact accepted payload, excluding record header. */
    int scoped; /* Expect the process header before the startup bytes. */
    DVM_LaunchProgram program; /* Decoded caller selector. */
} Startup;


/* EN: Copy a complete payload into the owner's private startup storage.
 * RU: Скопировать полные стартовые данные в частную память владельца. */
static int save_startup(void *state, const void *bytes, size_t size)
{
    Startup *startup = state;
    if (startup->scoped)
    {
        if (!styxlaunchprogramdecode(bytes, size, &startup->program))
        {
            return 0;
        }
        bytes = (const unsigned char *)bytes + STYX_LAUNCH_PROGRAM_HEADER;
        size -= STYX_LAUNCH_PROGRAM_HEADER;
    }
    memcpy(startup->bytes, bytes, size);
    startup->size = size;
    return 1;
}


/* EN: Wait for one validated inbox record or a bounded refusal/EOF.
 * RU: Дождаться одной проверенной записи либо ограниченного отказа или EOF. */
static int receive_startup(DVM_Provider *inbox, Startup *startup, unsigned seconds)
{
    DVM_VfsHandle reader = {0};
    struct timespec start, now;
    if (clock_gettime(CLOCK_MONOTONIC, &start) || dvm_provider_open(inbox, ".", "r", &reader) != FVM_OK)
    {
        return 0;
    }
    StyxLaunchReceiver receiver = {.tag = DVM_STYX_LAUNCH_TAG, .start_payload = save_startup,
        .state = startup, .payload_limit = sizeof(startup->bytes)};
    unsigned char wire[64];
    int accepted = 0;
    do
    {
        DVM_IoResult result = dvm_vfs_read_chunk(&reader, wire, sizeof(wire));
        if (result.status == FVM_OK && (result.flags & DVM_IO_RECORD))
        {
            accepted = styxterminallaunchreceive(&receiver, 0, wire, result.count);
            break;
        }
        if (result.stop != DVM_IO_AGAIN)
        {
            break;
        }
        const struct timespec pause = {0, 10000000};
        nanosleep(&pause, NULL);
    }
    while (!dvm_cancel_requested(dvm_cancel_current()) &&
        (!seconds || (!clock_gettime(CLOCK_MONOTONIC, &now) && now.tv_sec - start.tv_sec < seconds)));
    dvm_vfs_close(&reader);
    return accepted;
}


/* EN: Export the host-selected executable relative to the host working directory.
 * RU: Выдать выбранный владельцем файл относительно рабочего каталога хоста. */
DVM_Provider *dvm_terminal_publish_program(DVM_Namespace *group, const char *path)
{
    DVM_ResourcePlan selection = {0};
    if (!dvm_resource_plan(group, path, 0, &selection))
    {
        return NULL;
    }
    FVM_FileInfo info;
    DVM_Provider *file = dvm_resource_info(&selection, &info) == FVM_OK && !info.directory ?
        dvm_export_native(&selection, 0) : NULL;
    dvm_resource_close(&selection);
    return file;
}


/* EN: Consume an optional library carrier and mount its root in the owner namespace.
 * RU: Принять соединение библиотек и смонтировать его в namespace владельца. */
static int mount_library(DVM_Namespace *group, const char *descriptor, const char *path)
{
    int fd = dvm_terminal_connected(descriptor);
    StyxClient *client = fd >= 3 ? styxclientmountphy(
        phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 4096, "owner", "", 5000, 8) : NULL;
    DVM_Provider *root = client ? styxmountprovider(client, FVM_ACCESS_MASK) : NULL;
    DVM_NamespacePlan target;
    int installed = root && dvm_namespace_plan(group, path, &target) &&
        dvm_namespace_mount_provider(group, &target, root, "/styx-libraries") == 1;
    dvm_provider_release(root);
    return installed;
}


#define LIBRARY_MOUNT_LIMIT 8U /* Maximum library mounts admitted before startup. */

typedef struct LibraryMount
{
    const char *descriptor; /* Borrowed argument naming the delegated stream. */
    const char *path; /* Borrowed owner-side absolute mount path. */
    long number; /* Parsed descriptor used to detect duplicates. */
} LibraryMount;


/* EN: Reject the same path and nested mounts before touching any carrier.
 * RU: Отклонить одинаковые и вложенные точки маунта до обращения к соединениям. */
static int overlaps(const char *first, const char *second)
{
    size_t a = strlen(first), b = strlen(second);
    size_t prefix = a < b ? a : b;
    return !memcmp(first, second, prefix) && (a == b || (a < b ? second[a] : first[b]) == '/');
}


/* EN: Parse and check one mount against the previously accepted prefix.
 * RU: Разобрать и сверить маунт с ранее проверенными параметрами. */
static int admit_library(LibraryMount *mounts, size_t count, const char *launch, const char *fd, const char *path)
{
    char *end;
    long number = strtol(fd, &end, 10);
    if (*end || number < 3 || number > INT_MAX || number == strtol(launch, NULL, 10) ||
        !dvm_terminal_mount_path_valid(path))
    {
        return 0;
    }
    for (size_t i = 0; i < count; i++)
    {
        if (number == mounts[i].number || overlaps(path, mounts[i].path))
        {
            return 0;
        }
    }
    mounts[count] = (LibraryMount){fd, path, number};
    return 1;
}


/* EN: Reject aliases of the same socket before any Styx handshake starts.
 * RU: Отклонить дубликаты одного сокета до начала обмена Styx. */
static int distinct_carriers(int launch, const LibraryMount *mounts, size_t count)
{
    struct stat identities[LIBRARY_MOUNT_LIMIT + 1];
    if (fstat(launch, &identities[0]))
    {
        return 0;
    }
    for (size_t i = 0; i < count; i++)
    {
        struct stat *current = &identities[i + 1];
        int descriptor = dvm_terminal_connected(mounts[i].descriptor);
        if (descriptor < 0)
        {
            return 0;
        }
        int failed = fstat(descriptor, current);
        close(descriptor);
        if (failed)
        {
            return 0;
        }
        for (size_t j = 0; j <= i; j++)
        {
            if (current->st_dev == identities[j].st_dev && current->st_ino == identities[j].st_ino)
            {
                return 0;
            }
        }
    }
    return 1;
}


/* EN: Take one delegated launch and boot only the locally configured Dis image.
 * RU: Принять один запрос и запустить только локально заданный образ Dis. */
int dvm_styx_serve_cli(int argc, char **argv)
{
    int scoped = argc > 3 && !strcmp(argv[3], "--context");
    int separator = scoped ? 4 : 3;
    LibraryMount libraries[LIBRARY_MOUNT_LIMIT];
    size_t count = 0;
    int valid = 1;
    while (valid && argc > separator + 1 && !strcmp(argv[separator], "--library-fd"))
    {
        const char *library = argv[separator + 1];
        const char *path = "/libraries";
        separator += 2;
        if (argc > separator + 1 && !strcmp(argv[separator], "--library-at"))
        {
            path = argv[separator + 1];
            separator += 2;
        }
        valid = count < LIBRARY_MOUNT_LIMIT && admit_library(libraries, count, argv[2], library, path);
        if (valid)
        {
            count++;
        }
    }
    if (!valid || argc <= separator + 1 || strcmp(argv[separator], "--"))
    {
        fprintf(stderr, "usage: disvm --styx-serve-fd FD [--context] [--library-fd FD [--library-at PATH]]... -- [VM OPTIONS] OWNER.dis\n");
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
    if (!distinct_carriers(fd, libraries, count))
    {
        close(breaker);
        close(fd);
        fprintf(stderr, "disvm: launch and library carriers must be distinct connected sockets\n");
        return 2;
    }
    const char *program = argv[argc - 1];
    char cwd[DVM_NS_PATH_LIMIT];
    DVM_LaunchHandoff handoff = {.local_role = DVM_ORIGIN_PROVIDER};
    handoff.group = getcwd(cwd, sizeof(cwd)) ? dvm_namespace_create(cwd, "/", NULL) : NULL;
    DVM_Namespace *host = handoff.group ? dvm_namespace_create("/", cwd, NULL) : NULL;
    DVM_Provider *image = host && dvm_namespace_pin(host) && dvm_namespace_pin(handoff.group) ?
        dvm_terminal_publish_program(host, program) : NULL;
    dvm_namespace_release(host);
    PhyBidir *mux = image ? phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536) : NULL;
    if (!image)
    {
        close(fd);
    }
    const char *declaration = scoped ? DVM_STYX_LAUNCH_CONTEXT : DVM_STYX_LAUNCH_RAW;
    DVM_Provider *profile = mux ? dvm_module_adapter(declaration, strlen(declaration)) : NULL;
    StyxUnibusServer *server = profile ?
        styxunibusserverstartexit(phybidirchannel(mux, 0), DVM_STYX_LAUNCH_TAG, image, profile) : NULL;
    dvm_provider_release(profile);
    int published = image != NULL;
    dvm_provider_release(image);
    StyxClient *reverse = server && scoped ?
        styxclientmountphy(phybidirchannel(mux, 1), 4096, "provider", "", 5000, 8) : NULL;
    DVM_Provider *root = reverse ? styxmountprovider(reverse, FVM_ACCESS_MASK) : NULL;
    Startup startup = {.scoped = scoped};
    int result = 1;
    int ready = server && (!scoped || root);
    int library_ready = ready;
    for (size_t i = 0; library_ready && i < count; i++)
    {
        library_ready = mount_library(handoff.group, libraries[i].descriptor, libraries[i].path);
    }
    int received = library_ready && receive_startup(styxunibusserverinbox(server), &startup, OWNER_WAIT_SECONDS);
    if (received &&
        (!scoped || dvm_launch_handoff_program(&handoff, root, startup.program.pid,
            startup.program.parent_pid, startup.program.hidden_fds)))
    {
        strcpy(handoff.description.module, DVM_STYX_LAUNCH_ENTRY);
        handoff.description.argument_bytes = startup.size;
        handoff.arguments = startup.bytes;
        result = dvm_runtime_main_with_handoff(argc - separator, argv + separator, &handoff);
        styxunibusserverexit(server, result);
    }
    else
    {
        if (received)
        {
            styxunibusserverexit(server, result);
        }
        fprintf(stderr, "%s\n", !published ? "disvm: cannot publish the owner program" :
            !server ? "disvm: cannot start the Styx launch server" :
            !ready ? "disvm: cannot mount reverse launch context" :
            !library_ready ? "disvm: cannot mount Styx library at the configured path" : "disvm: no accepted Styx launch request");
    }
    shutdown(breaker, SHUT_RDWR);
    close(breaker);
    styxunibusserverjoin(server);
    dvm_provider_release(handoff.program);
    dvm_provider_release(handoff.program_parent);
    dvm_provider_release(handoff.program_vfs);
    dvm_provider_release(handoff.program_var);
    dvm_provider_release(root);
    phybidirfree(mux);
    dvm_namespace_release(handoff.group);
    return result;
}

/* EN: Acquire one queued launch and populate the borrowed startup buffer.
 * RU: Принять один запуск из очереди и заполнить заимствованный буфер. */
int dvm_terminal_prepare_shared_launch(StyxUnibusServer *server, DVM_Provider *root,
    DVM_LaunchHandoff *handoff, unsigned char *arguments, StyxLaunchTicket **ticket)
{
    unsigned char wire[64];
    size_t size;
    *ticket = styxunibusservernextlaunch(server, wire, sizeof(wire), &size);
    if (!*ticket)
    {
        return 0;
    }
    Startup startup = {.scoped = 1};
    StyxLaunchReceiver receiver = {.tag = DVM_STYX_LAUNCH_TAG, .start_payload = save_startup,
        .state = &startup, .payload_limit = sizeof(startup.bytes)};
    if (!styxterminallaunchreceive(&receiver, 0, wire, size) ||
        !dvm_launch_handoff_program(handoff, root, startup.program.pid,
            startup.program.parent_pid, startup.program.hidden_fds))
    {
        return -1;
    }
    memcpy(arguments, startup.bytes, startup.size);
    handoff->arguments = arguments;
    handoff->description.argument_bytes = startup.size;
    return 1;
}
