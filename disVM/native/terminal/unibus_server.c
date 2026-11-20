/* Copyright (c) 2026 bhgv
 * EN: Write-only reverse Styx export backed by the local Unibus inbox.
 * RU: Встречный экспорт Styx для записи в локальный приёмник Unibus.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "scalar_function.h"
#include "scalar_function_owner.h"
#include "scalar_verify.h"
#include "flashvm/srecord.h"
#include "unibus_server.h"
#include "file_provider.h"
#include "../launch/request.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

struct StyxUnibusServer
{
    StyxScalarFunctionOwner *owner; /* Optional owned Dis service routes. */
    size_t interface_size; /* Byte count advertised for the mounted profile. */
    unsigned char interface[STYX_SCALAR_INTERFACE_CAPACITY]; /* Immutable profile. */
    StyxScalarFunction function; /* Optional mounted scalar export. */
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
    if (qid->path == 6)
    {
        return styxscalarfunctionopen(&current->server, mode, &current->function, current->owner != NULL);
    }
    if (qid->path == Qroot || qid->path == 4 || qid->path == 5 || qid->path == 7)
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
    if (qid.path == 6)
    {
        return styxscalarfunctionwrite(styxgetaux(&current->server), bytes, *count, offset);
    }
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
    if (qid.path == 6)
    {
        return styxscalarfunctionread(styxgetaux(&current->server), bytes, count, offset);
    }
    if (qid.path == 7 && offset >= 0)
    {
        size_t available = (uvlong)offset < current->interface_size ?
            current->interface_size - (size_t)offset : 0;
        if (*count > available)
        {
            *count = available;
        }
        if (*count)
        {
            memcpy(bytes, current->interface + offset, *count);
        }
        return nil;
    }
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


/* EN/RU: Release the open context according to the server's active export mode.
 * Освободить контекст открытия согласно режиму экспорта сервера. */
static void close_node(void *state)
{
    if (current->function.method)
    {
        free(state);
    }
    else
    {
        styxfileclose(state);
    }
}


static Styxops operations = {.open = open_node, .read = read_node, .write = write_record, .freefid = close_node}; /* Immutable callbacks. */


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
    styxunibusmailboxseal(server->owner ? server->owner->request : server->inbox);
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


/* EN/RU: Add the function directory and its compiler-signature record.
 * Добавить каталог функции и запись сигнатуры компилятора. */
static int export_function(StyxUnibusServer *server, const char *module, const char *name)
{
    if (!styxadddir(&server->server, Qroot, 4, "mounted", 0555, "unibus") ||
        !styxadddir(&server->server, 4, 5, (char *)module, 0555, "unibus") ||
        !styxaddfile(&server->server, 5, 6, (char *)name, 0777, "unibus"))
    {
        return 0;
    }
    if (server->function.request_tag != STYX_FUNCTION_REQUEST_TAG ||
        server->function.reply_tag != STYX_FUNCTION_REPLY_TAG)
    {
        return 1;
    }
    unsigned char header[INTERFACE_HEADER];
    styxscalar_interface_header(header, DVM_SCALAR_INT_SIGNATURE);
    size_t length = styxscalar_name_length(name);
    if (!length || fvm_srecord_write_pair(STYX_FUNCTION_PROFILE_TAG, header, sizeof(header),
            name, length, server->interface, sizeof(server->interface), &server->interface_size) != FVM_S_OK)
    {
        return 0;
    }
    Styxfile *profile = styxaddfile(&server->server, 5, 7, ".interface", 0444, "unibus");
    if (!profile)
    {
        return 0;
    }
    profile->d.length = server->interface_size;
    return 1;
}


/* EN/RU: See unibus_server.h; release partially initialized native resources on failure.
 * См. unibus_server.h; при отказе освободить частично созданные ресурсы. */
