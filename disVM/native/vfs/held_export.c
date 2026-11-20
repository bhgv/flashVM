/* Copyright (c) 2026 bhgv
 * EN: One-use export preserves the opened object and its sequential cursor.
 * RU: Одноразовый экспорт сохраняет открытый объект и позицию чтения и записи.
 */
#include "held_export.h"
#include "cancel.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Retained provider owns the file until one permitted open takes it.
 * Фабрика владеет файлом до первого разрешённого открытия. */
typedef struct HeldExport
{
    pthread_mutex_t lock;   /* Serializes take and retirement. */
    pthread_cond_t ready;   /* Wakes the worker when the factory is retired. */
    int retired;            /* Final provider reference has been released. */
    unsigned access;        /* Immutable rights ceiling specified by the native owner. */
    FVM_FileInfo info;      /* Immutable metadata snapshot with narrowed mode. */
    DVM_VfsHandle source;   /* Exclusive ownership until moved out or retired. */
} HeldExport;


/* EN/RU: One granted open holds the original cursor under its caller's FIFO.
 * Выданное открытие удерживает исходную позицию под FIFO вызывающего. */
typedef struct HeldOpen
{
    DVM_VfsOps ops;          /* Per-open operations selected by the requested mode. */
    DVM_VfsHandle source;    /* Original native file with its existing cursor. */
    FVM_FileInfo info;       /* Copied metadata with the granted open's mode. */
} HeldOpen;


/* EN/RU: Copy immutable open metadata into out without I/O.
 * Скопировать неизменяемые metadata открытого файла в out без I/O. */
static FVM_Result held_info(void *value, FVM_FileInfo *out)
{
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    *out = ((HeldOpen *)value)->info;
    return FVM_OK;
}


/* EN/RU: Read into bytes through the owned open state, preserving the source acknowledgement.
 * Прочитать в bytes через открытый объект, сохранив исходное подтверждение. */
static void held_read(void *value, void *bytes, size_t size, DVM_IoResult *out)
{
    *out = dvm_vfs_read_chunk(&((HeldOpen *)value)->source, bytes, size);
    out->observed_mode &= ((HeldOpen *)value)->info.mode;
}


/* EN/RU: Write size bytes through the opened object; preserve partial or uncertain effects.
 * Записать size байтов через открытый объект с сохранением частичных или неопределённых effects. */
static void held_write(void *value, const void *bytes, size_t size, DVM_IoResult *out)
{
    *out = dvm_vfs_write_chunk(&((HeldOpen *)value)->source, bytes, size);
    out->observed_mode &= ((HeldOpen *)value)->info.mode;
}


/* EN/RU: Close the owned original file and free open state after all borrowers retire.
 * Закрыть исходный файл и освободить состояние после завершения заимствований. */
static void held_close(void *value)
{
    HeldOpen *opened = value;
    dvm_vfs_close(&opened->source);
    free(opened);
}


/* EN/RU: Report whether the borrowed open supports cancellation during close.
 * Проверить поддержку отмены закрытия заимствованного открытого файла. */
static int held_close_ready(void *value)
{
    return dvm_vfs_close_cancellable(&((HeldOpen *)value)->source);
}


/* EN/RU: Get the display permission mask for the given read/write rights.
 * Получить отображаемую маску разрешений для чтения и записи. */
static unsigned long held_mode(unsigned access)
{
    return (access & FVM_ACCESS_READ ? 0444UL : 0) |
           (access & FVM_ACCESS_WRITE ? 0222UL : 0);
}


/* EN/RU: Copy the retained factory's dot metadata; other names are absent.
 * Скопировать metadata корня удерживаемой фабрики; других имён нет. */
static FVM_Result export_info(void *value, const char *relative, FVM_FileInfo *out)
{
    HeldExport *grant = value;
    if (strcmp(relative, "."))
    {
        return FVM_NOT_FOUND;
    }
    *out = grant->info;
    return FVM_OK;
}


/* EN/RU: Check the rights ceiling of the retained dot provider without consuming it.
 * Проверить предел прав удерживаемой фабрики без получения файла. */
static FVM_Result export_access(void *value, const char *relative, unsigned access)
{
    HeldExport *grant = value;
    if (strcmp(relative, "."))
    {
        return FVM_NOT_FOUND;
    }
    return (access & ~grant->access) ? FVM_ERROR : FVM_OK;
}


/* EN/RU: Decode only non-truncating read/write mode names accepted by the provider.
 * Распознать только режимы чтения и записи без усечения. */
static unsigned open_access(const char *mode)
{
    if (!strcmp(mode, "r"))
    {
        return FVM_ACCESS_READ;
    }
    if (!strcmp(mode, "wo"))
    {
        return FVM_ACCESS_WRITE;
    }
    if (!strcmp(mode, "rw"))
    {
        return FVM_ACCESS_READ | FVM_ACCESS_WRITE;
    }
    return 0;
}


