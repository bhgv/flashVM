/* Copyright (c) 2026 bhgv
 * EN: Native one-shot admission state and monotonic budget shared by resource client components.
 * RU: Native-состояние одноразового допуска и monotonic budget частей resource client.
 */
#ifndef STYX_BIDIR_MOVIE_INTERNAL_H
#define STYX_BIDIR_MOVIE_INTERNAL_H
#include "client.h"
#include "cancel.h"
#include "reply.h"
#include "request_payload.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* EN/RU: Native state only; host pins route and retires external callers before disposal.
 * Только native-состояние; host удерживает route и завершает callers перед освобождением.
 */
struct StyxBidirMovieClient
{
    pthread_mutex_t lock;   /* Protects one-use admission and explicit stop. */
    StyxBidirRoute *route;  /* Borrowed through final free; no automatic route shutdown. */
    DVM_Cancel *cancel;     /* Owned independent native cancellation source. */
    DVM_CancelTap *retired; /* Owned route subscription; callback retains only cancel. */
    unsigned index;         /* Immutable exclusively assigned resource queue. */
    unsigned long tag;      /* Immutable independently granted TX tag. */
    unsigned timeout;       /* Immutable total native budget in milliseconds. */
    int used;               /* Sticky once a valid fetch enters, success or failure. */
    int stopped;            /* Sticky explicit host cancellation. */
};


/* EN/RU: Return remaining monotonic budget rounded up to ms, capped at 120000; 0 elapsed/clock failure.
 * Остаток monotonic budget с округлением вверх до ms, максимум 120000; 0 истёк/ошибка clock.
 */
unsigned movie_budget(const struct timespec *deadline);
#endif
