/* Copyright (c) 2026 bhgv
 * EN: Coherent autonomous resource server status and bounded native completion readiness.
 * RU: Согласованный status resource server и ограниченное native-ожидание завершения.
 */
#define _POSIX_C_SOURCE 200809L
#include "server_internal.h"


/* EN/RU: See server.h; copy status under its short native mutex.
 * См. server.h; копировать status под коротким native mutex.
 */
int styxbidirmovieserverstate(StyxBidirMovieServer *server)
{
    if (!server)
    {
        return -1;
    }
    pthread_mutex_lock(&server->lock);
    int state = server->state; /* Coherent active/first-terminal state. */
    pthread_mutex_unlock(&server->lock);
    return state;
}


/* EN/RU: See server.h; spurious notifications never restart the monotonic wait budget.
 * См. server.h; ложные notifications не перезапускают monotonic budget.
 */
int styxbidirmovieserverwait(StyxBidirMovieServer *server, unsigned timeout)
{
    if (!server || timeout > 120000)
    {
        return 0;
    }
    struct timespec deadline; /* Original native wait deadline. */
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return 0;
    }
    deadline.tv_sec += timeout / 1000;
    deadline.tv_nsec += (long)(timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    pthread_mutex_lock(&server->lock);
    while (server->state < STYX_MOVIE_SERVER_DONE && timeout)
    {
        if (pthread_cond_timedwait(&server->changed, &server->lock, &deadline))
        {
            break;
        }
    }
    int done = server->state >= STYX_MOVIE_SERVER_DONE; /* Terminal status is not a worker join. */
    pthread_mutex_unlock(&server->lock);
    return done;
}
