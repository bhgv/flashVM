/* Copyright (c) 2026 bhgv
 * EN: Exercise the call-scoped virtual PID file over a real reverse Styx session.
 * RU: Проверить виртуальный .pid вызова через встречную сессию Styx.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "client.h"
#include <pthread.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Styxserver server; /* One reverse fixture owner. */
static pthread_t worker; /* Join before returning from the fixture. */
static StyxClient *client; /* Provider's reverse mount client. */
static const Path prog = 1; /* Virtual .prog directory Qid. */
extern size_t scalar_guest_read_caller(uint32_t, const char *, char *, size_t); /* Runtime reads live VFS. */

static const struct Route
{
    const char *name; /* One relative component. */
    unsigned parent; /* Parent selector, zero for the PID directory. */
    const char *file; /* Leaf path relative to the runtime PID directory. */
} routes[] = { /* Bounded fixture tree. */
    {NULL, 0, NULL}, {".pid", 0, ".pid"}, {".fd", 0, NULL}, {"0", 2, ".fd/0"},
    {".vfs", 0, NULL}, {"env", 4, NULL}, {"CLIENT", 5, ".vfs/env/CLIENT"},
    {".var", 0, NULL}, {"counter", 7, ".var/counter"}
};


/* EN: Read one known leaf from the published VFS, using the PID in the Qid.
 * RU: Прочитать лист опубликованной VFS по PID из Qid. */
static size_t content(Path path, char output[12])
{
    unsigned node = path & 15;
    return node < nelem(routes) && routes[node].file ?
        scalar_guest_read_caller((uint32_t)(path >> 4), routes[node].file, output, 12) : 0;
}


/* EN: Walk one component, verifying the selected PID against the runtime.
 * RU: Обойти компонент и сверить выбранный PID с runtime. */
static char *walk(Qid *qid, char *name)
{
    if (qid->path == Qroot && !strcmp(name, ".prog"))
    {
        *qid = styxqid(prog, 1);
        return nil;
    }
    char bytes[12];
    if (qid->path == prog)
    {
        char *end;
        unsigned long long pid = strtoull(name, &end, 10);
        if (*name >= '1' && *name <= '9' && !*end && pid <= UINT32_MAX &&
            content(((Path)pid << 4) | 1, bytes))
        {
            *qid = styxqid((Path)pid << 4, 1);
            return nil;
        }
    }
    else if (qid->path >= 16)
    {
        for (unsigned i = 1; i < nelem(routes); i++)
        {
            if (routes[i].parent == (qid->path & 15) && !strcmp(routes[i].name, name))
            {
                Path path = (qid->path & ~(Path)15) | i;
                if (!routes[i].file || content(path, bytes))
                {
                    *qid = styxqid(path, !routes[i].file);
                    return nil;
                }
            }
        }
    }
    return Enonexist;
}


/* EN: Open a file through the active context in read-only mode.
 * RU: Открыть файл активного контекста только для чтения. */
static char *open_identity(Qid *qid, int mode)
{
    char bytes[12];
    return mode == OREAD && content(qid->path, bytes) ? nil : Eperm;
}


/* EN: Read a slice from the selected runtime file.
 * RU: Прочитать часть выбранного файла runtime. */
static char *read_identity(Qid qid, char *out, ulong *count, vlong offset)
{
    char bytes[12];
    size_t length = content(qid.path, bytes);
    if (!length || offset < 0)
    {
        return Enonexist;
    }
    size_t available = (uvlong)offset < length ? length - (size_t)offset : 0;
    if (*count > available)
    {
        *count = available;
    }
    if (*count)
    {
        memcpy(out, bytes + offset, *count);
    }
    return nil;
}

static Styxops operations = {.walk = walk, .open = open_identity, .read = read_identity}; /* Lookup/read only. */


/* EN: Drive the native server on its exclusive worker.
 * RU: Обслужить нативный сервер на выделенном потоке. */
static void *serve(void *unused)
{
    (void)unused;
    while (!styxwait(&server) && !styxprocess(&server))
    {
    }
    styxend(&server);
    return NULL;
}


/* EN: Start the two endpoints of the reverse test transport.
 * RU: Запустить обе стороны встречного транспорта стенда. */
int scalar_reverse_start(RemotePhy *caller, RemotePhy *provider)
{
    if (styxphyinit(&server, &operations, caller, 0555, 0, 2) ||
        pthread_create(&worker, NULL, serve, NULL))
    {
        return 0;
    }
    client = styxclientmountphy(provider, 256, "provider", "", 3000, 4);
    return client != NULL;
}


/* EN: Compare an exact virtual file to its expected fixture value over Styx.
 * RU: Сверить виртуальный файл с ожидаемым значением через Styx. */
static int check_file(uint32_t pid, const char *relative, const char *expected)
{
    char path[sizeof(".prog/4294967295/.vfs/env/CLIENT")];
    char bytes[12];
    snprintf(path, sizeof(path), ".prog/%" PRIu32 "/%s", pid, relative);
    size_t length = strlen(expected);
    StyxFid *fid = styxclientwalk(client, path);
    if (!fid)
    {
        return 0;
    }
    StyxTransfer transfer = {0};
    if (styxclientopen(fid, OREAD))
    {
        styxclientread(fid, 0, bytes, sizeof(bytes), &transfer);
    }
    styxclientclunk(fid);
    return transfer.success && transfer.count == length && !memcmp(expected, bytes, length);
}


/* EN: Verify the PID and three inherited directory grants in the same call.
 * RU: Проверить PID и три унаследованных каталога в одном вызове. */
int scalar_reverse_check(uint32_t pid)
{
    char expected[12];
    snprintf(expected, sizeof(expected), "%" PRIu32 "\n", pid);
    return check_file(pid, ".pid", expected) && check_file(pid, ".fd/0", "/.fd") &&
        check_file(pid, ".vfs/env/CLIENT", "/.vfs") && check_file(pid, ".var/counter", "/.var");
}


/* EN: Stop the transport and join the reverse server owner.
 * RU: Остановить транспорт и дождаться потока встречного сервера. */
void scalar_reverse_stop(void)
{
    styxclientfree(client);
    pthread_join(worker, NULL);
}
