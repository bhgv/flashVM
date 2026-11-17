/* Copyright (c) 2026 bhgv
 * EN: Autonomous native fd owner, ordered frame sends, reply routing and flush fences.
 * RU: Autonomous native fd owner, ordered sends, reply routing и flush fences.
 */
#include "mux_internal.h"


/* EN/RU: See mux_internal.h; monotonic deadlines cannot renew on partial progress.
 * См. mux_internal.h; deadline не обновляется при partial progress. */
double styxmuxtime(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value))
    {
        return -1;
    }
    return value.tv_sec*1000.0+value.tv_nsec/1000000.0;
}


/* EN/RU: Cancel unsent work locally; sent work gets one original-codec flush/grace.
 * Отменить unsent work локально; sent work получает один flush/grace. */
static int deadlines(StyxMux *mux, double now)
{
    double remaining = 120000;
    int active = 0;
    for (unsigned i = 0; i < mux->limit; i++)
    {
        StyxFlight *flight = mux->slots[i];
        if (!flight)
        {
            continue;
        }
        if (!flight->flushing && ((!flight->watching && now >= flight->deadline) || styxcancelled(flight->cancel)))
        {
            if (flight->request[4] == Tversion)
            {
                styxmuxbroken(mux);
                return 0;
            }
            flight->flushing = 1;
            flight->flushdeadline = now+mux->timeout;
            if (!flight->answer.attempted)
            {
                styxmuxfinish(mux, flight);
                continue;
            }
            Fcall flush;
            memset(&flush, 0, sizeof(flush));
            flush.type = Tflush;
            flush.tag = flight->flush;
            flush.oldtag = flight->tag;
            if (convS2M(&flush, flight->flushbytes, sizeof(flight->flushbytes)) != sizeof(flight->flushbytes))
            {
                styxmuxbroken(mux);
                return 0;
            }
        }
        if (flight->watching && !flight->flushing)
        {
            continue;
        }
        active = 1;
        double until = (flight->flushing ? flight->flushdeadline : flight->deadline)-now;
        if (until <= 0)
        {
            styxmuxbroken(mux);
            return 0;
        }
        if (until < remaining)
        {
            remaining = until;
        }
    }
    return active ? (int)remaining+1 : -1;
}


/* EN/RU: Never interleave frames; flush priority follows completion of original bytes.
 * Не смешивать frames; flush priority только после полной отправки original bytes. */
static void selectframe(StyxMux *mux)
{
    if (mux->sending || (mux->beat.active && mux->beat.sent && mux->beat.sent < sizeof(mux->beat.bytes)))
    {
        return;
    }
    for (unsigned i = 0; i < mux->limit; i++)
    {
        StyxFlight *flight = mux->slots[i];
        if (flight && flight->flushing && flight->sent == flight->length && !flight->flushsent)
        {
            mux->sending = flight;
            mux->sendflush = 1;
            return;
        }
    }
    if (mux->beat.active && mux->beat.sent < sizeof(mux->beat.bytes))
    {
        return;
    }
    for (unsigned ordinal = 0; ordinal < mux->limit; ordinal++)
    {
        unsigned i = (mux->next+ordinal)%mux->limit;
        StyxFlight *flight = mux->slots[i];
        if (flight && flight->sent < flight->length)
        {
            mux->sending = flight;
            mux->sendflush = 0;
            mux->next = (i+1)%mux->limit;
            return;
        }
    }
}


/* EN/RU: Nonblocking sends acknowledge accepted bytes only; failed partial frames are fatal.
 * Nonblocking send учитывает accepted bytes; ошибка partial frame фатальна. */
static void sendframe(StyxMux *mux)
{
    selectframe(mux);
    StyxFlight *flight = mux->sending;
    if (!flight)
    {
        if (mux->beat.active && mux->beat.sent < sizeof(mux->beat.bytes))
        {
            styxbeatsend(mux);
        }
        return;
    }
    unsigned *position = mux->sendflush ? &flight->flushsent : &flight->sent;
    unsigned length = mux->sendflush ? sizeof(flight->flushbytes) : flight->length;
    uchar *bytes = mux->sendflush ? flight->flushbytes : flight->request;
    PhyTransfer out = phywrite(mux->phy, bytes+*position, length-*position);
    if (!mux->sendflush && (out.count || out.status != PHY_AGAIN || (out.flags & PHY_UNCERTAIN)))
    {
        flight->answer.attempted = 1;
    }
    *position += out.count;
    if ((out.flags & PHY_UNCERTAIN) || (out.status != PHY_OK && out.status != PHY_AGAIN))
    {
        styxmuxbroken(mux);
        return;
    }
    if (*position == length)
    {
        mux->sending = NULL;
    }
}


/* EN/RU: Route only leased tags and exact response kinds; Rerror to Tflush is invalid.
 * Маршрутизировать только leased tags и точные kinds; Rerror для Tflush недопустим. */
