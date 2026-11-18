/* Copyright (c) 2026 bhgv
 * EN: One native worker consumes existing typed offers without competing for UI/general records.
 * RU: Один native worker принимает существующие typed offers отдельно от UI/general records.
 */
#include "announced_internal.h"


/* EN/RU: See announced_internal.h; one record per iteration, native waits only, cancellation is local.
 * См. announced_internal.h; одна запись на итерацию, только native waits, отмена локальна.
 */
void *movie_receiver_run(void *state)
{
    StyxBidirMovieReceiver *receiver = state; /* Host pin through final join. */
    unsigned char bytes[MOVIE_OFFER_FRAME]; /* One bounded copied announcement body. */
    int phase = STYX_MOVIE_RECEIVER_CLOSED; /* Normal stream/link retirement unless malformed. */
    while (!dvm_cancel_requested(receiver->cancel))
    {
        unsigned long tag; /* Original RX metadata, never a fetch path or TX grant. */
        size_t size; /* Exact copied body extent. */
        int received = styxbidirroutetypednext(receiver->route, receiver->index, &tag, bytes, sizeof(bytes), &size);
        if (received == STYX_CONTROL_EMPTY)
        {
            if (styxbidirroutestate(receiver->route))
            {
                break;
            }
            styxbidirroutetypedwaitcancel(receiver->route, receiver->index, 120000, receiver->cancel);
            continue;
        }
        if (received != STYX_CONTROL_RECORD)
        {
            phase = received == STYX_CONTROL_END ? STYX_MOVIE_RECEIVER_CLOSED : STYX_MOVIE_RECEIVER_FAILED;
            break;
        }
        pthread_mutex_lock(&receiver->lock);
        int live = receiver->phase == STYX_MOVIE_RECEIVER_READING; /* Serialize with stop and final fetch publication. */
        int valid = live && movie_session_offer(receiver->session, bytes, size); /* Shared existing payload validation. */
        int available = valid ? styxmovieavailable(receiver->session, NULL) : -1; /* Session remains authoritative. */
        if (!valid || available < 0)
        {
            phase = valid || !live ? STYX_MOVIE_RECEIVER_CLOSED : STYX_MOVIE_RECEIVER_FAILED;
            movie_receiver_end(receiver, phase);
            pthread_mutex_unlock(&receiver->lock);
            dvm_cancel_request(receiver->cancel);
            return NULL;
        }
        pthread_cond_broadcast(&receiver->changed);
        pthread_mutex_unlock(&receiver->lock);
    }
    pthread_mutex_lock(&receiver->lock);
    movie_receiver_end(receiver, phase);
    pthread_mutex_unlock(&receiver->lock);
    dvm_cancel_request(receiver->cancel);
    return NULL;
}
