/* Copyright (c) 2026 bhgv
 * EN: Optional standard9P no-fid heartbeat with bounded send/reply observation windows.
 * RU: Optional standard9P heartbeat без fid с bounded send/reply окнами наблюдения.
 */
#include "mux_internal.h"

/* EN/RU: See mux.h; reject invalid policy atomically, preserving admitted probe fences.
 * См. mux.h; invalid policy отклоняется атомарно, admitted probe fences сохраняются. */


/* [standalone-contract:styxmuxkeepalive]
 * EN: styxmuxkeepalive: native client heartbeat operations and lifetime.
 * RU: styxmuxkeepalive: нативные client-операции heartbeat и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * idle (unsigned idle): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * reply (unsigned reply): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * misses (unsigned misses): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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
        mux->beat.due = now + idle;
        styxmuxwake(mux);
    }
    pthread_mutex_unlock(&mux->lock);
    return valid;
}

/* EN/RU: See mux_internal.h; valid activity defers idle probing and refreshes a sent probe response window.
 * См. mux_internal.h; valid activity откладывает idle probe и обновляет reply window отправленного probe. */


/* [standalone-contract:styxbeattraffic]
 * EN: styxbeattraffic: native client heartbeat operations and lifetime.
 * RU: styxbeattraffic: нативные client-операции heartbeat и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxbeattraffic(StyxMux *mux)
{
    StyxBeat *beat = &mux->beat;
    double now = styxmuxtime();
    beat->due = now + beat->idle;
    if (beat->active && beat->sent == beat->length)
    {
        beat->missed = 0;
        beat->deadline = now + beat->budget;
        if (beat->suspect)
        {
            beat->suspect = 0;
            beat->notice = STYX_LINK_READY;
        }
    }
}

/* EN/RU: See mux_internal.h; retain a single probe across missed windows, never reuse its tags.
 * См. mux_internal.h; один probe удерживается во всех missed windows, tag не переиспользуется. */


/* [standalone-contract:styxbeattick]
 * EN: styxbeattick: native client heartbeat operations and lifetime.
 * RU: styxbeattick: нативные client-операции heartbeat и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * now (double now): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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
        request.tag = 2 * mux->limit + 1;
        request.oldtag = 2 * mux->limit + 2;
        if (convS2M(&request, beat->bytes, sizeof(beat->bytes)) != sizeof(beat->bytes))
        {
            styxmuxbroken(mux);
            return 0;
        }
        beat->length = sizeof(beat->bytes);
        if (mux->walk_tag)
        {
            size_t encoded = 0;
            beat->wire =
                walk_packet_write(WALK_PACKET_REQUEST, mux->walk_tag, NULL, beat->bytes, sizeof(beat->bytes), &encoded);
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
        beat->deadline = now + beat->budget;
    }
    if (beat->active && now >= beat->deadline)
    {
        double windows = (now - beat->deadline) / beat->budget + 1;
        if (windows >= beat->limit - beat->missed)
        {
            mux->terminal = STYX_LINK_TIMEOUT;
            styxmuxbroken(mux);
            return 0;
        }
        unsigned elapsed = (unsigned)windows;
        beat->missed += elapsed;
        beat->deadline += elapsed * beat->budget;
        beat->suspect = 1;
        beat->notice = beat->sent == beat->length ? STYX_LINK_SUSPECT : STYX_LINK_STALLED;
    }
    double until = (beat->active ? beat->deadline : beat->due) - now;
    return until > 120000 ? 120000 : (int)until + 1;
}

/* EN/RU: See mux_internal.h; once selected, finish probe before any other output frame.
 * См. mux_internal.h; начатый probe завершается до любого другого output frame. */


/* [standalone-contract:styxbeatsend]
 * EN: styxbeatsend: native client heartbeat operations and lifetime.
 * RU: styxbeatsend: нативные client-операции heartbeat и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxbeatsend(StyxMux *mux)
{
    StyxBeat *beat = &mux->beat;
    PhyTransfer out =
        phywrite(mux->phy, (beat->wire ? beat->wire : beat->bytes) + beat->sent, beat->length - beat->sent);
    if ((out.flags & PHY_UNCERTAIN) || (out.status != PHY_OK && out.status != PHY_AGAIN))
    {
        styxmuxbroken(mux);
        return;
    }
    beat->sent += out.count;
    if (beat->sent == beat->length)
    {
        beat->missed = 0;
        beat->deadline = styxmuxtime() + beat->budget;
    }
}

/* EN/RU: See mux_internal.h; unknown/duplicate/premature probe replies are protocol failure.
 * См. mux_internal.h; unknown/duplicate/premature probe replies означают protocol failure. */


/* [standalone-contract:styxbeatreply]
 * EN: styxbeatreply: native client heartbeat operations and lifetime.
 * RU: styxbeatreply: нативные client-операции heartbeat и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: -1, 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxbeatreply(StyxMux *mux)
{
    if (GBIT16(mux->receive + 5) != 2 * mux->limit + 1)
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


/* [standalone-contract:styxbeatreport]
 * EN: styxbeatreport: native client heartbeat operations and lifetime.
 * RU: styxbeatreport: нативные client-операции heartbeat и время жизни.
 * mux (StyxMux *mux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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
