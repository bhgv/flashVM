/* Copyright (c) 2026 bhgv
 * EN: Thread-isolated logical PHY callbacks, bounded local acceptance and sticky native wake.
 * RU: Изолированные logical PHY callbacks, bounded local acceptance и sticky native wake.
 */
#include "internal.h"


/* EN/RU: Read confirmed queued input before reporting terminal peer/hub state; borrowed bytes.
 * Сначала прочитать confirmed input, затем terminal state; bytes borrowed. */
static PhyTransfer readbytes(void *state, void *bytes, size_t size)
{
    MuxChannel *channel = state;
    PhyMux *hub = channel->hub;
    PhyTransfer out = {0, PHY_AGAIN, PHY_PREFIX};
    pthread_mutex_lock(&hub->lock);
    if (channel->rx.count)
    {
        out.count = size < channel->rx.count ? size : channel->rx.count;
        muxget(&channel->rx, hub->capacity, bytes, out.count);
        channel->grant += out.count;
        out.status = PHY_OK;
    }
    else if (hub->failed || hub->stop)
    {
        out.status = PHY_ERROR;
    }
    else if (channel->remote)
    {
        out.status = PHY_EOF;
    }
    muxsignal(hub);
    pthread_mutex_unlock(&hub->lock);
    phywake(hub->carrier);
    return out;
}


/* EN/RU: Copy a bounded prefix into local queue, never claim remote delivery; borrowed input.
 * Скопировать bounded prefix в local queue, без обещания remote delivery; input borrowed. */
static PhyTransfer writebytes(void *state, const void *bytes, size_t size)
{
    MuxChannel *channel = state;
    PhyMux *hub = channel->hub;
    PhyTransfer out = {0, PHY_AGAIN, PHY_PREFIX};
    pthread_mutex_lock(&hub->lock);
    if (hub->failed || hub->stop || channel->remote || channel->closed)
    {
        out.status = PHY_ERROR;
    }
    else
    {
        size_t space = hub->capacity-channel->tx.count;
        out.count = size < space ? size : space;
        if (out.count)
        {
            muxput(&channel->tx, hub->capacity, bytes, out.count);
            out.status = PHY_OK;
        }
    }
    muxsignal(hub);
    pthread_mutex_unlock(&hub->lock);
    phywake(hub->carrier);
    return out;
}


/* EN/RU: Compute readiness under hub lock without consuming input/wake. / Readiness под lock без consume. */
static unsigned readiness(MuxChannel *channel, unsigned wanted)
{
    PhyMux *hub = channel->hub;
    unsigned ready = __atomic_load_n(&channel->wake, __ATOMIC_ACQUIRE) ? PHY_WAKE : 0;
    if ((wanted & PHY_READ) && channel->rx.count)
    {
        ready |= PHY_READ;
    }
    if ((wanted & PHY_WRITE) && channel->tx.count < hub->capacity && !channel->closed && !channel->remote)
    {
        ready |= PHY_WRITE;
    }
    if (channel->remote || channel->closed)
    {
        ready |= PHY_HANGUP;
    }
    if (hub->failed || hub->stop)
    {
        ready |= PHY_HANGUP;
        if (!channel->rx.count)
        {
            ready |= PHY_FAILED;
        }
    }
    return ready;
}


/* EN/RU: Native wait with one absolute monotonic timeout; sticky wake survives early signal.
 * Native wait с одним absolute monotonic timeout; early wake сохраняется. */
static int waitready(void *state, unsigned wanted, int timeout, unsigned *ready)
{
    MuxChannel *channel = state;
    PhyMux *hub = channel->hub;
    struct timespec deadline;
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return PHY_ERROR;
    }
    if (timeout > 0)
    {
        deadline.tv_sec += timeout/1000;
        deadline.tv_nsec += (timeout%1000)*1000000L;
        deadline.tv_sec += deadline.tv_nsec/1000000000L;
        deadline.tv_nsec %= 1000000000L;
    }
    for (;;)
    {
        unsigned generation = __atomic_load_n(&channel->generation, __ATOMIC_ACQUIRE);
        pthread_mutex_lock(&hub->lock);
        *ready = readiness(channel, wanted);
        if (*ready & PHY_WAKE)
        {
            __atomic_exchange_n(&channel->wake, 0, __ATOMIC_ACQ_REL);
        }
        pthread_mutex_unlock(&hub->lock);
        if (*ready || !timeout)
        {
            return PHY_OK;
        }
        struct timespec remaining;
        if (timeout > 0)
        {
            struct timespec current;
            if (clock_gettime(CLOCK_MONOTONIC, &current))
            {
                return PHY_ERROR;
            }
            remaining.tv_sec = deadline.tv_sec-current.tv_sec;
            remaining.tv_nsec = deadline.tv_nsec-current.tv_nsec;
            if (remaining.tv_nsec < 0)
            {
                remaining.tv_sec--;
                remaining.tv_nsec += 1000000000L;
            }
            if (remaining.tv_sec < 0 || (!remaining.tv_sec && !remaining.tv_nsec))
            {
                return PHY_OK;
            }
        }
        if (muxsleep(channel, generation, timeout < 0 ? NULL : &remaining))
        {
            return PHY_ERROR;
        }
    }
}


/* EN/RU: Pin-owned nonwaiting native local interrupt, no physical/wire operation. / Sticky local interrupt без wire I/O. */
static void wake(void *state)
{
    MuxChannel *channel = state;
    __atomic_store_n(&channel->wake, 1, __ATOMIC_RELEASE);
    muxsignalchannel(channel);
}


/* EN/RU: Copy queue byte hint under lock; no EOF or access inference. / Byte hint под lock, не EOF/permission. */
static int available(void *state, size_t *bytes)
{
    MuxChannel *channel = state;
    pthread_mutex_lock(&channel->hub->lock);
    *bytes = channel->rx.count;
    pthread_mutex_unlock(&channel->hub->lock);
    return PHY_OK;
}


/* EN/RU: Abort this logical channel only; in-flight physical frame finishes before ordered CLOSE.
 * Закрыть только logical channel; in-flight frame завершается перед ordered CLOSE. */
static void disconnect(void *state)
{
    MuxChannel *channel = state;
    PhyMux *hub = channel->hub;
    pthread_mutex_lock(&hub->lock);
    channel->closed = 1;
    channel->tx.count = 0;
    muxsignal(hub);
    pthread_mutex_unlock(&hub->lock);
    phywake(hub->carrier);
}


/* EN/RU: Consume endpoint ref after its I/O/wake users retire; channel memory owned by hub.
 * Поглотить endpoint ref после I/O/wake users; память channel принадлежит hub. */
static void destroy(void *state)
{
    muxdrop(((MuxChannel *)state)->hub);
}


/* EN/RU: See internal.h; callbacks operate on hub-owned memory, no state adoption on failure. / См. internal.h. */
RemotePhy *muxendpoint(MuxChannel *channel)
{
    static const PhyOps ops = {readbytes, writebytes, waitready, wake, available, NULL, disconnect, destroy};
    PhyInfo info = {PHY_STREAM|PHY_ORDERED|PHY_RELIABLE, 0, 0, 0, 1};
    return phycreate(&ops, channel, &info);
}
