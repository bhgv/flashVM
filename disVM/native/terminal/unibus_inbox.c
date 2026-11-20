/* Copyright (c) 2026 bhgv
 * EN: Bounded one-shot S-record inbox shared by a Styx writer and local VFS readers.
 * RU: Ограниченный одноразовый приём S-записи из Styx для локальных читателей VFS.
 */
#include "unibus_inbox.h"
#include "cancel.h"
#include "flashvm/srecord.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct Inbox
{
    pthread_mutex_t lock;    /* Protects the one-time record publication. */
    unsigned long tag;       /* Owner-selected tag. */
    size_t size;             /* Zero while empty. */
    unsigned char bytes[64]; /* One immutable complete wire record after admission. */
} Inbox;

typedef struct Cursor
{
    Inbox *inbox;     /* Provider holds the lifetime of the inbox. */
    size_t position;  /* Private sequential read position. */
} Cursor;


/* EN/RU: Describe the single bounded byte stream.
 * Описать единственный ограниченный поток байтов. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    memset(out, 0, sizeof(*out));
    strcpy(out->name, ".unibus");
    out->mode = 0666;
    return FVM_OK;
}


/* EN/RU: Read sequentially or report pending without consuming a partial publication.
 * Читать последовательно либо сообщить об ожидании без потребления частичной записи. */
static void read_chunk(void *state, void *bytes, size_t size, DVM_IoResult *out)
{
    Cursor *cursor = state;
    Inbox *inbox = cursor->inbox;
    *out = (DVM_IoResult){.status = FVM_OK, .flags = DVM_IO_PREFIX};
    if (!size)
    {
        return;
    }
    pthread_mutex_lock(&inbox->lock);
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        out->status = FVM_ERROR;
        out->stop = DVM_IO_CANCELLED;
    }
    else if (!inbox->size)
    {
        out->status = FVM_ERROR;
        out->stop = DVM_IO_AGAIN;
    }
    else
    {
        size_t remaining = inbox->size - cursor->position;
        out->count = size < remaining ? size : remaining;
        if (out->count)
        {
            memcpy(bytes, inbox->bytes + cursor->position, out->count);
            cursor->position += out->count;
            if (cursor->position == inbox->size)
            {
                out->flags |= DVM_IO_RECORD;
            }
        }
        else
        {
            out->stop = DVM_IO_EOF;
        }
    }
    pthread_mutex_unlock(&inbox->lock);
}


/* EN/RU: Validate and publish exactly one complete record under the inbox lock.
 * Проверить и опубликовать ровно одну полную запись под блокировкой. */
static void write_chunk(void *state, const void *bytes, size_t size, DVM_IoResult *out)
{
    Inbox *inbox = ((Cursor *)state)->inbox;
    FVM_SRecord record;
    size_t consumed;
    *out = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
    if (!size || size > sizeof(inbox->bytes) ||
        fvm_srecord_read(bytes, size, sizeof(inbox->bytes), &record, &consumed) != FVM_S_OK ||
        consumed != size || record.tag != inbox->tag)
    {
        return;
    }
    pthread_mutex_lock(&inbox->lock);
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        out->stop = DVM_IO_CANCELLED;
    }
    else if (!inbox->size)
    {
        memcpy(inbox->bytes, bytes, size);
        inbox->size = size;
        out->count = size;
        out->status = FVM_OK;
        out->stop = DVM_IO_NONE;
    }
    pthread_mutex_unlock(&inbox->lock);
}

static const DVM_VfsOps reader = {.read_chunk = read_chunk, .close = free}; /* Read-only cursor. */
static const DVM_VfsOps writer = {.write_chunk = write_chunk, .close = free}; /* Write-only cursor. */


/* EN/RU: Allocate one cursor with exactly the requested direction.
 * Выделить позицию только для запрошенного направления. */
static FVM_Result open_file(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    const DVM_VfsOps *ops = !strcmp(mode, "r") ? &reader : !strcmp(mode, "wo") ? &writer : NULL;
    if (!ops || dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    Cursor *cursor = calloc(1, sizeof(*cursor));
    if (!cursor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    cursor->inbox = state;
    out->ops = ops;
    out->state = cursor;
    out->native_flags = DVM_VFS_CANCEL_DATA;
    return FVM_OK;
}


/* EN/RU: Destroy the inbox after all retained handles retire.
 * Удалить приёмник после закрытия удерживаемых файлов. */
static void destroy(void *state)
{
    Inbox *inbox = state;
    pthread_mutex_destroy(&inbox->lock);
    free(inbox);
}

static const DVM_ProviderOps factory = {.info = info, .open = open_file, .destroy = destroy}; /* Exact root file. */


/* EN/RU: See unibus_inbox.h; the returned provider owns the inbox.
 * См. unibus_inbox.h; возвращённый провайдер владеет приёмником. */
DVM_Provider *styxunibusinbox(unsigned long tag)
{
    if (!tag || tag > FVM_SRECORD_U32)
    {
        return NULL;
    }
    Inbox *inbox = calloc(1, sizeof(*inbox));
    if (!inbox)
    {
        return NULL;
    }
    if (pthread_mutex_init(&inbox->lock, NULL))
    {
        free(inbox);
        return NULL;
    }
    inbox->tag = tag;
    DVM_Provider *provider = dvm_provider_create_native(&factory, inbox, NULL, 1,
        DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_CAPTURE | DVM_PROVIDER_CANCEL_LIST);
    if (!provider)
    {
        destroy(inbox);
    }
    return provider;
}
