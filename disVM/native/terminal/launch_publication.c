/* Copyright (c) 2026 bhgv
 * EN: VFS submit and completion nodes for a fixed local Dis command.
 * RU: Узлы запроса и результата VFS для фиксированной локальной команды Dis.
 */
#include "launch_publication.h"
#include "flashvm/srecord.h"
#include "launch/ticket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct LaunchFile
{
    StyxLaunchPublication *publication; /* Borrowed through factory retirement. */
    int completion;                     /* Selects read-only rather than write-only operation. */
} LaunchFile;

typedef struct LaunchOpen
{
    LaunchFile *file; /* Kept alive by the provider's handle wrapper. */
    char result[16]; /* Immutable decimal exit record once obtained. */
    size_t length;   /* Length of the cached record. */
    size_t position; /* Sequential read offset for this open. */
} LaunchOpen;


/* EN/RU: Describe one immutable endpoint with its exact access mode.
 * Описать неизменяемый узел с точным режимом доступа. */
static FVM_Result info(void *state, const char *name, FVM_FileInfo *out)
{
    LaunchFile *file = state;
    if (strcmp(name, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    strcpy(out->name, file->completion ? "result" : "submit");
    out->mode = file->completion ? 0444 : 0222;
    out->identity.low = file->completion ? 2 : 1;
    return FVM_OK;
}


/* EN/RU: Describe the retained open node.
 * Описать удерживаемый открытый узел. */
static FVM_Result openinfo(void *state, FVM_FileInfo *out)
{
    return info(((LaunchOpen *)state)->file, ".", out);
}


/* EN/RU: Acknowledge one complete authorized write or a definite refusal.
 * Подтвердить полную разрешённую запись либо определённый отказ. */
static void write_submit(void *state, const void *bytes, size_t size, DVM_IoResult *out)
{
    StyxLaunchPublication *publication = ((LaunchOpen *)state)->file->publication;
    pthread_mutex_lock(&publication->lock);
    int accepted = styxterminallaunchreceive(&publication->process.receiver, 0, bytes, size);
    pthread_mutex_unlock(&publication->lock);
    *out = (DVM_IoResult){.count = accepted ? size : 0, .status = accepted ? FVM_OK : FVM_ERROR,
        .stop = accepted ? DVM_IO_NONE : DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
}


/* EN/RU: Wait for a submitted command once per open, then stream the cached exit record.
 * Дождаться поданной команды для этого открытия, затем прочитать сохранённый статус. */
static FVM_Result read_result(void *state, void *bytes, size_t size, size_t *received)
{
    LaunchOpen *opened = state;
    if (!opened->length && size)
    {
        StyxLaunchPublication *publication = opened->file->publication;
        int status;
        pthread_mutex_lock(&publication->lock);
        int ready = styxlaunchprocessstatus(&publication->process, 1, &status);
        pthread_mutex_unlock(&publication->lock);
        if (ready != 1)
        {
            return FVM_ERROR;
        }
        opened->length = (size_t)snprintf(opened->result, sizeof(opened->result), "%d\n", status);
    }
    size_t available = opened->length - opened->position;
    *received = size < available ? size : available;
    if (*received)
    {
        memcpy(bytes, opened->result + opened->position, *received);
        opened->position += *received;
    }
    return FVM_OK;
}

static const DVM_VfsOps submit_ops = {.info = openinfo, .write_chunk = write_submit, .close = free}; /* Write-only fid. */
static const DVM_VfsOps result_ops = {.info = openinfo, .read = read_result, .close = free}; /* Read-only fid. */


/* EN/RU: Open only the declared access mode and allocate a fresh per-fid cursor.
 * Открыть только заявленный режим и выделить отдельную позицию fid. */
static FVM_Result openfile(void *state, const char *name, const char *mode, DVM_VfsHandle *out)
{
    LaunchFile *file = state;
    if (strcmp(name, "."))
    {
        return FVM_NOT_FOUND;
    }
    if (strcmp(mode, file->completion ? "r" : "wo"))
    {
        return FVM_ERROR;
    }
    LaunchOpen *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        return FVM_OUT_OF_MEMORY;
    }
    opened->file = file;
    out->ops = file->completion ? &result_ops : &submit_ops;
    out->state = opened;
    return FVM_OK;
}


/* EN/RU: Check the exact endpoint ceiling without taking an open.
 * Проверить точные права узла без открытия. */
static FVM_Result access_file(void *state, const char *name, unsigned access)
{
    LaunchFile *file = state;
    unsigned allowed = file->completion ? FVM_ACCESS_READ : FVM_ACCESS_WRITE;
    return strcmp(name, ".") ? FVM_NOT_FOUND : (access & ~allowed) ? FVM_ERROR : FVM_OK;
}

static const DVM_ProviderOps factory = {.info = info, .open = openfile, .access = access_file, .destroy = free}; /* Single-file factory. */


/* EN/RU: See launch_publication.h; the owner selects the command before publication.
 * См. launch_publication.h; владелец задаёт команду до публикации. */
int styxlaunchpublicationinit(StyxLaunchPublication *publication, unsigned long tag,
    const char *directory, char *const *argv)
{
    return publication && styxlaunchprocessinit(&publication->process, tag, directory, argv) &&
        pthread_mutex_init(&publication->lock, NULL) == 0;
}


/* EN/RU: See launch_publication.h; the returned factory owns its endpoint metadata.
 * См. launch_publication.h; фабрика владеет метаданными узла. */
DVM_Provider *styxlaunchpublicationfile(StyxLaunchPublication *publication, int completion)
{
    if (!publication || (completion != 0 && completion != 1))
    {
        return NULL;
    }
    LaunchFile *file = malloc(sizeof(*file));
    if (!file)
    {
        return NULL;
    }
    *file = (LaunchFile){publication, completion};
    DVM_Provider *provider = dvm_provider_create_native(&factory, file, NULL, 1, 0);
    if (!provider)
    {
        free(file);
    }
    return provider;
}


/* EN/RU: See launch_publication.h; classify only an entire initial frame.
 * См. launch_publication.h; классифицировать только целый первый кадр. */
int styxlaunchpublicationdispatch(StyxLaunchPublication *publication, uint64_t offset,
    const void *bytes, size_t size)
{
    FVM_SRecord record;
    size_t consumed;
    if (!publication || offset || !bytes || !size ||
        fvm_srecord_read(bytes, size, DVM_LAUNCH_ARGUMENT_LIMIT + FVM_SRECORD_INTEGER_MAX,
            &record, &consumed) != FVM_S_OK || consumed != size ||
        record.tag != publication->process.receiver.tag)
    {
        return 0;
    }
    pthread_mutex_lock(&publication->lock);
    int accepted = styxterminallaunchreceive(&publication->process.receiver, offset, bytes, size);
    pthread_mutex_unlock(&publication->lock);
    return accepted ? 1 : -1;
}


/* EN/RU: Encode one owner-selected reply tag/ordinal with the decimal exit status.
 * Закодировать десятичный статус с заданными владельцем тегом и номером ответа. */
int styxlaunchpublicationresultrecord(StyxLaunchPublication *publication, int wait,
    const FVM_SOrdinal *ordinal, unsigned long tag, void *bytes, size_t capacity, size_t *used)
{
    if (!publication || !ordinal || !tag || tag > FVM_SRECORD_U32 || !bytes || !used)
    {
        return -1;
    }
    *used = 0;
    int status;
    pthread_mutex_lock(&publication->lock);
    int ready = styxlaunchprocessstatus(&publication->process, wait, &status);
    pthread_mutex_unlock(&publication->lock);
    if (ready != 1)
    {
        return ready;
    }
    char text[16];
    int length = snprintf(text, sizeof(text), "%d\n", status);
    if (length <= 0 || (size_t)length >= sizeof(text))
    {
        return -1;
    }
    return fvm_sordered_write(ordinal, tag, text, (size_t)length, bytes, capacity, used) == FVM_S_OK ? 1 : -1;
}


/* EN/RU: See launch_publication.h; return the same status contract as processstatus.
 * См. launch_publication.h; результат по контракту processstatus. */
int styxlaunchpublicationfinish(StyxLaunchPublication *publication, int *status)
{
    if (!publication || !status)
    {
        return -1;
    }
    int result = styxlaunchprocessstatus(&publication->process, 1, status);
    pthread_mutex_destroy(&publication->lock);
    return result;
}