/* EN/RU: Accept one permitted dot open. All fallible setup precedes taking the cursor.
 * Принять одно разрешённое открытие корня. Все операции, которые могут завершиться
 * ошибкой, предшествуют получению открытого файла. */
static FVM_Result export_open(void *value, const char *relative, const char *mode, DVM_VfsHandle *out)
{
    HeldExport *grant = value;
    unsigned access = open_access(mode);
    if (!access)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Result status = export_access(grant, relative, access);
    if (status != FVM_OK)
    {
        return status;
    }
    HeldOpen *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        return FVM_OUT_OF_MEMORY;
    }
    pthread_mutex_lock(&grant->lock);
    int acquired = grant->source.ops && dvm_vfs_move(&opened->source, &grant->source);
    pthread_mutex_unlock(&grant->lock);
    if (!acquired)
    {
        free(opened);
        return FVM_NOT_FOUND;
    }
    opened->info = grant->info;
    opened->info.mode &= held_mode(access);
    opened->ops.info = held_info;
    opened->ops.close = held_close;
    opened->ops.read_chunk = access & FVM_ACCESS_READ ? held_read : NULL;
    opened->ops.write_chunk = access & FVM_ACCESS_WRITE ? held_write : NULL;
    out->ops = &opened->ops;
    out->state = opened;
    out->close_ready = held_close_ready;
    out->native_flags = (opened->source.native_flags & (DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_CLOSE)) |
                        DVM_VFS_CANCEL_INFO;
    return FVM_OK;
}


/* EN/RU: Free worker-owned state after closing any unclaimed file outside locks.
 * Освободить состояние worker после закрытия невостребованного файла вне блокировок. */
static void *retire_worker(void *value)
{
    HeldExport *grant = value;
    pthread_mutex_lock(&grant->lock);
    while (!grant->retired)
    {
        pthread_cond_wait(&grant->ready, &grant->lock);
    }
    pthread_mutex_unlock(&grant->lock);
    dvm_vfs_close(&grant->source);
    pthread_cond_destroy(&grant->ready);
    pthread_mutex_destroy(&grant->lock);
    free(grant);
    return NULL;
}


/* EN/RU: Hand final cleanup of the provider state to its worker without a native close wait.
 * Передать окончательную очистку состояния worker без ожидания закрытия файла. */
static void export_destroy(void *value)
{
    HeldExport *grant = value;
    pthread_mutex_lock(&grant->lock);
    grant->retired = 1;
    pthread_cond_signal(&grant->ready);
    pthread_mutex_unlock(&grant->lock);
}


static const DVM_ProviderOps factory = { /* Only the fixed dot has an identity. */
    .info = export_info,
    .open = export_open,
    .destroy = export_destroy,
    .access = export_access
};


/* EN/RU: Start a detached cleanup worker for grant; on failure the caller still owns state.
 * Запустить отделённый worker очистки grant; при отказе состояние остаётся у вызывающего. */
static int start_worker(HeldExport *grant)
{
    if (pthread_mutex_init(&grant->lock, NULL))
    {
        return 0;
    }
    if (pthread_cond_init(&grant->ready, NULL))
    {
        pthread_mutex_destroy(&grant->lock);
        return 0;
    }
    pthread_attr_t attr;
    pthread_t worker;
    int initialized = !pthread_attr_init(&attr);
    int started = initialized && !pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) &&
                  !pthread_create(&worker, &attr, retire_worker, grant);
    if (initialized)
    {
        pthread_attr_destroy(&attr);
    }
    if (!started)
    {
        pthread_cond_destroy(&grant->ready);
        pthread_mutex_destroy(&grant->lock);
    }
    return started;
}


/* EN/RU: See held_export.h. The source stays exclusively borrowed during setup.
 * См. held_export.h. Во время подготовки source остаётся в исключительном заимствовании. */
DVM_Provider *styxheldexport(DVM_VfsHandle *source, unsigned access)
{
    if (!source || !source->ops || !access || (access & ~(FVM_ACCESS_READ | FVM_ACCESS_WRITE)) ||
        ((access & FVM_ACCESS_READ) && !source->ops->read && !source->ops->read_chunk) ||
        ((access & FVM_ACCESS_WRITE) && !source->ops->write && !source->ops->write_chunk))
    {
        return NULL;
    }
    HeldExport *grant = calloc(1, sizeof(*grant));
    if (!grant)
    {
        return NULL;
    }
    if (dvm_vfs_info(source, &grant->info) != FVM_OK || grant->info.directory || !start_worker(grant))
    {
        free(grant);
        return NULL;
    }
    grant->access = access;
    grant->info.mode &= held_mode(access);
    DVM_Provider *provider = dvm_provider_create_native(&factory, grant, NULL, 1, 0);
    if (!provider)
    {
        export_destroy(grant);
        return NULL;
    }
    if (!dvm_vfs_move(&grant->source, source))
    {
        dvm_provider_release(provider);
        return NULL;
    }
    return provider;
}
