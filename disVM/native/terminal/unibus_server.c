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
#include "load_profile.h"
#include "flashvm/srecord.h"
#include "unibus_server.h"
#include "file_provider.h"
#include "../launch/request.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "unibus_server_state.h"
#include "program_record.h"
#include "cancel.h"
#include <time.h>
#include "unibus_launch_queue.inc"
static __thread StyxUnibusServer *current; /* Set only on the native server thread. */


/* EN/RU: Identify a mounted entry by qid and its offset within the module.
 * Найти запись смонтированного модуля по qid и смещению внутри модуля. */
static MountedExport *mounted_node(Path path, unsigned offset)
{
    if (path < FIRST_MODULE)
    {
        return NULL;
    }
    Path index = path - FIRST_MODULE;
    return index % 3 == offset && index / 3 < current->count ?
        &current->modules[index / 3] : NULL;
}


/* EN: Recognize the per-module field delta capability file.
 * RU: Распознать файл поддержки дельт полей в каталоге модуля. */
static int delta_node(Path path)
{
    Path first = LAUNCH_EXIT + 1;
    return current->loaded && path >= first && path - first < current->count;
}


typedef struct LaunchFid
{
    StyxOpen *writer; /* Owned legacy inbox backing. */
    StyxLaunchTicket *ticket; /* Owned result for a persistent read/write fid. */
    int waiting; /* Read/write fid admitted to observe completion. */
    int submitted; /* Set only after the complete request is accepted. */
} LaunchFid;


/* EN: Consume the file context attached to a launch fid.
 * RU: Освободить файловый контекст, прикреплённый к fid запуска. */
static void close_launch_file(void *state)
{
    LaunchFid *fid = state;
    close_launch_ticket(fid->ticket);
    styxfileclose(fid->writer);
    free(fid);
}


/* EN/RU: Permit root traversal and write-only access to the published file.
 * Разрешить обход корня и доступ только на запись в опубликованный файл. */
static char *open_node(Qid *qid, int mode)
{
    MountedExport *module = mounted_node(qid->path, 1);
    if (module)
    {
        if (mode != ORDWR || !styxunibusawaitload(current))
        {
            return Eperm;
        }
        return styxscalarfunctionopen(&current->server, mode, &module->function, module->owner.request != NULL,
            module->function.program_root && current->loaded ? &current->function_workers : NULL, &module->busy);
    }
    if (delta_node(qid->path))
    {
        return mode == OREAD ? nil : Eperm;
    }
    if (mounted_node(qid->path, 2))
    {
        return mode == OREAD && styxunibusawaitload(current) ? nil : Eperm;
    }
    if (qid->path == Qroot || qid->path == MOUNTED_ROOT || (current->completion && qid->path == LAUNCH_EXIT) || mounted_node(qid->path, 0))
    {
        return mode == OREAD ? nil : Eperm;
    }
    if (qid->path == 1 && (mode == OWRITE || (mode == ORDWR && current->completion)))
    {
        LaunchFid *fid = calloc(1, sizeof(*fid));
        if (!fid)
        {
            return Eperm;
        }
        if (!current->launches)
        {
            fid->writer = styxfileopen(current->file, OWRITE);
        }
        fid->waiting = mode == ORDWR;
        if ((!current->launches && !fid->writer) || styxsetauxowned(&current->server, fid, close_launch_file) < 0)
        {
            close_launch_file(fid);
            return Eperm;
        }
        return nil;
    }
    StyxBacking *node = mode != OREAD ? NULL :
        qid->path == 2 ? current->original :
        qid->path == 3 ? current->profile : NULL;
    if (!node)
    {
        return Eperm;
    }
    LaunchFid *fid = calloc(1, sizeof(*fid));
    if (!fid)
    {
        return Eperm;
    }
    fid->writer = styxfileopen(node, mode);
    if (!fid->writer || styxsetauxowned(&current->server, fid, close_launch_file) < 0)
    {
        close_launch_file(fid);
        return Eperm;
    }
    return nil;
}


/* EN/RU: Publish exactly the acknowledged complete record.
 * Опубликовать только полностью подтверждённую запись. */
