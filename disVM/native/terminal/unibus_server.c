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
    StyxBacking *profile; /* Optional compiled interface record. */
    StyxBacking *original; /* Optional read-only Dis image. */
    StyxBacking *file;   /* Retained backing for write fids. */
};

static __thread StyxUnibusServer *current; /* Set only on the native server thread. */


/* EN/RU: Permit root traversal and write-only access to the published file.
 * Разрешить обход корня и доступ только на запись в опубликованный файл. */
static char *open_node(Qid *qid, int mode)
{
    if (qid->path == Qroot)
    {
        return mode == OREAD ? nil : Eperm;
    }
    StyxBacking *node = qid->path == 1 && mode == OWRITE ? current->file :
        mode != OREAD ? NULL :
        qid->path == 2 ? current->original :
        qid->path == 3 ? current->profile : NULL;
    if (!node)
    {
        return Eperm;
    }
    StyxOpen *opened = styxfileopen(node, mode);
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

/* EN/RU: Return a confirmed prefix from the original Dis image.
 * Вернуть подтверждённый префикс исходного образа Dis. */
static char *read_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    StyxTransfer result;
    if ((qid.path != 2 && qid.path != 3) || offset < 0)
    {
        return Ebadarg;
    }
    styxfileread(styxgetaux(&current->server), offset, bytes, *count, &result);
    if (!result.success || !result.confirmed || result.uncertain || result.count > *count)
    {
        return Ebadarg;
    }
    *count = result.count;
    return nil;
}


static Styxops operations = {.open = open_node, .read = read_node, .write = write_record, .freefid = styxfileclose}; /* Immutable callbacks. */


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


/* EN/RU: Add one optional readable file with its advertised native length.
 * Добавить необязательный читаемый файл с его нативной длиной. */
static int export_file(Styxserver *server, StyxBacking *backing, ulong qid, const char *name)
{
    if (!backing)
    {
        return 1;
    }
    StyxMeta details;
    if (!styxbackinginfo(backing, &details) || details.directory)
    {
        return 0;
    }
    Styxfile *node = styxaddfile(server, Qroot, qid, (char *)name, 0444, "unibus");
    if (!node)
    {
        return 0;
    }
    node->d.length = details.length;
    return 1;
}


/* EN/RU: See unibus_server.h; release partially initialized native resources on failure.
 * См. unibus_server.h; при отказе освободить частично созданные ресурсы. */
static StyxUnibusServer *start(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile, int reusable)
{
    StyxUnibusServer *server = channel ? calloc(1, sizeof(*server)) : NULL;
    if (!server)
    {
        phyfree(channel);
        return NULL;
    }
    server->inbox = reusable ? styxunibusmailbox(tag) : styxunibusinbox(tag);
    server->file = styxbacking(server->inbox);
    server->original = original ? styxbacking(original) : NULL;
    server->profile = profile ? styxbacking(profile) : NULL;
    if (!server->file || (original && !server->original) || (profile && !server->profile))
    {
        phyfree(channel);
    }
    else if (!styxphyinit(&server->server, &operations, channel, 0555, 1, 2))
    {
        if (export_file(&server->server, server->original, 2, "implementation.dis") &&
            export_file(&server->server, server->profile, 3, ".interface") &&
            styxaddfile(&server->server, Qroot, 1, STYX_UNIBUS_NODE, 0222, "unibus") &&
            !pthread_create(&server->thread, NULL, serve, server))
        {
            return server;
        }
        styxend(&server->server);
    }
    styxbackingfree(server->profile);
    styxbackingfree(server->original);
    styxbackingfree(server->file);
    dvm_provider_release(server->inbox);
    free(server);
    return NULL;
}


/* EN/RU: Publish the existing one-shot module endpoint.
 * Опубликовать прежнюю одноразовую точку входа модуля. */
StyxUnibusServer *styxunibusserverstartmodule(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile)
{
    return start(channel, tag, original, profile, 0);
}


/* EN/RU: Publish a reusable endpoint for one serialized scalar session.
 * Опубликовать переиспользуемую точку входа для последовательной сессии. */
StyxUnibusServer *styxunibusserverstartsession(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile)
{
    return start(channel, tag, original, profile, 1);
}


/* EN/RU: Export only the original file and inbox.
 * Опубликовать исходный файл и приёмник. */
StyxUnibusServer *styxunibusserverstartfile(RemotePhy *channel, unsigned long tag, DVM_Provider *original)
{
    return styxunibusserverstartmodule(channel, tag, original, NULL);
}


/* EN/RU: Preserve the default inbox-only server.
 * Сохранить сервер, публикующий только приёмник. */
StyxUnibusServer *styxunibusserverstart(RemotePhy *channel, unsigned long tag)
{
    return styxunibusserverstartfile(channel, tag, NULL);
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
    styxbackingfree(server->profile);
    styxbackingfree(server->original);
    styxbackingfree(server->file);
    dvm_provider_release(server->inbox);
    free(server);
    return 1;
}
