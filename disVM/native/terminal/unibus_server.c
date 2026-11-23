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


#include "unibus_server_channels.inc"


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


/* EN: Locate a named program by qid, returning its one-based ticket slot.
 * RU: Найти программу по qid и вернуть её номер в билете с единицы. */
static size_t program_slot(Path path, unsigned offset)
{
    if (path < FIRST_PROGRAM)
    {
        return 0;
    }
    Path index = path - FIRST_PROGRAM;
    return index % 2 == offset && index / 2 < current->program_count ? index / 2 + 1 : 0;
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
        StyxChannelExports *exports = current->modules[module->directory_leader].channel_exports;
        return styxscalarfunctionopen_export(&current->server, mode, &module->function, module->owner.request != NULL,
            module->function.program_root && current->loaded ? &current->function_workers : NULL, &module->busy, exports);
    }
    StyxChannelExports *exports = channels_for_node(qid->path);
    if (exports)
    {
        return open_channel_operation(exports, mode);
    }
    if (delta_node(qid->path))
    {
        return mode == OREAD ? nil : Eperm;
    }
    if (mounted_node(qid->path, 2))
    {
        return mode == OREAD && styxunibusawaitload(current) ? nil : Eperm;
    }
    if (qid->path == Qroot || qid->path == PROGRAM_DIRECTORY || qid->path == MOUNTED_ROOT || (current->completion && qid->path == LAUNCH_EXIT) || mounted_node(qid->path, 0))
    {
        return mode == OREAD ? nil : Eperm;
    }
    size_t request_slot = program_slot(qid->path, 1);
    if ((qid->path == 1 && (mode == OWRITE || (mode == ORDWR && current->completion))) ||
        (request_slot && mode == ORDWR))
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
    size_t image_slot = program_slot(qid->path, 0);
    StyxBacking *node = mode != OREAD ? NULL :
        image_slot ? current->programs[image_slot - 1] :
        qid->path == PROGRAM_CATALOG ? current->catalog :
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
    if (channels_for_node(qid.path))
    {
        return write_channel_operation(bytes, count, offset);
    }
    StyxTransfer result;
    size_t slot = program_slot(qid.path, 1);
    if ((qid.path != 1 && !slot) || offset < 0)
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
                fid->waiting, slot, &fid->ticket))
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
    if (channels_for_node(qid.path))
    {
        return read_channel_operation(bytes, count, offset);
    }
    if (delta_node(qid.path) && offset >= 0)
    {
        static const char capability[] = STYX_MANAGED_POSITION_GRAPH_MARKER;
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
    if ((qid.path == 1 || program_slot(qid.path, 1)) && current->completion && offset >= 0)
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
    if ((qid.path != 2 && qid.path != 3 && qid.path != PROGRAM_CATALOG && !program_slot(qid.path, 0)) || offset < 0)
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
static int export_file(Styxserver *server, ulong parent, StyxBacking *backing, ulong qid, const char *name)
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
    Styxfile *node = styxaddfile(server, parent, qid, (char *)name, 0444, "unibus");
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
            if (server->caller_root)
            {
                directory->channel_exports = styxchannel_exports_create();
                if (!directory->channel_exports || !styxaddfile(&server->server, parent,
                    FIRST_MODULE_CHANNELS + leader, STYX_CHANNEL_EXPORT_FILE, 0666, "unibus"))
                {
                    return 0;
                }
            }
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


#include "unibus_server_setup.inc"
#include "unibus_server_lifetime.inc"