static char *write_record(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (mounted_node(qid.path, 1))
    {
        return styxscalarfunctionwrite(styxgetaux(&current->server), bytes, *count, offset);
    }
    StyxTransfer result;
    if (qid.path != 1 || offset < 0)
    {
        return Ebadarg;
    }
    LaunchFid *fid = styxgetaux(&current->server);
    if (!fid || (fid->waiting && (offset || fid->submitted)))
    {
        return Ebadarg;
    }
    if (current->launches)
    {
        if (offset || fid->submitted || !enqueue_launch(current->launches, bytes, *count,
                fid->waiting, &fid->ticket))
        {
            return Ebadarg;
        }
        fid->submitted = 1;
        return nil;
    }
    styxfilewrite(fid->writer, offset, bytes, *count, &result);
    if (!result.success || !result.confirmed || result.uncertain || result.count != *count)
    {
        return Ebadarg;
    }
    fid->submitted = 1;
    return nil;
}

/* EN/RU: Return a confirmed prefix from the original Dis image.
 * Вернуть подтверждённый префикс исходного образа Dis. */
static char *read_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (mounted_node(qid.path, 1))
    {
        return styxscalarfunctionread(styxgetaux(&current->server), bytes, count, offset);
    }
    if (delta_node(qid.path) && offset >= 0)
    {
        static const char capability[] = STYX_MANAGED_SPARSE_GRAPH_MARKER;
        size_t remaining = offset < STYX_MANAGED_GRAPH_VERSION ?
            STYX_MANAGED_GRAPH_VERSION - (size_t)offset : 0;
        if (*count > remaining)
        {
            *count = remaining;
        }
        if (*count)
        {
            memcpy(bytes, capability + offset, *count);
        }
        return nil;
    }
    MountedExport *module = mounted_node(qid.path, 2);
    if (module && offset >= 0)
    {
        if (current->loaded && current->load_state != 1)
        {
            return Eperm;
        }
        size_t available = (uvlong)offset < module->interface_size ?
            module->interface_size - (size_t)offset : 0;
        if (*count > available)
        {
            *count = available;
        }
        if (*count)
        {
            memcpy(bytes, module->interface + offset, *count);
        }
        return nil;
    }
    if (qid.path == 1 && current->completion && offset >= 0)
    {
        LaunchFid *fid = styxgetaux(&current->server);
        if (!fid || !fid->waiting || !fid->submitted)
        {
            return Eperm;
        }
        *count = current->launches ? launch_ticket_read(fid->ticket, bytes, *count, offset) :
            styxunibusexitread(current, bytes, *count, offset);
        return nil;
    }
    if (current->completion && qid.path == (current->count ? LAUNCH_EXIT : MOUNTED_ROOT) && offset >= 0)
    {
        *count = current->launches ? launch_ticket_read(current->launches->legacy, bytes, *count, offset) :
            styxunibusexitread(current, bytes, *count, offset);
        return nil;
    }
    StyxTransfer result;
    if ((qid.path != 2 && qid.path != 3) || offset < 0)
    {
        return Ebadarg;
    }
    LaunchFid *fid = styxgetaux(&current->server);
    styxfileread(fid ? fid->writer : NULL, offset, bytes, *count, &result);
    if (!result.success || !result.confirmed || result.uncertain || result.count > *count)
    {
        return Ebadarg;
    }
    *count = result.count;
    return nil;
}


static Styxops operations = {.open = open_node, .read = read_node, .write = write_record}; /* Immutable callbacks. */


/* EN/RU: Dispatch all requests on the server's own thread, then retire all fids.
 * Обрабатывать запросы на потоке сервера, затем закрыть все fid. */
