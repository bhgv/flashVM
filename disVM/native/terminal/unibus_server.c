/* Copyright (c) 2026 bhgv
 * EN: Write-only reverse Styx export backed by the local Unibus inbox.
 * RU: Встречный экспорт Styx для записи в локальный приёмник Unibus.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "unibus_server.h"
#include "file_provider.h"
#include "../launch/request.h"
#include <pthread.h>
#include <stdlib.h>

struct StyxUnibusServer
{
    Styxserver server;   /* Original Styx dispatcher and owned physical channel. */
    pthread_t thread;    /* Exclusive native dispatcher. */
    DVM_Provider *inbox; /* Owner reference for local readers. */
    StyxBacking *file;   /* Retained backing for write fids. */
};

static __thread StyxUnibusServer *current; /* Set only on the native server thread. */


/* EN/RU: Permit root traversal and write-only access to the published file.
 * Разрешить обход корня и доступ только на запись в опубликованный файл. */
static char *open_write(Qid *qid, int mode)
{
    if (qid->path == Qroot)
    {
        return mode == OREAD ? nil : Eperm;
    }
    if (qid->path != 1 || mode != OWRITE)
    {
        return Eperm;
    }
    StyxOpen *opened = styxfileopen(current->file, mode);
    if (!opened || styxsetaux(&current->server, opened) < 0)
    {
        styxfileclose(opened);
        return Eperm;
    }
    return nil;
}


/* EN/RU: Publish exactly the acknowledged complete record.
 * Опубликовать только полностью подтверждённую запись. */
static char *write_record(Qid qid, char *bytes, ulong *count, vlong offset)
{
    StyxTransfer result;
    if (qid.path != 1 || offset < 0)
    {
        return Ebadarg;
    }
    styxfilewrite(styxgetaux(&current->server), offset, bytes, *count, &result);
    return result.success && result.confirmed && !result.uncertain && result.count == *count ? nil : Ebadarg;
}

static Styxops operations = {.open = open_write, .write = write_record, .freefid = styxfileclose}; /* Immutable callbacks. */


/* EN/RU: Dispatch all requests on the server's own thread, then retire all fids.
 * Обрабатывать запросы на потоке сервера, затем закрыть все fid. */
static void *serve(void *arg)
{
    StyxUnibusServer *server = arg;
    current = server;
    while (!styxwait(&server->server) && !styxprocess(&server->server))
    {
    }
    styxend(&server->server);
    current = NULL;
    return NULL;
}


/* EN/RU: See unibus_server.h; release partially initialized native resources on failure.
 * См. unibus_server.h; при отказе освободить частично созданные ресурсы. */
StyxUnibusServer *styxunibusserverstart(RemotePhy *channel, unsigned long tag)
{
    StyxUnibusServer *server = channel ? calloc(1, sizeof(*server)) : NULL;
    if (!server)
    {
        phyfree(channel);
        return NULL;
    }
    server->inbox = styxunibusinbox(tag);
    server->file = styxbacking(server->inbox);
    if (!server->file)
    {
        phyfree(channel);
    }
    else if (!styxphyinit(&server->server, &operations, channel, 0555, 1, 2))
    {
        if (styxaddfile(&server->server, Qroot, 1, STYX_UNIBUS_NODE, 0222, "unibus") &&
            !pthread_create(&server->thread, NULL, serve, server))
        {
            return server;
        }
        styxend(&server->server);
    }
    styxbackingfree(server->file);
    dvm_provider_release(server->inbox);
    free(server);
    return NULL;
}


/* EN/RU: See unibus_server.h; the server retains the original factory.
 * См. unibus_server.h; сервер удерживает исходную фабрику. */
DVM_Provider *styxunibusserverinbox(StyxUnibusServer *server)
{
    return server ? server->inbox : NULL;
}


/* EN/RU: See unibus_server.h; join before releasing the export and factory.
 * См. unibus_server.h; дождаться потока до освобождения экспорта и фабрики. */
int styxunibusserverjoin(StyxUnibusServer *server)
{
    if (!server)
    {
        return 1;
    }
    if (pthread_join(server->thread, NULL))
    {
        return 0;
    }
    styxbackingfree(server->file);
    dvm_provider_release(server->inbox);
    free(server);
    return 1;
}
