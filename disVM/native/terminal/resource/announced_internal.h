/* Copyright (c) 2026 bhgv
 * EN: Private native offer receiver ownership, readiness and same-client generation fencing.
 * RU: Private-владение native offer receiver, readiness и same-client generation fencing.
 */
#ifndef STYX_BIDIR_MOVIE_ANNOUNCED_INTERNAL_H
#define STYX_BIDIR_MOVIE_ANNOUNCED_INTERNAL_H
#include "announced.h"
#include "../../movie/session_internal.h"
#include "../../movie/announcement.h"
#include "request_payload.h"
#include "cancel.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN/RU: No VM pointers; host keeps route alive through all users and final join/free.
 * Без VM pointers; host удерживает route до завершения users и final join/free.
 */
struct StyxBidirMovieReceiver
{
    pthread_mutex_t lock; /* Serializes accepted offer updates, readiness and final origin publication. */
    pthread_cond_t changed; /* Monotonic native readiness, never a guest polling loop. */
    pthread_t worker; /* Sole typed offer consumer. */
    StyxBidirRoute *route; /* Borrowed through final free. */
    StyxMovieSession *session; /* Owned exact-client session with exclusive typed feed claim. */
    DVM_Cancel *cancel; /* Owned local source, separately pinned by terminal watcher. */
    StyxWatch *link; /* Owned exact-client terminal subscription; callbacks reference only cancel. */
    unsigned index; /* Immutable exclusive offer queue index. */
    unsigned timeout; /* Initial native fetch readiness budget. */
    int claimed; /* Exclusive session feed was acquired. */
    int phase; /* READING or first terminal outcome, under lock. */
};


/* EN/RU: Native entry, borrowing receiver through join; no VM calls or competing raw stream reader.
 * Native entry, receiver заимствован до join; без VM calls/второго raw reader.
 */
void *movie_receiver_run(void *receiver);


/* EN/RU: Caller holds receiver mutex; retire session and publish first terminal phase, waking waiters.
 * Caller держит receiver mutex; завершить session и первый terminal phase, разбудить waiters.
 */
void movie_receiver_end(StyxBidirMovieReceiver *receiver, int phase);


/* EN/RU: Caller holds receiver mutex; authoritative native availability/optional generation, no I/O.
 * Caller держит receiver mutex; authoritative native availability/optional generation, без I/O.
 */
int movie_receiver_available(StyxBidirMovieReceiver *receiver, unsigned long long *generation);
#endif
