/* Copyright (c) 2026 bhgv
 * EN: Optional standard9P no-fid heartbeat with bounded send/reply observation windows.
 * RU: Optional standard9P heartbeat без fid с bounded send/reply окнами наблюдения.
 */
#include "mux_internal.h"


/* EN/RU: See mux.h; reject invalid policy atomically, preserving admitted probe fences.
 * См. mux.h; invalid policy отклоняется атомарно, admitted probe fences сохраняются. */
int styxmuxkeepalive(StyxMux *mux, unsigned idle, unsigned reply, unsigned misses)
{
    if ((idle || reply || misses) && (!idle || idle > 120000 || !reply || reply > 120000 || misses < 2 || misses > 16))
    {
        return 0;
    }
    double now = styxmuxtime();
    pthread_mutex_lock(&mux->lock);
    int valid = now >= 0 && !mux->failed && !mux->stopping;
    if (valid)
    {
        mux->beat.idle = idle;
        mux->beat.reply = reply;
        mux->beat.misses = misses;
        mux->beat.due = now+idle;
        styxmuxwake(mux);
    }
    pthread_mutex_unlock(&mux->lock);
    return valid;
}


/* EN/RU: See mux_internal.h; valid activity defers idle probing and refreshes a sent probe response window.
 * См. mux_internal.h; valid activity откладывает idle probe и обновляет reply window отправленного probe. */
void styxbeattraffic(StyxMux *mux)
{
    StyxBeat *beat = &mux->beat;
    double now = styxmuxtime();
    beat->due = now+beat->idle;
    if (beat->active && beat->sent == beat->length)
    {
        beat->missed = 0;
        beat->deadline = now+beat->budget;
        if (beat->suspect)
        {
            beat->suspect = 0;
            beat->notice = STYX_LINK_READY;
        }
    }
}


/* EN/RU: See mux_internal.h; retain a single probe across missed windows, never reuse its tags.
 * См. mux_internal.h; один probe удерживается во всех missed windows, tag не переиспользуется. */
int styxbeattick(StyxMux *mux, double now)
{
    StyxBeat *beat = &mux->beat;
    if (mux->failed || mux->stopping)
    {
        return -1;
    }
    if (!beat->active && !beat->idle)
    {
        return -1;
    }
    if (!beat->active && now >= beat->due)
    {
        Fcall request;
        memset(&request, 0, sizeof(request));
        request.type = Tflush;
        request.tag = 2*mux->limit+1;
        request.oldtag = 2*mux->limit+2;
        if (convS2M(&request, beat->bytes, sizeof(beat->bytes)) != sizeof(beat->bytes))
        {
            styxmuxbroken(mux);
            return 0;
        }
        beat->length = sizeof(beat->bytes);
        if (mux->walk_tag)
        {
            size_t encoded = 0;
            beat->wire = walk_packet_write(WALK_PACKET_REQUEST, mux->walk_tag, NULL,
                beat->bytes, sizeof(beat->bytes), &encoded);
            if (!beat->wire)
            {
                styxmuxbroken(mux);
                return 0;
            }
            beat->length = (unsigned)encoded;
        }
        beat->active = 1;
        beat->sent = 0;
        beat->missed = 0;
        beat->budget = beat->reply;
        beat->limit = beat->misses;
        beat->deadline = now+beat->budget;
    }
    if (beat->active && now >= beat->deadline)
    {
        double windows = (now-beat->deadline)/beat->budget+1;
        if (windows >= beat->limit-beat->missed)
        {
            mux->terminal = STYX_LINK_TIMEOUT;
            styxmuxbroken(mux);
            return 0;
        }
        unsigned elapsed = (unsigned)windows;
        beat->missed += elapsed;
        beat->deadline += elapsed*beat->budget;
        beat->suspect = 1;
        beat->notice = beat->sent == beat->length ? STYX_LINK_SUSPECT : STYX_LINK_STALLED;
    }
    double until = (beat->active ? beat->deadline : beat->due)-now;
    return until > 120000 ? 120000 : (int)until+1;
}


/* EN/RU: See mux_internal.h; once selected, finish probe before any other output frame.
 * См. mux_internal.h; начатый probe завершается до любого другого output frame. */
void styxbeatsend(StyxMux *mux)
{
    StyxBeat *beat = &mux->beat;
    PhyTransfer out = phywrite(mux->phy, (beat->wire ? beat->wire : beat->bytes) + beat->sent, beat->length - beat->sent);
    if ((out.flags & PHY_UNCERTAIN) || (out.status != PHY_OK && out.status != PHY_AGAIN))
    {
        styxmuxbroken(mux);
        return;
    }
    beat->sent += out.count;
    if (beat->sent == beat->length)
    {
        beat->missed = 0;
        beat->deadline = styxmuxtime()+beat->budget;
    }
}


/* EN/RU: See mux_internal.h; unknown/duplicate/premature probe replies are protocol failure.
 * См. mux_internal.h; unknown/duplicate/premature probe replies означают protocol failure. */
int styxbeatreply(StyxMux *mux)
{
    if (GBIT16(mux->receive+5) != 2*mux->limit+1)
    {
        return -1;
    }
    StyxBeat *beat = &mux->beat;
    Fcall reply;
    if (!beat->active || beat->sent != beat->length || mux->want != 7 ||
        convM2S(mux->receive, mux->want, &reply) != mux->want || reply.type != Rflush)
    {
        return 0;
    }
    beat->active = 0;
    if (beat->completed != ~0ULL)
    {
        beat->completed++;
    }
    pthread_cond_broadcast(&mux->ended);
    free(beat->wire);
    beat->wire = NULL;
    beat->length = 0;
    if (beat->suspect)
    {
        beat->notice = STYX_LINK_READY;
        beat->suspect = 0;
    }
    styxbeattraffic(mux);
    return 1;
}


/* EN/RU: See mux_internal.h; callback publication never owns engine lock or touches VM state.
 * См. mux_internal.h; publication не удерживает engine lock и не трогает VM state. */
void styxbeatreport(StyxMux *mux)
{
    int state = mux->beat.notice;
    mux->beat.notice = 0;
    if (state)
    {
        pthread_mutex_unlock(&mux->lock);
        styxlifechange(mux->life, state);
        pthread_mutex_lock(&mux->lock);
    }
}
