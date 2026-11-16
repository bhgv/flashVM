/* Copyright (c) 2026 bhgv
 * EN: Bounded multiplex admission, native wakeup and nonwaiting owner retirement.
 * RU: Bounded multiplex admission, native wakeup и nonwaiting owner retirement.
 */
#include "mux_internal.h"


/* EN/RU: See mux_internal.h; cleanup owns descriptors, buffers and native locks.
 * См. mux_internal.h; cleanup владеет descriptors, buffers и locks. */
void styxmuxdestroy(StyxMux *mux)
{
    close(mux->wake[0]);
    close(mux->wake[1]);
    free(mux->receive);
    free(mux->decode);
    free(mux->slots);
    styxlifedrop(mux->life);
    pthread_cond_destroy(&mux->ended);
    pthread_mutex_destroy(&mux->lock);
    free(mux);
}


/* EN/RU: See mux.h; a full pipe already represents pending native work.
 * См. mux.h; full pipe уже означает pending native work. */
void styxmuxwake(StyxMux *mux)
{
    char byte = 1;
    ssize_t n;
    do
    {
        n = write(mux->wake[1], &byte, 1);
    } while (n < 0 && errno == EINTR);
}


/* EN/RU: See mux_internal.h; remove before waking so caller cannot free live table storage.
 * См. mux_internal.h; убрать до wake, чтобы caller не освободил live table storage. */
void styxmuxfinish(StyxMux *mux, StyxFlight *flight)
{
    mux->slots[flight->slot] = NULL;
    if (mux->sending == flight)
    {
        mux->sending = NULL;
    }
    if (!flight->original)
    {
        flight->answer.status = -1;
        flight->answer.cancelled = flight->flushing;
    }
    flight->done = 1;
    pthread_cond_signal(&flight->condition);
}


/* EN/RU: See mux_internal.h; acknowledged effects remain acknowledged even if flush fails.
 * См. mux_internal.h; known acknowledgement сохраняется даже при отказе flush. */
void styxmuxbroken(StyxMux *mux)
{
    if (!mux->terminal)
    {
        mux->terminal = mux->stopping ? (mux->localstop ? STYX_LINK_CLOSED : 0) : STYX_LINK_FAILED;
    }
    mux->failed = 1;
    for (unsigned i = 0; i < mux->limit; i++)
    {
        if (mux->slots[i])
        {
            styxmuxfinish(mux, mux->slots[i]);
        }
    }
}


/* EN/RU: See mux.h; native engine methods never acquire a caller's Client lock.
 * См. mux.h; engine methods не захватывают caller Client lock. */
void styxmuxfail(StyxMux *mux)
{
    pthread_mutex_lock(&mux->lock);
    styxmuxbroken(mux);
    styxmuxwake(mux);
    pthread_mutex_unlock(&mux->lock);
}


/* EN/RU: Local terminal-state snapshot, no protocol or transport probing.
 * Local terminal-state snapshot без protocol/transport probing. */
int styxmuxalive(StyxMux *mux)
{
    pthread_mutex_lock(&mux->lock);
    int alive = !mux->failed && !mux->stopping;
    pthread_mutex_unlock(&mux->lock);
    return alive;
}


/* EN/RU: See mux.h; reject quota/OOM/token reuse before effects. Watching reads reserve
 * no more than limit-1 slots, retaining ordinary RPC capacity. Request bytes are copied.
 * См. mux.h; refusal до effects; watching занимает максимум limit-1 slots, bytes copied. */
StyxFlight *styxmuxsubmit(StyxMux *mux, const void *bytes, unsigned length, unsigned bound, StyxCancel *cancel, int watching)
{
    pthread_mutex_lock(&mux->lock);
    unsigned index;
    for (index = 0; index < mux->limit && mux->slots[index]; index++)
    {
    }
    unsigned watches = 0;
    for (unsigned i = 0; i < mux->limit; i++)
    {
        watches += mux->slots[i] && mux->slots[i]->watching;
    }
    if (mux->failed || mux->stopping || index == mux->limit || length < 7 || length > bound || bound > mux->capacity ||
        (watching && (mux->limit < 2 || watches >= mux->limit-1 || ((const uchar *)bytes)[4] != Tread)))
    {
        pthread_mutex_unlock(&mux->lock);
        return NULL;
    }
    StyxFlight *flight = calloc(1, sizeof(*flight));
    if (!flight)
    {
        pthread_mutex_unlock(&mux->lock);
        return NULL;
    }
    flight->request = malloc(length);
    flight->answer.bytes = malloc(bound);
    if (!flight->request || !flight->answer.bytes || pthread_cond_init(&flight->condition, NULL))
    {
        free(flight->request);
        free(flight->answer.bytes);
        free(flight);
        pthread_mutex_unlock(&mux->lock);
        return NULL;
    }
    if (!styxcancelbind(cancel, mux))
    {
        pthread_cond_destroy(&flight->condition);
        free(flight->request);
        free(flight->answer.bytes);
        free(flight);
        pthread_mutex_unlock(&mux->lock);
        return NULL;
    }
    flight->watching = watching;
    flight->cancel = cancel;
    flight->mux = mux;
    flight->slot = index;
    flight->tag = ((const uchar *)bytes)[4] == Tversion ? NOTAG : index+1;
    flight->flush = mux->limit+index+1;
    flight->bound = bound;
    flight->length = length;
    memcpy(flight->request, bytes, length);
    PBIT16(flight->request+5, flight->tag);
    flight->deadline = styxmuxtime()+mux->timeout;
    mux->slots[index] = flight;
    styxmuxwake(mux);
    pthread_mutex_unlock(&mux->lock);
    return flight;
}


