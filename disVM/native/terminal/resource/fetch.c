/* Copyright (c) 2026 bhgv
 * EN: Existing resource request/reply exchange through S-record queues, with native cancellation.
 * RU: Обмен существующими request/reply через S-очереди с native-отменой.
 */
#define _POSIX_C_SOURCE 200809L
#include "internal.h"


/* EN/RU: Stack-owned staging gate, used synchronously only through this fetch's reply state.
 * Gate staging на стеке используется только синхронно через reply state данного fetch.
 */
typedef struct MovieSink
{
    StyxBidirMovieClient *client; /* Borrowed throughout synchronous fetch. */
    FVM_DesktopWrite write; /* Borrowed native staging callback. */
    void *state; /* Borrowed caller context. */
    struct timespec deadline; /* One deadline for every chunk and queue wait. */
} MovieSink;


/* EN/RU: Gate borrowed data before/after one synchronous sink call; never replay a refused callback.
 * Проверить отмену/budget до и после одного sink call, отказ не повторяется.
 */
static FVM_Result staging(const void *bytes, size_t size, void *state)
{
    MovieSink *sink = state; /* Pinned owner-local stack gate. */
    if (dvm_cancel_requested(sink->client->cancel) || !movie_budget(&sink->deadline))
    {
        return FVM_ERROR;
    }
    FVM_Result result = sink->write(bytes, size, sink->state); /* Native sink owns its own I/O bounds. */
    if (result == FVM_OK && (dvm_cancel_requested(sink->client->cancel) || !movie_budget(&sink->deadline)))
    {
        result = FVM_ERROR;
    }
    return result;
}


/* EN/RU: Forward outer native scope cancellation only to a retained source, never a client/route pointer.
 * Перенести отмену внешнего native scope в retained source, без client/route pointer.
 */
static void forward(void *source)
{
    dvm_cancel_request(source);
}


/* EN/RU: Release native source after all forwarded callbacks retire, possibly after fetch returns.
 * Освободить native source после forwarded callbacks, возможно после возврата fetch.
 */
static void release(void *source)
{
    dvm_cancel_release(source);
}


/* EN/RU: See client.h; successful queue acceptance is not peer execution, responses remain mandatory.
 * См. client.h; принятие TX-очередью не означает исполнения peer, ответ обязателен.
 */
FVM_Result styxbidirmoviefetch(unsigned purpose, const char *reference, const char *base,
    FVM_DesktopWrite write, void *state, char *resolved, size_t capacity, void *userdata)
{
    StyxBidirMovieClient *client = userdata; /* Host pins this native one-shot handle. */
    unsigned char bytes[MOVIE_FRAME]; /* One outgoing request or copied incoming response. */
    size_t size; /* Exact request/reply body length. */
    if (!client || !write || !resolved || !capacity ||
        (purpose != FVM_DESKTOP_FETCH_CODE && purpose != FVM_DESKTOP_FETCH_DATA) ||
        !movie_request_payload(bytes, sizeof(bytes), &size,
            purpose == FVM_DESKTOP_FETCH_CODE ? MOVIE_CODE : MOVIE_DATA, reference, base))
    {
        return FVM_INVALID_ARGUMENT;
    }
    pthread_mutex_lock(&client->lock);
    int admitted = !client->used && !client->stopped; /* Exactly one valid attempt owns this queue. */
    if (admitted)
    {
        client->used = 1;
    }
    pthread_mutex_unlock(&client->lock);
    if (!admitted)
    {
        return FVM_ERROR;
    }
    MovieSink sink = {client, write, state, {0, 0}}; /* Synchronous owner-local staging. */
    FVM_Result result = FVM_ERROR; /* No partial stream may become success. */
    DVM_Cancel *outer = dvm_cancel_current(); /* Borrowed through the caller's existing scope. */
    DVM_CancelTap *tap = NULL; /* Optional outer cancellation forwarding subscription. */
    DVM_CancelScope scope = {0}; /* Own source becomes current during native sink calls. */
    if (outer)
    {
        if (!dvm_cancel_retain(client->cancel))
        {
            return FVM_ERROR;
        }
        tap = dvm_cancel_subscribe(outer, forward, client->cancel, release);
        if (!tap)
        {
            dvm_cancel_release(client->cancel);
            return FVM_ERROR;
        }
    }
    if (!dvm_cancel_enter(&scope, client->cancel) || dvm_cancel_requested(client->cancel) ||
        clock_gettime(CLOCK_MONOTONIC, &sink.deadline))
    {
        goto done;
    }
    sink.deadline.tv_sec += client->timeout / 1000;
    sink.deadline.tv_nsec += (long)(client->timeout % 1000) * 1000000L;
    if (sink.deadline.tv_nsec >= 1000000000L)
    {
        sink.deadline.tv_sec++;
        sink.deadline.tv_nsec -= 1000000000L;
    }
    if (styxbidirroutestate(client->route) || styxbidirroutetypedwait(client->route, client->index, 0) ||
        !styxbidirroutesend(client->route, client->tag, bytes, size))
    {
        goto done;
    }
    char origin[MOVIE_PATH]; /* Private origin until final success admission, not caller-visible staging. */
    MovieReply reply; /* Same state machine as the ordinary Styx resource client. */
    size_t output_capacity = capacity < sizeof(origin) ? capacity : sizeof(origin); /* Preserve caller's bound. */
    if (!movie_reply_init(&reply, staging, &sink, origin, output_capacity))
    {
        goto done;
    }
    for (;;)
    {
        unsigned remaining = movie_budget(&sink.deadline); /* Original budget, never restarted by a record. */
        if (!remaining || dvm_cancel_requested(client->cancel))
        {
            break;
        }
        unsigned long tag; /* Original RX tag, never reused as outgoing authority. */
        int received = styxbidirroutetypednext(client->route, client->index, &tag, bytes, sizeof(bytes), &size);
        if (received == STYX_CONTROL_EMPTY)
        {
            if (!styxbidirroutetypedwaitcancel(client->route, client->index, remaining, client->cancel))
            {
                break;
            }
            continue;
        }
        if (received != STYX_CONTROL_RECORD)
        {
            break;
        }
        int accepted = movie_reply_accept(&reply, bytes, size); /* Consume exactly one checked resource body. */
        if (accepted != MOVIE_REPLY_MORE)
        {
            result = movie_reply_result(&reply);
            break;
        }
    }
    pthread_mutex_lock(&client->lock);
    if (result == FVM_OK && !client->stopped && !dvm_cancel_requested(client->cancel) &&
        movie_budget(&sink.deadline) && !styxbidirroutestate(client->route))
    {
        memcpy(resolved, origin, reply.path_size + 1);
    }
    else if (result == FVM_OK)
    {
        result = FVM_ERROR;
    }
    pthread_mutex_unlock(&client->lock);
done:
    if (scope.cancel)
    {
        dvm_cancel_leave(&scope);
    }
    dvm_cancel_unsubscribe(tap);
    return result;
}
