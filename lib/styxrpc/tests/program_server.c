/* Copyright (c) 2026 bhgv
 * EN: Check cross-thread PID registration, slot reuse and stale fids on a live Styx carrier.
 * RU: Проверка межпоточной выдачи PID, повторного использования слотов и старых fid на Styx.
 */
#include "styxrpc/program.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>


/* EN: One PID resource with immutable data and a synchronized close count.
 * RU: Ресурс PID с неизменяемыми данными и синхронизированным счётчиком закрытий. */
typedef struct Probe
{
    char byte;        /* Byte returned by the virtual file. */
    unsigned closes;  /* Number of successful opens already retired. */
} Probe;


/* EN: Return probe's byte at offset zero, or a successful empty slice.
 * RU: Вернуть байт probe при нулевом смещении либо успешный пустой фрагмент. */
static int read_byte(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    const Probe *probe = cookie;
    *size = !offset && capacity ? 1 : 0;
    if (*size)
    {
        *(char *)bytes = probe->byte;
    }
    return 1;
}


/* EN: Refuse writes to the probe without modifying the input or count.
 * RU: Отклонить запись в пробу без изменения входных данных и счётчика. */
static int refuse_write(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    (void)cookie;
    (void)offset;
    (void)bytes;
    (void)count;
    (void)size;
    return 0;
}


/* EN: Count retirement of one exact fid on the dispatcher or revoking thread.
 * RU: Сосчитать отзыв одного fid в диспетчере или отзывающем потоке. */
static void close_probe(void *cookie)
{
    Probe *probe = cookie;
    __atomic_add_fetch(&probe->closes, 1, __ATOMIC_RELAXED);
}


/* EN: Serve requests on the worker and hand server ownership back at disconnect.
 * RU: Обслуживать запросы в рабочем потоке и вернуть владение сервером при обрыве. */
static void *dispatch(void *server)
{
    while (styxrpc_server_step(server))
    {
    }
    return NULL;
}


/* EN: Walk to the exact PID bus on the mounted client and return its unopened fid.
 * RU: Найти точную шину PID у client и вернуть её неоткрытый fid. */
static StyxFid *walk_pid(StyxClient *client, uint32_t pid)
{
    char path[sizeof(".prog/4294967295/.unibus")];
    snprintf(path, sizeof(path), ".prog/%" PRIu32 "/.unibus", pid);
    return styxclientwalk(client, path);
}


/* EN: Publish and revoke on the caller while the worker runs, retaining the stale fid.
 * RU: Опубликовать и отозвать в вызывающем потоке при работающем сервере, сохранив старый fid. */
static int exercise(StyxRpcServer *server, StyxClient *client, Probe *first, Probe *second)
{
    StyxRpcResource resource = {first, NULL, read_byte, refuse_write, close_probe};
    uint32_t original = styxrpc_program_server_grant(server, &resource);
    StyxFid *old = original ? walk_pid(client, original) : NULL;
    int opened = old && styxclientopen(old, 0);
    int full = !styxrpc_program_server_grant(server, &resource);
    int retired = original && styxrpc_program_server_revoke(server, original);
    int once = __atomic_load_n(&first->closes, __ATOMIC_RELAXED) == 1;

    resource.context = second;
    uint32_t replacement = styxrpc_program_server_grant(server, &resource);
    StyxFid *fresh = replacement ? walk_pid(client, replacement) : NULL;
    StyxFid *gone = original ? walk_pid(client, original) : NULL;
    int good = opened && full && retired && once && replacement && replacement != original &&
        !gone && fresh && styxclientopen(fresh, 0);
    if (good)
    {
        StyxTransfer stale, read;
        char byte = 0;
        styxclientread(old, 0, &byte, 1, &stale);
        styxclientread(fresh, 0, &byte, 1, &read);
        good = !stale.success && read.success && read.count == 1 && byte == second->byte;
    }
    styxclientclunk(old);
    styxclientclunk(gone);
    styxclientclunk(fresh);
    int revoked = replacement && styxrpc_program_server_revoke(server, replacement);
    return good && revoked && __atomic_load_n(&first->closes, __ATOMIC_RELAXED) == 1 &&
        __atomic_load_n(&second->closes, __ATOMIC_RELAXED) == 1;
}


#include "program_wait.inc"


/* EN: Serve a five-slot registry, enough for the root and just one grant at a time.
 * RU: Обслужить реестр из пяти слотов, достаточный для корня и одной выдачи PID. */
int main(void)
{
    StyxRpcRegistration nodes[5];
    StyxRpcPrograms programs;
    int sockets[2];
    if (!styxrpc_programs_init(&programs, nodes, sizeof(nodes) / sizeof(nodes[0])) ||
        socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        return 1;
    }
    StyxRpcServer *server = styxrpc_program_server_create(
        phyposixfd(sockets[1], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), &programs);
    pthread_t worker;
    if (!server || pthread_create(&worker, NULL, dispatch, server))
    {
        styxrpc_server_destroy(server);
        close(sockets[0]);
        return 1;
    }
    StyxClient *client = styxclientmountfd(sockets[0], 4096, "test", "", 1000, 8);
    Probe first = {'a', 0}, second = {'b', 0};
    int good = client && exercise(server, client, &first, &second);
    styxclientshutdown(client);
    styxclientfree(client);
    pthread_join(worker, NULL);
    styxrpc_server_destroy(server);
    if (!good)
    {
        fputs("program server did not isolate a revoked PID and its replacement\n", stderr);
    }
    if (!check_program_wait())
    {
        fputs("program-only reverse progress bypassed a fence or failed to resume\n", stderr);
        return 1;
    }
    return good ? 0 : 1;
}
