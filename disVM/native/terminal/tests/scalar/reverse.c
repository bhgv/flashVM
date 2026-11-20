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
extern size_t scalar_guest_read_caller(uint32_t, char *, size_t); /* Runtime reads the live VFS. */


/* EN: Translate test Qids and verify that the runtime still publishes the PID.
 * RU: Перевести Qid стенда и проверить, что runtime ещё публикует PID. */
static size_t identity(Path path, char output[12])
{
    return path >= 2 ? scalar_guest_read_caller((uint32_t)(path >> 1), output, 12) : 0;
}


/* EN: Walk one component in the fixture's virtual caller tree.
 * RU: Обойти один компонент виртуального дерева клиента. */
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
            identity((Path)pid << 1, bytes))
        {
            *qid = styxqid((Path)pid << 1, 1);
            return nil;
        }
    }
    else if (qid->path >= 2 && !(qid->path & 1) && !strcmp(name, ".pid") && identity(qid->path, bytes))
    {
        *qid = styxqid(qid->path | 1, 0);
        return nil;
    }
    return Enonexist;
}


/* EN: Open an active identity file read-only.
 * RU: Открыть активный файл идентификатора только для чтения. */
static char *open_identity(Qid *qid, int mode)
{
    char bytes[12];
    return mode == OREAD && qid->path >= 3 && (qid->path & 1) && identity(qid->path, bytes) ? nil : Eperm;
}


/* EN: Read a slice from the runtime VFS identity file.
 * RU: Прочитать часть файла идентификатора из VFS runtime. */
static char *read_identity(Qid qid, char *out, ulong *count, vlong offset)
{
    char bytes[12];
    size_t length = identity(qid.path, bytes);
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


/* EN: Walk and read the runtime-published identity through ordinary Styx.
 * RU: Пройти по пути и прочитать опубликованный runtime идентификатор через Styx. */
int scalar_reverse_check(uint32_t pid)
{
    char path[sizeof(".prog/4294967295/.pid")];
    char expected[12];
    char bytes[12];
    snprintf(path, sizeof(path), ".prog/%" PRIu32 "/.pid", pid);
    size_t length = (size_t)snprintf(expected, sizeof(expected), "%" PRIu32 "\n", pid);
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


/* EN: Stop the transport and join the reverse server owner.
 * RU: Остановить транспорт и дождаться потока встречного сервера. */
void scalar_reverse_stop(void)
{
    styxclientfree(client);
    pthread_join(worker, NULL);
}
