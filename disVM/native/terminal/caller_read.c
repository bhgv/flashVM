/* Copyright (c) 2026 bhgv
 * EN: Defer cancellable provider reads while the reverse Styx dispatcher processes traffic.
 * RU: Откладывать отменяемые чтения провайдера, сохраняя работу встречного диспетчера Styx.
 */
#include "caller_styx_state.h"
#include "cancel.h"
#include <stdlib.h>

#define CALLER_READ_LIMIT 16U /* Maximum outstanding byte reads including cancelling jobs. */


/* EN: Keep wire ownership on the dispatcher and give the native worker one cursor and buffer.
 * RU: Оставить протокол у диспетчера и выдать нативному потоку один курсор и буфер. */
struct CallerRead
{
    CallerRead *next;        /* Dispatcher-owned list linkage. */
    CallerOpen *opened;      /* Exclusive cursor through worker join. */
    StyxRead *request;       /* Owned deferred request on the dispatcher, cleared on retirement. */
    DVM_Cancel *cancel;      /* Owned stop source available to both threads. */
    StyxPhyWake *wake;       /* Server-owned wake pin, valid through all joins. */
    pthread_t thread;       /* Native provider worker joined by the dispatcher. */
    size_t capacity;        /* Immutable byte limit of the provider read. */
    vlong offset;           /* Absolute requested offset. */
    StyxTransfer result;    /* Written by the worker, published with done. */
    int started;            /* Dispatcher tracks whether thread requires join. */
    int done;               /* Atomic completion, published by the worker. */
    unsigned char bytes[];  /* Owned output, read only after completion. */
};


/* EN/RU: See caller_styx_state.h. / См. caller_styx_state.h. */
void styxcaller_close_open(void *state)
{
    CallerOpen *opened = state;
    if (opened->read)
    {
        opened->retired = 1;
        return;
    }
    styxfileclose(opened->file);
    free(opened);
}


/* EN: Relinquish the deferred wire request and wake an unfinished provider on retirement.
 * RU: Освободить отложенный запрос и разбудить незавершённое чтение при снятии. */
static void release_read(void *state)
{
    CallerRead *read = state;
    styxreadfree(read->request);
    read->request = NULL;
    if (!__atomic_load_n(&read->done, __ATOMIC_ACQUIRE))
    {
        dvm_cancel_request(read->cancel);
    }
}


/* EN: Read the exclusive cursor under read's native cancel scope and wake the dispatcher.
 * RU: Прочитать исключительный курсор под областью отмены read и разбудить диспетчер. */
static void *read_data(void *state)
{
    CallerRead *read = state;
    DVM_CancelScope scope = {0};
    if (dvm_cancel_enter(&scope, read->cancel))
    {
        if (!dvm_cancel_requested(read->cancel))
        {
            styxfileread(read->opened->file, read->offset, read->bytes, read->capacity, &read->result);
        }
        dvm_cancel_leave(&scope);
    }
    __atomic_store_n(&read->done, 1, __ATOMIC_RELEASE);
    styxphywake(read->wake);
    return NULL;
}


/* EN/RU: See caller_styx_state.h. / См. caller_styx_state.h. */
char *styxcaller_defer_read(StyxCallerServer *server, CallerOpen *opened, ulong *count, vlong offset)
{
    if (server->read_count >= CALLER_READ_LIMIT || *count > SIZE_MAX - sizeof(CallerRead))
    {
        return "too many pending reads";
    }
    CallerRead *read = calloc(1, sizeof(*read) + *count);
    if (!read)
    {
        return Enomem;
    }
    read->cancel = dvm_cancel_create();
    if (!read->cancel || !(read->request = styxdeferread(&server->server, read, release_read)))
    {
        dvm_cancel_release(read->cancel);
        free(read);
        return Enomem;
    }
    read->capacity = *count;
    read->offset = offset;
    read->opened = opened;
    read->wake = server->wake;
    read->next = server->reads;
    server->reads = read;
    ++server->read_count;
    opened->read = read;
    if (pthread_create(&read->thread, NULL, read_data, read))
    {
        __atomic_store_n(&read->done, 1, __ATOMIC_RELEASE);
        return Enomem;
    }
    read->started = 1;
    *count = 0;
    return nil;
}


/* EN: Complete read's wire request with its provider outcome. Return 1 if retired.
 * RU: Завершить запрос read результатом провайдера. Вернуть 1 после снятия запроса. */
static int send_read(CallerRead *read)
{
    const StyxTransfer *result = &read->result;
    if (result->confirmed && !result->uncertain && (result->success || result->count) &&
        result->count <= read->capacity)
    {
        return styxreadreply(read->request, read->bytes, result->count, nil);
    }
    return styxreadreply(read->request, nil, 0, result->again ? "resource temporarily unavailable" : Eperm);
}


/* EN/RU: See caller_styx_state.h. / См. caller_styx_state.h. */
void styxcaller_finish_reads(StyxCallerServer *server, int closing)
{
    CallerRead **link = &server->reads;
    while (*link)
    {
        CallerRead *read = *link;
        if (!closing && (!__atomic_load_n(&read->done, __ATOMIC_ACQUIRE) ||
            (read->request && !styxphyspace(&server->server))))
        {
            link = &read->next;
            continue;
        }
        if (read->request && !send_read(read))
        {
            link = &read->next;
            continue;
        }
        if (read->started)
        {
            pthread_join(read->thread, NULL);
        }
        read->opened->read = NULL;
        if (read->opened->retired)
        {
            styxcaller_close_open(read->opened);
        }
        *link = read->next;
        --server->read_count;
        dvm_cancel_release(read->cancel);
        free(read);
    }
}