static void *serve(void *arg)
{
    StyxUnibusServer *server = arg;
    current = server;
    while (!styxwait(&server->server) && !styxprocess(&server->server))
    {
    }
    seal_launch_queue(server->launches);
    styxend(&server->server);
    styxunibusmailboxseal(server->inbox);
    for (size_t i = 0; i < server->count; i++)
    {
        styxunibusmailboxseal(server->modules[i].owner.request);
        styxunibusmailboxseal(server->modules[i].load_offer);
    }
    current = NULL;
    __atomic_store_n(&server->stopped, 1, __ATOMIC_RELEASE);
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
static int export_function(StyxUnibusServer *server, size_t index, size_t leader, const StyxMountedScalar *entry)
{
    MountedExport *module = &server->modules[index];
    MountedExport *directory = &server->modules[leader];
    Path qid = FIRST_MODULE + index * 3;
    Path parent = FIRST_MODULE + leader * 3;
    if ((leader == index && !styxadddir(&server->server, MOUNTED_ROOT, parent, (char *)entry->module, 0555, "unibus")) ||
        !styxaddfile(&server->server, parent, qid + 1, (char *)entry->name, 0777, "unibus"))
    {
        return 0;
    }
    if (server->loaded)
    {
        if (!directory->profile_node)
        {
            directory->profile_node = styxaddfile(&server->server, parent, parent + 2, ".interface", 0444, "unibus");
        }
        if (leader == index)
        {
            Styxfile *marker = styxaddfile(&server->server, parent, LAUNCH_EXIT + 1 + leader,
                STYX_MANAGED_DELTA_FILE, 0444, "unibus");
            if (!marker)
            {
                return 0;
            }
            marker->d.length = STYX_MANAGED_GRAPH_VERSION;
        }
        return directory->profile_node != NULL;
    }
    uint32_t signature = styxscalar_request_signature(module->function.request_tag);
    if (!signature || module->function.reply_tag != styxfunction_reply_tag(module->function.request_tag))
    {
        return 1;
    }
    unsigned char header[INTERFACE_HEADER];
    styxscalar_interface_header(header, signature);
    size_t length = styxscalar_name_length(entry->name);
    size_t used;
    unsigned long tag = (module->function.caller_method || module->function.program_root) ? STYX_FUNCTION_CALLER_PROFILE : STYX_FUNCTION_PROFILE_TAG;
    if (!length || fvm_srecord_write_pair(tag, header, sizeof(header), entry->name, length,
            directory->interface + directory->interface_size,
            sizeof(directory->interface) - directory->interface_size, &used) != FVM_S_OK)
    {
        return 0;
    }
    if (!directory->profile_node)
    {
        directory->profile_node = styxaddfile(&server->server, parent, parent + 2, ".interface", 0444, "unibus");
        if (!directory->profile_node)
        {
            return 0;
        }
    }
    directory->interface_size += used;
    directory->profile_node->d.length = directory->interface_size;
    return 1;
}


/* EN/RU: See unibus_server.h; release partially initialized native resources on failure.
 * См. unibus_server.h; при отказе освободить частично созданные ресурсы. */
StyxUnibusServer *styxunibusstart(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile, int reusable, int completion)
{
    StyxUnibusServer *server = channel ? calloc(1, sizeof(*server)) : NULL;
    if (!server)
    {
        phyfree(channel);
        return NULL;
    }
    server->completion = completion;
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
            (!completion || styxaddfile(&server->server, Qroot, MOUNTED_ROOT, ".exit", 0444, "unibus")) &&
            styxaddfile(&server->server, Qroot, 1, STYX_UNIBUS_NODE, completion ? 0666 : 0222, "unibus") &&
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


StyxUnibusServer *styxunibusserverstartmodule(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile)
{
    return styxunibusstart(channel, tag, original, profile, 0, 0);
}


/* EN/RU: Publish a reusable endpoint for one serialized scalar session.
 * Опубликовать переиспользуемую точку входа для последовательной сессии. */
StyxUnibusServer *styxunibusserverstartsession(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile)
{
    return styxunibusstart(channel, tag, original, profile, 1, 0);
}


/* EN/RU: Validate a single non-special Styx path component.
 * Проверить один обычный элемент пути Styx. */
static int node_name(const char *name)
{
    return name && *name && !strchr(name, '/') && strcmp(name, ".") && strcmp(name, "..");
}


/* EN/RU: Drop all allocated routes and the array after server stop or setup refusal.
 * Освободить все выделенные маршруты и массив после остановки или отказа запуска. */
static void free_modules(StyxUnibusServer *server)
{
    for (size_t i = 0; i < server->count; i++)
    {
        dvm_provider_release(server->modules[i].owner.request);
        dvm_provider_release(server->modules[i].owner.reply);
        dvm_provider_release(server->modules[i].load_offer);
    }
    free(server->modules);
}


/* EN/RU: Validate one borrowed entry and reserve its private dispatch routes.
 * Проверить заимствованную запись и зарезервировать её собственные маршруты. */
static int configure(MountedExport *module, const StyxMountedScalar *entry, int loaded)
{
    if (!node_name(entry->module) || !node_name(entry->name) || !strcmp(entry->name, ".interface") ||
        !strcmp(entry->name, STYX_MANAGED_DELTA_FILE))
    {
        return 0;
    }
    if (loaded)
    {
        return styxunibusprepareload(module, entry->name);
    }
    if (entry->native)
    {
        module->function = *entry->native;
        return styxscalarfunctionvalid(&module->function);
    }
    return styxscalarownerconfigure(&module->owner, &module->function, entry->dis_big, entry->dis_tuple, entry->dis_ref, entry->dis_call, entry->dis_ref_pair);
}

/* EN/RU: Export all distinct entries before making the dispatcher visible.
 * Опубликовать различные записи до запуска диспетчера. */
static int export_modules(StyxUnibusServer *server, const StyxMountedScalar *entries, int loaded)
{
    if (!styxadddir(&server->server, Qroot, MOUNTED_ROOT, "mounted", 0555, "unibus"))
    {
        return 0;
    }
    for (size_t i = 0; i < server->count; i++)
    {
        if (!configure(&server->modules[i], &entries[i], loaded))
        {
            return 0;
        }
        size_t leader = i, exports = 1;
        for (size_t j = 0; j < i; j++)
        {
            if (!strcmp(entries[i].module, entries[j].module))
            {
                if (++exports > STYX_MOUNTED_EXPORT_LIMIT || !strcmp(entries[i].name, entries[j].name))
                {
                    return 0;
                }
                if (leader == i)
                {
                    leader = j;
                }
            }
        }
        server->modules[i].directory_leader = leader;
        if (!export_function(server, i, leader, &entries[i]))
        {
            return 0;
        }
    }
    return 1;
}

/* EN/RU: Consume the channel and publish a bounded table on one Styx session.
 * Принять канал и опубликовать ограниченную таблицу в одной сессии Styx. */
StyxUnibusServer *styxunibusstarttable(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count, int loaded, DVM_Provider *reverse, const StyxUnibusLaunch *launch)
{
    StyxUnibusServer *server = channel && entries && count && count <= STYX_MOUNTED_SCALAR_LIMIT ?
        calloc(1, sizeof(*server)) : NULL;
    if (!server)
    {
        phyfree(channel);
        return NULL;
    }
    server->modules = calloc(count, sizeof(*server->modules));
    if (!server->modules)
    {
        phyfree(channel);
        free(server);
        return NULL;
    }
    server->count = count;
    server->loaded = loaded;
    if (reverse && !dvm_provider_retain(reverse))
    {
        free_modules(server);
        phyfree(channel);
        free(server);
        return NULL;
    }
    server->caller_root = reverse;
    if (launch)
    {
        server->completion = 1;
        if (launch->persistent)
        {
            server->launches = launch_queue(launch->tag);
        }
        else
        {
            server->inbox = styxunibusinbox(launch->tag);
            server->file = server->inbox ? styxbacking(server->inbox) : NULL;
        }
        server->original = launch->original ? styxbacking(launch->original) : NULL;
        server->profile = launch->profile ? styxbacking(launch->profile) : NULL;
    }
    if (launch && (!(server->launches || server->file) || !server->original || !server->profile))
    {
        phyfree(channel);
    }
    else if (!styxphyinit(&server->server, &operations, channel, 0555, 1, 2))
    {
        int launch_nodes = !launch ||
            (export_file(&server->server, server->original, 2, "implementation.dis") &&
             export_file(&server->server, server->profile, 3, ".interface") &&
             styxaddfile(&server->server, Qroot, LAUNCH_EXIT, ".exit", 0444, "unibus") &&
             styxaddfile(&server->server, Qroot, 1, STYX_UNIBUS_NODE, 0666, "unibus"));
        if (launch_nodes && export_modules(server, entries, loaded) && !pthread_create(&server->thread, NULL, serve, server))
        {
            return server;
        }
        styxend(&server->server);
    }
    free_launch_queue(server->launches);
    styxbackingfree(server->profile);
    styxbackingfree(server->original);
    styxbackingfree(server->file);
    dvm_provider_release(server->inbox);
    free_modules(server);
    dvm_provider_release(server->caller_root);
    free(server);
    return NULL;
}


#include "unibus_server_lifetime.inc"
