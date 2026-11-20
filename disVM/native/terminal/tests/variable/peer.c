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
#include <inttypes.h>
#include <lib9.h>
#include <styx.h>
#include "variable_server.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "resource.h"
#include "mount.h"
#include "launch/ticket.h"

extern int __real_main(int argc, char **argv); /* Actual hosted DisVM entry. */
extern int __real_dvm_stack_bind(DVM_ResourcePlan *, DVM_ResourcePlan *, unsigned, int64_t); /* Actual mount. */

static pthread_t reader; /* Joined before the fixture exits. */
static StyxVariableServer *server; /* Owns the exported native provider pin. */
static int remote_fd = -1; /* Client carrier end transferred to its thread. */
static uint32_t pid; /* Limbo PID copied from the fixture mount spelling. */
static int legacy; /* Exercise the original unscoped export too. */
static int result; /* Set by the remote reader, consumed after join. */


/* EN/RU: Verify that a scoped peer can read only the selected process identity.
 * Проверить доступ только к идентификатору выбранного процесса. */
static int verify_identity(StyxClient *client)
{
    char path[64], expected[16], bytes[16];
    snprintf(path, sizeof(path), "loaded.dis/.prog/%" PRIu32 "/.pid", pid);
    snprintf(expected, sizeof(expected), "%" PRIu32 "\n", pid);
    StyxFid *identity = styxclientwalk(client, path);
    StyxTransfer transfer = {0};
    if (identity && styxclientopen(identity, OREAD))
    {
        styxclientread(identity, 0, bytes, sizeof(bytes), &transfer);
    }
    styxclientclunk(identity);
    StyxFid *unscoped = styxclientwalk(client, ".var/count");
    StyxFid *other = styxclientwalk(client, "loaded.dis/.prog/0/.var/count");
    int verified = transfer.success && transfer.count == strlen(expected) &&
        !memcmp(bytes, expected, transfer.count) && !unscoped && !other;
    styxclientclunk(unscoped);
    styxclientclunk(other);
    return verified;
}


/* EN/RU: Open the remote cell through the actual receiver's scoped alias.
 * Открыть удалённую ячейку через рабочий псевдоним принимающей стороны. */
static int open_remote(StyxClient *client, DVM_VfsHandle *file)
{
    if (!client || (!legacy && !verify_identity(client)) || !styxclientretain(client))
    {
        return 0;
    }
    DVM_Provider *root = styxmountproviderat(client, legacy ? ".var/count" : "loaded.dis",
        FVM_ACCESS_READ | FVM_ACCESS_WRITE | FVM_ACCESS_EXEC);
    if (!root)
    {
        return 0;
    }
    int opened;
    if (legacy)
    {
        opened = dvm_provider_open(root, ".", "rw", file) == FVM_OK;
    }
    else
    {
        DVM_LaunchHandoff *context = calloc(1, sizeof(*context));
        if (context)
        {
            context->group = dvm_namespace_create("/", "/", NULL);
        }
        DVM_Namespace *group = dvm_launch_handoff_program(context, root, pid, 0, 0) ?
            dvm_launch_program_namespace(context) : NULL;
        DVM_ResourcePlan plan = {0};
        opened = group && dvm_resource_plan(group, "/.prog/.self/.var/count", 0, &plan) &&
            plan.provider && dvm_provider_open(plan.provider, plan.relative, "rw", file) == FVM_OK;
        dvm_resource_close(&plan);
        dvm_namespace_release(group);
        dvm_launch_handoff_free(context);
    }
    dvm_provider_release(root);
    return opened;
}


/* EN/RU: Read a typed owner update, then write through the same remote handle.
 * Прочитать обновление владельца и записать ответ через тот же дескриптор. */
static void *exchange(void *unused)
{
    (void)unused;
    int breaker = dup(remote_fd);
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    StyxClient *client = styxclientmountphy(phyposixfd(remote_fd, caps), 256, "fixture", "", 1500, 4);
    DVM_VfsHandle file = {0};
    if (open_remote(client, &file))
    {
        const unsigned char expected[] = {255, 255, 255, 254};
        unsigned char bytes[4];
        long position;
        for (unsigned attempt = 0; attempt < 300; attempt++)
        {
            if (dvm_vfs_seek(&file, 0, FVM_FILE_SEEK_START, &position) != FVM_OK)
            {
                break;
            }
            DVM_IoResult transfer = dvm_vfs_read_chunk(&file, bytes, sizeof(bytes));
            if (transfer.status != FVM_OK || transfer.count != sizeof(bytes))
            {
                break;
            }
            if (!memcmp(bytes, expected, sizeof(bytes)) &&
                dvm_vfs_seek(&file, 0, FVM_FILE_SEEK_START, &position) == FVM_OK)
            {
                const unsigned char update[] = {0, 1, 0, 3};
                transfer = dvm_vfs_write_chunk(&file, update, sizeof(update));
                result = transfer.status == FVM_OK && (transfer.flags & DVM_IO_PREFIX) &&
                    !(transfer.flags & DVM_IO_UNCERTAIN) && transfer.count == sizeof(update);
                break;
            }
            const struct timespec delay = {0, 10000000};
            nanosleep(&delay, NULL);
        }
    }
    dvm_vfs_close(&file);
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
    if (mounted != 1 || server || strncmp(target->names.path, "/cell.", 6))
    {
        return mounted;
    }
    char *end;
    unsigned long number = strtoul(target->names.path + 6, &end, 10);
    if (*end || !number || number > UINT32_MAX)
    {
        exit(2);
    }
    pid = (uint32_t)number;
    int pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair))
    {
        exit(2);
    }
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    RemotePhy *channel = phyposixfd(pair[0], caps);
    legacy = getenv("DVM_TEST_VARIABLE_LEGACY") != NULL;
    server = legacy ? styxvariableserverstart(channel, "count", source->provider) :
        styxvariableserverstartprogram(channel, "loaded.dis", pid, "count", source->provider);
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