static StyxUnibusServer *start(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile, int reusable,
    const char *module, const char *name, const StyxScalarFunction *function, StyxScalarFunctionOwner *owner)
{
    StyxUnibusServer *server = channel ? calloc(1, sizeof(*server)) : NULL;
    if (!server)
    {
        phyfree(channel);
        return NULL;
    }
    if (function)
    {
        server->function = *function;
        server->owner = owner;
        if (!styxphyinit(&server->server, &operations, channel, 0555, 1, 2))
        {
            if (export_function(server, module, name) &&
                !pthread_create(&server->thread, NULL, serve, server))
            {
                return server;
            }
            styxend(&server->server);
        }
        free(server);
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
    return start(channel, tag, original, profile, 0, NULL, NULL, NULL, NULL);
}


/* EN/RU: Publish a reusable endpoint for one serialized scalar session.
 * Опубликовать переиспользуемую точку входа для последовательной сессии. */
StyxUnibusServer *styxunibusserverstartsession(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile)
{
    return start(channel, tag, original, profile, 1, NULL, NULL, NULL, NULL);
}


/* EN/RU: Validate a single non-special Styx path component.
 * Проверить один обычный элемент пути Styx. */
static int node_name(const char *name)
{
    return name && *name && !strchr(name, '/') && strcmp(name, ".") && strcmp(name, "..");
}


/* EN/RU: Publish the named module and function on a consumed channel.
 * Опубликовать модуль и функцию с заданными именами на принятом канале. */
StyxUnibusServer *styxunibusserverstartfunction(RemotePhy *channel, const char *module,
    const char *name, const StyxScalarFunction *function)
{
    if (!node_name(module) || !node_name(name) || !function || !function->method ||
        !strcmp(name, ".interface") || !function->request_tag || !function->reply_tag ||
        function->request_tag > UINT32_MAX || function->reply_tag > UINT32_MAX)
    {
        phyfree(channel);
        return NULL;
    }
    return start(channel, 0, NULL, NULL, 0, module, name, function, NULL);
}


/* EN/RU: Release owner inboxes after the dispatcher has stopped.
 * Освободить приёмники владельца после остановки диспетчера. */
static void free_owner(StyxScalarFunctionOwner *owner)
{
    if (owner)
    {
        dvm_provider_release(owner->request);
        dvm_provider_release(owner->reply);
        free(owner);
    }
}


/* EN/RU: Publish one named export backed by the shared Dis service.
 * Опубликовать один именованный экспорт через общий сервис Dis. */
StyxUnibusServer *styxunibusserverstartdis(RemotePhy *channel, const char *module, const char *name)
{
    StyxScalarFunctionOwner *owner = calloc(1, sizeof(*owner));
    if (owner)
    {
        owner->request = styxunibusmailbox(STYX_FUNCTION_REQUEST_TAG);
        owner->reply = styxunibusmailbox(STYX_FUNCTION_REPLY_TAG);
    }
    if (!owner || !owner->request || !owner->reply || !node_name(module) ||
        !node_name(name) || !strcmp(name, ".interface"))
    {
        phyfree(channel);
        free_owner(owner);
        return NULL;
    }
    const StyxScalarFunction function = {
        STYX_FUNCTION_REQUEST_TAG, STYX_FUNCTION_REPLY_TAG, styxscalarfunctiondispatch, owner
    };
    StyxUnibusServer *server = start(channel, 0, NULL, NULL, 0, module, name, &function, owner);
    if (!server)
    {
        free_owner(owner);
    }
    return server;
}


/* EN/RU: Borrow the service's write mailbox through server join.
 * Заимствовать приёмник записи сервиса до завершения сервера. */
DVM_Provider *styxunibusserverreply(StyxUnibusServer *server)
{
    return server && server->owner ? server->owner->reply : NULL;
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
    return !server ? NULL : server->owner ? server->owner->request : server->inbox;
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
    free_owner(server->owner);
    free(server);
    return 1;
}
