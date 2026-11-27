/* Copyright (c) 2026 bhgv
 * EN: Defer cancellable data operations while the reverse Styx dispatcher handles traffic.
 * RU: Откладывать отменяемый обмен данными, сохраняя работу диспетчера Styx.
 */
#include "caller_styx_state.h"
#include "cancel.h"
#include <stdlib.h>
#include <string.h>

#define CALLER_IO_LIMIT 16U /* Maximum outstanding reads and writes, including cancellation. */


/* EN: Keep wire ownership on the dispatcher and give the native worker one cursor and buffer.
 * RU: Оставить протокол у диспетчера и выдать нативному потоку один курсор и буфер. */
struct CallerIo
{
    CallerIo *next;         /* Dispatcher-owned list linkage. */
    CallerOpen *opened;     /* Exclusive cursor through worker join. */
    StyxRead *request;      /* Owner-thread request shared by the native read and write APIs. */
    DVM_Cancel *cancel;      /* Owned stop source available to both threads. */
    StyxPhyWake *wake;      /* Server-owned wake pin, valid through all joins. */
    pthread_t thread;       /* Native provider worker joined by the dispatcher. */
    size_t capacity;        /* Immutable byte limit of the provider transfer. */
    vlong offset;           /* Absolute requested offset. */
    StyxTransfer result;    /* Written by the worker, published with done. */
    int started;            /* Dispatcher tracks whether thread requires join. */
    int done;               /* Atomic completion, published by the worker. */
    int writing;            /* Use bytes as write input when nonzero. */
    unsigned char bytes[];  /* Owned write input or read output. */
};


/* EN/RU: See caller_styx_state.h. / См. caller_styx_state.h. */
void styxcaller_close_open(void *state)
{
    CallerOpen *opened = state;
    if (opened->active)
    {
        opened->retired = 1;
        return;
    }
    styxfileclose(opened->file);
    free(opened);
}


/* EN: Drop the wire reference from state and cancel any unfinished provider transfer.
 * RU: Освободить запрос из state и отменить незавершённую передачу провайдера. */
static void release_io(void *state)
{
    CallerIo *work = state;
    if (work->writing)
    {
        styxwritefree(work->request);
    }
    else
    {
        styxreadfree(work->request);
    }
    work->request = NULL;
    if (!__atomic_load_n(&work->done, __ATOMIC_ACQUIRE))
    {
        dvm_cancel_request(work->cancel);
    }
}


/* EN: Transfer on state's exclusive cursor under its cancel scope, then wake the dispatcher.
 * RU: Выполнить передачу на исключительном курсоре state с отменой и разбудить диспетчер. */
static void *run_io(void *state)
{
    CallerIo *work = state;
    DVM_CancelScope scope = {0};
    if (dvm_cancel_enter(&scope, work->cancel))
    {
        if (!dvm_cancel_requested(work->cancel))
        {
            if (work->writing)
            {
                styxfilewrite(work->opened->file, work->offset, work->bytes, work->capacity, &work->result);
            }
            else
            {
                styxfileread(work->opened->file, work->offset, work->bytes, work->capacity, &work->result);
            }
        }
        dvm_cancel_leave(&scope);
    }
    __atomic_store_n(&work->done, 1, __ATOMIC_RELEASE);
    styxphywake(work->wake);
    return NULL;
}


/* EN/RU: See caller_styx_state.h. / См. caller_styx_state.h. */
char *styxcaller_defer_io(StyxCallerServer *server, CallerOpen *opened, const void *input,
    ulong *count, vlong offset)
{
    if (server->io_count >= CALLER_IO_LIMIT || *count > SIZE_MAX - sizeof(CallerIo))
    {
        return "too many pending data requests";
    }
    CallerIo *work = calloc(1, sizeof(*work) + *count);
    if (!work)
    {
        return Enomem;
    }
    work->writing = input != NULL;
    work->cancel = dvm_cancel_create();
    if (work->cancel)
    {
        work->request = work->writing ? styxdeferwrite(&server->server, work, release_io) :
            styxdeferread(&server->server, work, release_io);
    }
    if (!work->request)
    {
        dvm_cancel_release(work->cancel);
        free(work);
        return Enomem;
    }
    if (input)
    {
        memcpy(work->bytes, input, *count);
    }
    work->capacity = *count;
    work->offset = offset;
    work->opened = opened;
    work->wake = server->wake;
    work->next = server->pending;
    server->pending = work;
    ++server->io_count;
    opened->active = work;
    if (pthread_create(&work->thread, NULL, run_io, work))
    {
        __atomic_store_n(&work->done, 1, __ATOMIC_RELEASE);
        return Enomem;
    }
    work->started = 1;
    *count = 0;
    return nil;
}


/* EN: Complete work's wire request with its provider outcome. Return 1 if retired.
 * RU: Завершить запрос work результатом провайдера. Вернуть 1 после снятия запроса. */
static int send_io(CallerIo *work)
{
    const StyxTransfer *result = &work->result;
    int acknowledged = result->confirmed && !result->uncertain && (result->success || result->count) &&
        result->count <= work->capacity;
    ulong count = acknowledged ? result->count : 0;
    char *error = acknowledged ? nil : result->again ? "resource temporarily unavailable" : Eperm;
    if (work->writing)
    {
        return styxwritereply(work->request, count, error);
    }
    return styxreadreply(work->request, work->bytes, count, error);
}


/* EN/RU: See caller_styx_state.h. / См. caller_styx_state.h. */
void styxcaller_finish_io(StyxCallerServer *server, int closing)
{
    CallerIo **link = &server->pending;
    while (*link)
    {
        CallerIo *work = *link;
        if (!closing && (!__atomic_load_n(&work->done, __ATOMIC_ACQUIRE) ||
            (work->request && !styxphyspace(&server->server))))
        {
            link = &work->next;
            continue;
        }
        if (work->request && !send_io(work))
        {
            link = &work->next;
            continue;
        }
        if (work->started)
        {
            pthread_join(work->thread, NULL);
        }
        work->opened->active = NULL;
        if (work->opened->retired)
        {
            styxcaller_close_open(work->opened);
        }
        *link = work->next;
        --server->io_count;
        dvm_cancel_release(work->cancel);
        free(work);
    }
}
