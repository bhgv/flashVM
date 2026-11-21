/* Copyright (c) 2026 bhgv
 * EN: Bounded one-shot S-record inbox shared by a Styx writer and local VFS readers.
 * RU: Ограниченный одноразовый приём S-записи из Styx для локальных читателей VFS.
 */
#include "unibus_inbox.h"
#include "cancel.h"
#include "load_profile.h"
#include "provider_internal.h"
#include "flashvm/srecord.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct Inbox
{
    pthread_mutex_t lock;    /* Protects the one-time record publication. */
    unsigned long tag;       /* Owner-selected tag. */
    int sealed; /* No further writes after carrier shutdown. */
    int reusable; /* Consume complete records for a serialized live route. */
    size_t size;             /* Zero while empty. */
    DVM_Provider *context;   /* Owned context of a queued local request. */
    size_t capacity; /* Exact accepted wire record limit. */
    unsigned char bytes[]; /* One immutable complete wire record after admission. */
} Inbox;

typedef struct Cursor
{
    Inbox *inbox;     /* Provider holds the lifetime of the inbox. */
    size_t position;  /* Private sequential read position. */
    DVM_Provider *context; /* Owned context of the last complete read. */
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
    dvm_provider_release(cursor->context);
    cursor->context = NULL;
    pthread_mutex_lock(&inbox->lock);
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        out->status = FVM_ERROR;
        out->stop = DVM_IO_CANCELLED;
    }
    else if (!inbox->size)
    {
        if (inbox->sealed)
        {
            out->stop = DVM_IO_EOF;
        }
        else
        {
            out->status = FVM_ERROR;
            out->stop = DVM_IO_AGAIN;
        }
    }
    else if (inbox->reusable)
    {
        if (size < inbox->size)
        {
            out->status = FVM_ERROR;
            out->stop = DVM_IO_DENIED;
        }
        else
        {
            memcpy(bytes, inbox->bytes, inbox->size);
            out->count = inbox->size;
            out->flags |= DVM_IO_RECORD;
            inbox->size = 0;
            cursor->context = inbox->context;
            inbox->context = NULL;
        }
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


/* EN: Transfer context on success together with the complete validated record.
 * RU: Передать контекст при успешной публикации полной проверенной записи. */
static void publish(Inbox *inbox, const void *bytes, size_t size, DVM_Provider *context, DVM_IoResult *out)
{
    FVM_SRecord record;
    size_t consumed;
    *out = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
    if (!size || size > inbox->capacity ||
        fvm_srecord_read(bytes, size, inbox->capacity, &record, &consumed) != FVM_S_OK ||
        consumed != size || (inbox->tag && record.tag != inbox->tag))
    {
        return;
    }
    pthread_mutex_lock(&inbox->lock);
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        out->stop = DVM_IO_CANCELLED;
    }
    else if (!inbox->sealed && !inbox->size)
    {
        memcpy(inbox->bytes, bytes, size);
        inbox->size = size;
        inbox->context = context;
        out->count = size;
        out->status = FVM_OK;
        out->stop = DVM_IO_NONE;
    }
    pthread_mutex_unlock(&inbox->lock);
}




/* EN: Publish one local record without an attached caller context.
 * RU: Опубликовать одну локальную запись без контекста клиента. */
static void write_chunk(void *state, const void *bytes, size_t size, DVM_IoResult *out)
{
    publish(((Cursor *)state)->inbox, bytes, size, NULL, out);
}


/* EN: Transfer the context held by the last read; subsequent queries return NULL.
 * RU: Передать контекст последнего чтения; следующие запросы вернут NULL. */
static DVM_Provider *take_context(void *state)
{
    Cursor *cursor = state;
    DVM_Provider *context = cursor->context;
    cursor->context = NULL;
    return context;
}


/* EN: Release any unread context before consuming an exclusive cursor.
 * RU: Освободить невзятый контекст перед освобождением курсора. */
static void close_reader(void *state)
{
    dvm_provider_release(((Cursor *)state)->context);
    free(state);
}

static const DVM_VfsOps reader = {.read_chunk = read_chunk, .close = close_reader,
    .take_context = take_context}; /* Read-only cursor. */
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
    dvm_provider_release(inbox->context);
    pthread_mutex_destroy(&inbox->lock);
    free(inbox);
}

static const DVM_ProviderOps factory = {.info = info, .open = open_file, .destroy = destroy}; /* Exact root file. */


/* EN/RU: See unibus_inbox.h. / См. unibus_inbox.h. */
int styxunibusmailboxcontext(DVM_Provider *provider, const void *bytes, size_t size, DVM_Provider *context)
{
    if (!provider || provider->ops != &factory || !((Inbox *)provider->state)->reusable ||
        !context || !dvm_provider_retain(context))
    {
        return 0;
    }
    DVM_IoResult sent;
    publish(provider->state, bytes, size, context, &sent);
    if (sent.status == FVM_OK)
    {
        return 1;
    }
    dvm_provider_release(context);
    return 0;
}


/* EN/RU: Seal a reusable inbox without discarding its last confirmed record.
 * Закрыть переиспользуемый приёмник, сохраняя последнюю подтверждённую запись. */
void styxunibusmailboxseal(DVM_Provider *provider)
{
    if (provider && provider->ops == &factory)
    {
        Inbox *inbox = provider->state;
        pthread_mutex_lock(&inbox->lock);
        inbox->sealed = inbox->reusable;
        pthread_mutex_unlock(&inbox->lock);
    }
}


/* EN/RU: See unibus_inbox.h; the returned provider owns the inbox.
 * См. unibus_inbox.h; возвращённый провайдер владеет приёмником. */
static DVM_Provider *create_inbox(unsigned long tag, int reusable, size_t capacity)
{
    if (tag > FVM_SRECORD_U32)
    {
        return NULL;
    }
    Inbox *inbox = calloc(1, sizeof(*inbox) + capacity);
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
    inbox->capacity = capacity;
    inbox->reusable = reusable;
    DVM_Provider *provider = dvm_provider_create_native(&factory, inbox, NULL, 1,
        DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_CAPTURE | DVM_PROVIDER_CANCEL_LIST);
    if (!provider)
    {
        destroy(inbox);
    }
    return provider;
}


/* EN/RU: Preserve the original one-shot inbox.
 * Сохранить исходный одноразовый приёмник. */
DVM_Provider *styxunibusinbox(unsigned long tag)
{
    return tag ? create_inbox(tag, 0, 64) : NULL;
}


/* EN/RU: Open a reusable one-record slot for a serialized session.
 * Создать переиспользуемый приёмник для последовательной сессии. */
DVM_Provider *styxunibusmailbox(unsigned long tag)
{
    return tag ? create_inbox(tag, 1, 64) : NULL;
}



/* EN/RU: See unibus_inbox.h. / См. unibus_inbox.h. */
DVM_Provider *styxunibusloadmailbox(void)
{
    return create_inbox(0, 1, 64);
}



/* EN/RU: See unibus_inbox.h. / См. unibus_inbox.h. */
DVM_Provider *styxunibusprofilemailbox(void)
{
    return create_inbox(0, 1, STYX_SHAPED_OFFER_MAX);
}