static int dispatch(StyxMux *mux)
{
    int probe = styxbeatreply(mux);
    if (probe >= 0)
    {
        return probe;
    }
    ushort tag = GBIT16(mux->receive+5);
    StyxFlight *flight = NULL;
    int flush = 0;
    for (unsigned i = 0; i < mux->limit; i++)
    {
        StyxFlight *candidate = mux->slots[i];
        if (candidate && (tag == candidate->tag || tag == candidate->flush))
        {
            flight = candidate;
            flush = tag == candidate->flush;
            break;
        }
    }
    if (!flight || mux->want > flight->bound)
    {
        return 0;
    }
    if (!flush && (flight->original || flight->sent != flight->length))
    {
        return 0;
    }
    memcpy(mux->decode, mux->receive, mux->want);
    Fcall reply;
    if (convM2S(mux->decode, mux->want, &reply) != mux->want)
    {
        return 0;
    }
    if (flush)
    {
        if (!flight->flushing || flight->flushsent != sizeof(flight->flushbytes) || reply.type != Rflush)
        {
            return 0;
        }
        styxmuxfinish(mux, flight);
        styxbeattraffic(mux);
        return 1;
    }
    if (flight->sent != flight->length || flight->original ||
        (reply.type != flight->request[4]+1 && reply.type != Rerror))
    {
        return 0;
    }
    if ((reply.type == Rwalk && reply.nwqid > GBIT16(flight->request+15)) ||
        ((reply.type == Rread || reply.type == Rwrite) && reply.count > GBIT32(flight->request+19)))
    {
        return 0;
    }
    memcpy(flight->answer.bytes, mux->receive, mux->want);
    flight->answer.length = mux->want;
    flight->answer.status = reply.type == Rerror ? 0 : 1;
    flight->original = 1;
    if (!flight->flushsent)
    {
        styxmuxfinish(mux, flight);
    }
    styxbeattraffic(mux);
    return 1;
}


/* EN/RU: Drain complete bounded frames before handling EOF so known acknowledgements win.
 * Разобрать полные bounded frames до EOF, сохраняя known acknowledgements. */
static void receiveframes(StyxMux *mux)
{
    for (;;)
    {
        PhyTransfer out = phyread(mux->phy, mux->receive+mux->used, mux->want-mux->used);
        int terminal = (out.flags & PHY_UNCERTAIN) || (out.status != PHY_OK && out.status != PHY_AGAIN);
        mux->used += out.count;
        if (!out.count)
        {
            if (terminal)
            {
                styxmuxbroken(mux);
            }
            return;
        }
        if (mux->used == 4 && mux->want == 4)
        {
            mux->want = GBIT32(mux->receive);
            if (mux->want < 7 || mux->want > mux->capacity)
            {
                styxmuxbroken(mux);
                return;
            }
        }
        if (mux->used == mux->want)
        {
            if (!dispatch(mux))
            {
                styxmuxbroken(mux);
                return;
            }
            mux->used = 0;
            mux->want = 4;
        }
        if (terminal)
        {
            styxmuxbroken(mux);
            return;
        }
    }
}


/* EN/RU: See mux_internal.h; only this native OS thread performs carrier read/write/wait/disconnect.
 * См. mux_internal.h; только этот native OS thread выполняет carrier I/O. */
void *styxmuxrun(void *argument)
{
    StyxMux *mux = argument;
    pthread_mutex_lock(&mux->lock);
    while (!mux->failed && !mux->stopping)
    {
        double now = styxmuxtime();
        if (now < 0)
        {
            styxmuxbroken(mux);
            break;
        }
        int timeout = deadlines(mux, now);
        int heartbeat = styxbeattick(mux, now);
        if (heartbeat >= 0 && (timeout < 0 || heartbeat < timeout))
        {
            timeout = heartbeat;
        }
        styxbeatreport(mux);
        selectframe(mux);
        if (mux->failed)
        {
            break;
        }
        unsigned wanted = PHY_READ;
        if (mux->sending || (mux->beat.active && mux->beat.sent < sizeof(mux->beat.bytes)))
        {
            wanted |= PHY_WRITE;
        }
        unsigned ready = 0;
        pthread_mutex_unlock(&mux->lock);
        int status = phywait(mux->phy, wanted, timeout, &ready);
        pthread_mutex_lock(&mux->lock);
        if (status != PHY_OK || (ready & PHY_FAILED))
        {
            styxmuxbroken(mux);
            break;
        }
        if (ready & (PHY_READ|PHY_HANGUP))
        {
            receiveframes(mux);
            styxbeatreport(mux);
        }
        if (mux->stopping || mux->failed)
        {
            break;
        }
        now = styxmuxtime();
        if (now < 0)
        {
            styxmuxbroken(mux);
            break;
        }
        deadlines(mux, now);
        styxbeattick(mux, now);
        styxbeatreport(mux);
        if (!mux->failed && !mux->stopping)
        {
            sendframe(mux);
        }
    }
    styxmuxbroken(mux);
    pthread_mutex_unlock(&mux->lock);
    phydisconnect(mux->phy);
    pthread_mutex_lock(&mux->lock);
    mux->finished = 1;
    pthread_cond_broadcast(&mux->ended);
    int reason = mux->terminal;
    pthread_mutex_unlock(&mux->lock);
    if (reason)
    {
        styxlifeemit(mux->life, reason);
    }
    pthread_mutex_lock(&mux->lock);
    int last = --mux->references == 0;
    pthread_mutex_unlock(&mux->lock);
    if (last)
    {
        styxmuxdestroy(mux);
    }
    return NULL;
}