/* EN/RU: Wait/collect only on native caller; detach token before engine owner can retire.
 * Wait/collect только native caller; detach token до engine owner retirement. */
StyxAnswer styxmuxcollect(StyxFlight *flight)
{
    StyxMux *mux = flight->mux;
    pthread_mutex_lock(&mux->lock);
    while (!flight->done)
    {
        pthread_cond_wait(&flight->condition, &mux->lock);
    }
    StyxAnswer answer = flight->answer;
    pthread_mutex_unlock(&mux->lock);
    styxcancelunbind(flight->cancel);
    pthread_cond_destroy(&flight->condition);
    free(flight->request);
    free(flight);
    return answer;
}


/* EN/RU: See mux.h; explicit native shutdown can wait, final provider destruction cannot.
 * См. mux.h; explicit native shutdown может ждать, final provider destruction — нет. */
void styxmuxshutdown(StyxMux *mux)
{
    pthread_mutex_lock(&mux->lock);
    mux->localstop = 1;
    mux->stopping = 1;
    styxmuxwake(mux);
    while (!mux->finished)
    {
        pthread_cond_wait(&mux->ended, &mux->lock);
    }
    pthread_mutex_unlock(&mux->lock);
}


/* EN/RU: See mux.h; worker owns its remaining reference and eventually destroys the engine.
 * См. mux.h; worker владеет оставшейся ссылкой и завершает engine. */
void styxmuxfree(StyxMux *mux)
{
    pthread_mutex_lock(&mux->lock);
    mux->stopping = 1;
    styxmuxwake(mux);
    int last = --mux->references == 0;
    pthread_mutex_unlock(&mux->lock);
    if (last)
    {
        styxmuxdestroy(mux);
    }
}


/* EN/RU: See mux.h; adopt only nonblocking connected stream sockets, detached native owner.
 * См. mux.h; принять connected stream socket, detached native owner. */
StyxMux *styxmuxcreate(int fd, unsigned capacity, unsigned timeout, unsigned limit)
{
    int pipes[2] = {-1, -1};
    int type;
    socklen_t size = sizeof(type);
    struct linger linger = {0, 0};
    int flags = fd >= 0 ? fcntl(fd, F_GETFL, 0) : -1;
    if (fd < 0 || capacity < 256 || capacity > 65536 || !timeout || timeout > 120000 || !limit || limit > 128 ||
        flags < 0 || getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &size) || type != SOCK_STREAM ||
        fcntl(fd, F_SETFL, flags|O_NONBLOCK) || fcntl(fd, F_SETFD, FD_CLOEXEC) || setsockopt(fd, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) || pipe(pipes))
    {
        if (fd >= 0)
        {
            close(fd);
        }
        return NULL;
    }
    if (fcntl(pipes[0], F_SETFL, O_NONBLOCK) || fcntl(pipes[1], F_SETFL, O_NONBLOCK) ||
        fcntl(pipes[0], F_SETFD, FD_CLOEXEC) || fcntl(pipes[1], F_SETFD, FD_CLOEXEC))
    {
        close(fd);
        close(pipes[0]);
        close(pipes[1]);
        return NULL;
    }
    StyxMux *mux = calloc(1, sizeof(*mux));
    if (!mux)
    {
        close(fd);
        close(pipes[0]);
        close(pipes[1]);
        return NULL;
    }
    if (pthread_mutex_init(&mux->lock, NULL))
    {
        free(mux);
        close(fd);
        close(pipes[0]);
        close(pipes[1]);
        return NULL;
    }
    if (pthread_cond_init(&mux->ended, NULL))
    {
        pthread_mutex_destroy(&mux->lock);
        free(mux);
        close(fd);
        close(pipes[0]);
        close(pipes[1]);
        return NULL;
    }
    mux->fd = fd;
    mux->wake[0] = pipes[0];
    mux->wake[1] = pipes[1];
    mux->capacity = capacity;
    mux->timeout = timeout;
    mux->limit = limit;
    mux->references = 2;
    mux->want = 4;
    mux->receive = malloc(capacity);
    mux->decode = malloc(capacity);
    mux->slots = calloc(limit, sizeof(*mux->slots));
    mux->life = styxlifecreate();
    pthread_t thread;
    pthread_attr_t attributes;
    int initialized = pthread_attr_init(&attributes) == 0;
    int good = initialized && !pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
    if (!mux->receive || !mux->decode || !mux->slots || !mux->life || !good || pthread_create(&thread, &attributes, styxmuxrun, mux))
    {
        if (initialized)
        {
            pthread_attr_destroy(&attributes);
        }
        close(fd);
        styxmuxdestroy(mux);
        return NULL;
    }
    pthread_attr_destroy(&attributes);
    return mux;
}


/* EN/RU: See mux.h; caller retains engine owner through any source retain.
 * См. mux.h; caller удерживает engine owner во время source retain. */
StyxLife *styxmuxlife(StyxMux *mux)
{
    return mux->life;
}


/* EN/RU: See mux.h; native state snapshot only, no wire request/admission reservation.
 * См. mux.h; только native snapshot без wire request/reservation. */
int styxmuxwatchable(StyxMux *mux)
{
    pthread_mutex_lock(&mux->lock);
    int valid = !mux->failed && !mux->stopping && mux->limit >= 2;
    pthread_mutex_unlock(&mux->lock);
    return valid;
}
