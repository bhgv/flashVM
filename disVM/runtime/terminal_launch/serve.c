/* Copyright (c) 2026 bhgv
 * EN: Serve one provider-selected Dis launch over a delegated Styx carrier.
 * RU: Обслужить один запуск Dis, выбранный владельцем выданного транспорта Styx.
 */
#include "config.h"
#include "unibus_server.h"
#include "launch_receive.h"
#include "vfs/launch/runtime.h"
#include "vfs/export.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define OWNER_LAUNCH_TAG 17UL /* S-record tag accepted by the delegated one-shot service. */
#define OWNER_WAIT_SECONDS 30 /* Bound initial admission even when the peer stays connected. */

typedef struct Startup
{
    unsigned char bytes[64]; /* The one-shot inbox bounds the complete record to 64 bytes. */
    size_t size; /* Exact accepted payload, excluding record header. */
} Startup;


/* EN: Copy a complete payload into the owner's private startup storage.
 * RU: Скопировать полные стартовые данные в частную память владельца. */
static int save_startup(void *state, const void *bytes, size_t size)
{
    Startup *startup = state;
    memcpy(startup->bytes, bytes, size);
    startup->size = size;
    return 1;
}


/* EN: Wait for one validated inbox record or a bounded refusal/EOF.
 * RU: Дождаться одной проверенной записи либо ограниченного отказа или EOF. */
static int receive_startup(DVM_Provider *inbox, Startup *startup)
{
    DVM_VfsHandle reader = {0};
    struct timespec start, now;
    if (clock_gettime(CLOCK_MONOTONIC, &start) || dvm_provider_open(inbox, ".", "r", &reader) != FVM_OK)
    {
        return 0;
    }
    StyxLaunchReceiver receiver = {.tag = OWNER_LAUNCH_TAG, .start_payload = save_startup,
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
    while (!clock_gettime(CLOCK_MONOTONIC, &now) &&
        now.tv_sec - start.tv_sec < OWNER_WAIT_SECONDS);
    dvm_vfs_close(&reader);
    return accepted;
}


/* EN: Export the host-selected executable relative to the host working directory.
 * RU: Выдать выбранный владельцем файл относительно рабочего каталога хоста. */
static DVM_Provider *publish_program(DVM_Namespace *group, const char *path)
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


/* EN: Take one delegated launch and boot only the locally configured Dis image.
 * RU: Принять один запрос и запустить только локально заданный образ Dis. */
int dvm_styx_serve_cli(int argc, char **argv)
{
    if (argc < 5 || strcmp(argv[3], "--"))
    {
        fprintf(stderr, "usage: disvm --styx-serve-fd FD -- [VM OPTIONS] OWNER.dis\n");
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
    const char *program = argv[argc - 1];
    char cwd[DVM_NS_PATH_LIMIT];
    DVM_LaunchHandoff handoff = {.local_role = DVM_ORIGIN_PROVIDER};
    handoff.group = getcwd(cwd, sizeof(cwd)) ? dvm_namespace_create(cwd, "/", NULL) : NULL;
    DVM_Namespace *host = handoff.group ? dvm_namespace_create("/", cwd, NULL) : NULL;
    DVM_Provider *image = host && dvm_namespace_pin(host) && dvm_namespace_pin(handoff.group) ?
        publish_program(host, program) : NULL;
    dvm_namespace_release(host);
    PhyBidir *mux = image ? phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536) : NULL;
    if (!image)
    {
        close(fd);
    }
    StyxUnibusServer *server = mux ?
        styxunibusserverstartfile(phybidirchannel(mux, 0), OWNER_LAUNCH_TAG, image) : NULL;
    int published = image != NULL;
    dvm_provider_release(image);
    Startup startup = {0};
    int result = 1;
    if (server && receive_startup(styxunibusserverinbox(server), &startup))
    {
        strcpy(handoff.description.module, "implementation.dis");
        handoff.description.argument_bytes = startup.size;
        handoff.arguments = startup.bytes;
        result = dvm_runtime_main_with_handoff(argc - 3, argv + 3, &handoff);
    }
    else
    {
        fprintf(stderr, "%s\n", !published ? "disvm: cannot publish the owner program" :
            !server ? "disvm: cannot start the Styx launch server" : "disvm: no accepted Styx launch request");
    }
    shutdown(breaker, SHUT_RDWR);
    close(breaker);
    styxunibusserverjoin(server);
    phybidirfree(mux);
    dvm_namespace_release(handoff.group);
    return result;
}
