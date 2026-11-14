/* Copyright (c) 2026 bhgv
 * EN: Independent bounded local socket control, with source reentry gates.
 * RU: Независимое bounded local socket управление с source reentry gates.
 */
#define _GNU_SOURCE
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "internal.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


/* EN/RU: Remove only this service's original socket inode, never a replacement.
 * Удалить только original socket inode сервиса, не подменённый путь. */
static void remove_socket(void)
{
    struct stat current;
    if (dvm_debug_service.inode && !lstat(dvm_debug_service.path, &current) &&
        current.st_dev == dvm_debug_service.device && current.st_ino == dvm_debug_service.inode &&
        S_ISSOCK(current.st_mode))
    {
        unlink(dvm_debug_service.path);
    }
}


/* EN: Create a private0600 socket before VM threads. Refuse an existing pathname;
 * no stale-path deletion. Restore process umask before starting any extra thread.
 * RU: Создать private0600 socket до VM threads. Отказать существующему пути;
 * без удаления stale path. Вернуть umask до запуска дополнительного thread. */
int dvm_debug_start(const char *path)
{
    struct sockaddr_un address = {0};
    struct stat identity;
    mode_t previous;
    int bound;
    if (!path || !*path || strlen(path) >= sizeof(address.sun_path) || dvm_debug_enabled || !dvm_debug_session_init())
    {
        return 0;
    }
    dvm_debug_service.listener = -1;
    dvm_debug_service.input = -1;
    dvm_debug_service.output = -1;
    dvm_debug_service.lock = fvm_port_sync_create();
    if (!dvm_debug_service.lock)
    {
        return 0;
    }
    snprintf(dvm_debug_service.path, sizeof(dvm_debug_service.path), "%s", path);
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path), "%s", path);
    dvm_debug_service.listener = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    previous = umask(077);
    bound = dvm_debug_service.listener >= 0 &&
            !bind(dvm_debug_service.listener, (struct sockaddr *)&address, sizeof(address));
    umask(previous);
    if (bound && !lstat(path, &identity))
    {
        dvm_debug_service.device = identity.st_dev;
        dvm_debug_service.inode = identity.st_ino;
    }
    if (!bound || !dvm_debug_service.inode || listen(dvm_debug_service.listener, 8) ||
        pthread_create(&dvm_debug_service.thread, NULL, dvm_debug_transport, NULL))
    {
        if (dvm_debug_service.listener >= 0)
        {
            close(dvm_debug_service.listener);
        }
        remove_socket();
        fvm_port_sync_destroy(dvm_debug_service.lock);
        memset(&dvm_debug_service, 0, sizeof(dvm_debug_service));
        return 0;
    }
    dvm_debug_service.started = 1;
    dvm_debug_enabled = 1;
    return 1;
}


/* EN: One bounded client command; slow/disconnected clients never retain VM roots.
 * Poll deadlines bound each read/write; transport storage is private native data.
 * RU: Одна bounded client command; slow/disconnected clients не удерживают VM roots.
 * Poll deadlines ограничивают read/write; storage — private native данные. */
static void client_command(int client)
{
    char input[256];
    char output[4096];
    size_t used = 0;
    size_t sent = 0;
    unsigned int attempts;
    for (attempts = 0; attempts < 4 && used + 1 < sizeof(input); attempts++)
    {
        struct pollfd wait = {client, POLLIN, 0};
        ssize_t bytes;
        if (poll(&wait, 1, 25) <= 0)
        {
            continue;
        }
        bytes = recv(client, input + used, sizeof(input) - used - 1, 0);
        if (bytes <= 0)
        {
            return;
        }
        used += (size_t)bytes;
        input[used] = 0;
        if (strchr(input, '\n'))
        {
            *strchr(input, '\n') = 0;
            dvm_debug_command(input, output, sizeof(output));
            break;
        }
    }
    if (attempts == 4 || !used || used + 1 == sizeof(input))
    {
        return;
    }
    used = strlen(output);
    for (attempts = 0; attempts < 4 && sent < used; attempts++)
    {
        struct pollfd wait = {client, POLLOUT, 0};
        ssize_t bytes;
        if (poll(&wait, 1, 25) <= 0)
        {
            continue;
        }
        bytes = send(client, output + sent, used - sent, MSG_NOSIGNAL);
        if (bytes > 0)
        {
            sent += (size_t)bytes;
        }
    }
}


