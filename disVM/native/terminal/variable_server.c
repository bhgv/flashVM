/* Copyright (c) 2026 bhgv
 * EN: Original Styx dispatcher for an explicitly granted variable file.
 * RU: Исходный диспетчер Styx для явно выданного файла переменной.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "variable_server.h"
#include "file_provider.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

struct StyxVariableServer
{
    Styxserver server; /* Owns the original dispatcher and channel. */
    StyxBacking *backing; /* Pins the exact published VFS node. */
    pthread_t worker; /* Dispatch thread joined before retirement. */
};

static __thread StyxVariableServer *active; /* Set on the dispatcher only. */


/* EN/RU: Open an exact variable fid with its own handle and destructor.
 * Открыть точный fid переменной со своим дескриптором и освобождением. */
static char *open_node(Qid *qid, int mode)
{
    if (qid->path != 2)
    {
        return mode == OREAD ? nil : Eperm;
    }
    StyxOpen *opened = styxfileopen(active->backing, mode);
    if (!opened || styxsetauxowned(&active->server, opened, styxfileclose) < 0)
    {
        styxfileclose(opened);
        return Eperm;
    }
    return nil;
}


/* EN/RU: Return only a confirmed read from the currently opened variable.
 * Вернуть подтверждённое чтение из открытой переменной. */
static char *read_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (qid.path != 2 || offset < 0)
    {
        return Ebadarg;
    }
    StyxTransfer result;
    styxfileread(styxgetaux(&active->server), offset, bytes, *count, &result);
    if (!result.success || !result.confirmed || result.uncertain || result.count > *count)
    {
        return Ebadarg;
    }
    *count = result.count;
    return nil;
}


/* EN/RU: Forward one write and acknowledge only a confirmed byte count.
 * Передать одну запись и подтвердить только достоверное число байтов. */
static char *write_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (qid.path != 2 || offset < 0)
    {
        return Ebadarg;
    }
    StyxTransfer result;
    styxfilewrite(styxgetaux(&active->server), offset, bytes, *count, &result);
    if (!result.success || !result.confirmed || result.uncertain || result.count > *count)
    {
        return Ebadarg;
    }
    *count = result.count;
    return nil;
}

static Styxops ops = {.open = open_node, .read = read_node, .write = write_node}; /* Shared dispatcher operations. */


/* EN/RU: Retire all fids before the host releases the retained backing.
 * Закрыть все fid до освобождения удерживаемого файла владельцем. */
static void *serve(void *state)
{
    active = state;
    while (!styxwait(&active->server) && !styxprocess(&active->server))
    {
    }
    styxend(&active->server);
    active = NULL;
    return NULL;
}


/* EN/RU: Validate the file and its displayed permissions before starting dispatch.
 * Проверить файл и его права перед запуском диспетчера. */
StyxVariableServer *styxvariableserverstart(RemotePhy *channel, const char *name, DVM_Provider *file)
{
    StyxVariableServer *server = channel && name && *name && strcmp(name, ".") &&
        strcmp(name, "..") && !strchr(name, '/') ? calloc(1, sizeof(*server)) : NULL;
    if (!server)
    {
        phyfree(channel);
        return NULL;
    }
    server->backing = file ? styxbacking(file) : NULL;
    StyxMeta info;
    if (!server->backing || !styxbackinginfo(server->backing, &info) || info.directory || !(info.mode & 0666))
    {
        phyfree(channel);
    }
    else if (!styxphyinit(&server->server, &ops, channel, 0555, 1, 2))
    {
        Styxfile *node = styxadddir(&server->server, Qroot, 1, ".var", 0555, "variable") ?
            styxaddfile(&server->server, 1, 2, (char *)name, info.mode & 0666, "variable") : NULL;
        if (node)
        {
            node->d.length = info.length;
            if (!pthread_create(&server->worker, NULL, serve, server))
            {
                return server;
            }
        }
        styxend(&server->server);
    }
    styxbackingfree(server->backing);
    free(server);
    return NULL;
}


/* EN/RU: Finish server-side ownership after the dispatcher has retired.
 * Освободить владение сервера после остановки диспетчера. */
int styxvariableserverjoin(StyxVariableServer *server)
{
    if (!server || pthread_join(server->worker, NULL))
    {
        return 0;
    }
    styxbackingfree(server->backing);
    free(server);
    return 1;
}
