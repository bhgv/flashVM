/* Copyright (c) 2026 bhgv
 * EN: Private autonomous VFS request worker, cancellation, status and ownership contracts.
 * RU: Private-контракты VFS request worker, отмены, status и владения.
 */
#ifndef STYX_BIDIR_MOVIE_SERVER_INTERNAL_H
#define STYX_BIDIR_MOVIE_SERVER_INTERNAL_H
#include "server.h"
#include "internal.h"
#include "service.h"


/* EN/RU: No managed pointers; worker owns VFS handles, host owns final join and route lifetime.
 * Без managed pointers; worker владеет VFS handles, host — final join и lifetime route.
 */
struct StyxBidirMovieServer
{
    pthread_mutex_t lock;   /* Protects current status and terminal publication. */
    pthread_cond_t changed; /* Monotonic status readiness, no guest participation. */
    pthread_t worker;       /* Sole resource RX consumer and VFS request owner. */
    StyxBidirRoute *route;  /* Borrowed through final worker join and free. */
    DVM_Provider *service;  /* Owned real VFS service retaining live prepared namespace. */
    DVM_Cancel *cancel;     /* Owned independent native cancellation source. */
    DVM_CancelTap *retired; /* Owned route retirement tap; callback independently pins cancel only. */
    unsigned index;         /* Immutable exclusively assigned RX queue. */
    unsigned long tag;      /* Immutable independently granted TX binding. */
    unsigned timeout;       /* Entire request/response native budget in milliseconds. */
    int state;              /* WAITING/WORKING or first terminal outcome, under lock. */
};


/* EN/RU: Set active phase/first terminal state unless already terminal; borrowed server, native only.
 * Установить active phase/первый terminal status, если ещё не terminal; без guest/I/O.
 */
void movie_server_publish(StyxBidirMovieServer *server, int state);


/* EN/RU: Native worker entry; borrow server until host joins, no guest calls or hidden executor.
 * Native worker entry, server заимствован до join, без guest calls/скрытого executor.
 */
void *movie_server_run(void *server);

/* EN/RU: Subscribe to sticky route retirement, 1/0; private constructor only, source pin adopted on success.
 * Подписаться на sticky route retirement, 1/0; private constructor, source pin принят при успехе.
 */
int movie_server_watch(StyxBidirMovieServer *server);
#endif