/* EN/RU: Serve native control independently of every guest; detach inspector on exit.
 * Обслуживать native control независимо от guests; отключить inspector при выходе. */
void *dvm_debug_transport(void *argument)
{
    (void)argument;
    if (dvm_debug_service.input >= 0)
    {
        dvm_debug_pipe_loop();
        dvm_debug_inspector_stop();
        return NULL;
    }
    while (!__atomic_load_n(&dvm_debug_service.stopping, __ATOMIC_ACQUIRE))
    {
        struct pollfd wait = {dvm_debug_service.listener, POLLIN, 0};
        int client;
        if (poll(&wait, 1, 25) <= 0)
        {
            continue;
        }
        client = accept4(dvm_debug_service.listener, NULL, NULL, SOCK_CLOEXEC | SOCK_NONBLOCK);
        if (client >= 0)
        {
            client_command(client);
            close(client);
        }
    }
    dvm_debug_inspector_stop();
    return NULL;
}


/* EN: Release every debug gate and join transport before GC shutdown. Disconnect
 * alone never calls this function and never resumes a retained continuation.
 * RU: Освободить debug gates и завершить transport до GC shutdown. Disconnect
 * сам по себе не вызывает эту функцию и не возобновляет retained continuation. */
void dvm_debug_quiesce(void)
{
    DebugTarget *target;
    if (!dvm_debug_enabled)
    {
        return;
    }
    fvm_port_sync_lock(dvm_debug_service.lock);
    for (target = dvm_debug_service.targets; target; target = target->next)
    {
        fvm_port_sync_lock(target->wake);
        target->closing = 1;
        fvm_port_sync_signal_all(target->wake);
        fvm_port_sync_unlock(target->wake);
    }
    fvm_port_sync_unlock(dvm_debug_service.lock);
    __atomic_store_n(&dvm_debug_service.stopping, 1, __ATOMIC_RELEASE);
    if (dvm_debug_service.started)
    {
        pthread_join(dvm_debug_service.thread, NULL);
        dvm_debug_service.started = 0;
    }
}


/* EN/RU: After all source/inspector joins, free cards and owned socket/mutex.
 * После source/inspector joins удалить cards и собственные socket/mutex. */
void dvm_debug_destroy(void)
{
    DebugTarget *target;
    if (!dvm_debug_enabled)
    {
        return;
    }
    while ((target = dvm_debug_service.targets) != NULL)
    {
        dvm_debug_service.targets = target->next;
        fvm_port_sync_destroy(target->wake);
        free(target);
        __atomic_fetch_add(&dvm_debug_service.freed, 1, __ATOMIC_RELAXED);
    }
    if (dvm_debug_service.listener >= 0)
    {
        close(dvm_debug_service.listener);
    }
    if (dvm_debug_service.input >= 0)
    {
        close(dvm_debug_service.input);
        if (dvm_debug_service.output >= 0)
        {
            close(dvm_debug_service.output);
        }
    }
    remove_socket();
    fvm_port_sync_destroy(dvm_debug_service.lock);
    fprintf(
        stderr,
        "NATIVEDEBUGGER made=%lu freed=%lu faults=%lu inspections=%lu inspector_joined=1 supervisor_joined=1 unhandled=%lu\n",
        __atomic_load_n(&dvm_debug_service.made, __ATOMIC_RELAXED),
        __atomic_load_n(&dvm_debug_service.freed, __ATOMIC_RELAXED),
        __atomic_load_n(&dvm_debug_service.faults, __ATOMIC_RELAXED),
        __atomic_load_n(&dvm_debug_service.inspections, __ATOMIC_RELAXED),
        __atomic_load_n(&dvm_debug_service.unhandled, __ATOMIC_RELAXED));
    memset(&dvm_debug_service, 0, sizeof(dvm_debug_service));
    dvm_debug_enabled = 0;
}
