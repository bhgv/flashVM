/* Copyright (c) 2026 bhgv
 * EN: Collect a reply while optionally checking the caller's operation stop flag.
 * RU: Ожидать ответ с необязательной проверкой флага отмены у вызывающего.
 */
#define STYX_STOP_POLL_NS 10000000L /* Maximum nominal interval between caller stop checks. */


/* EN: Wait once on flight with its mux locked. Release that lock during the
 * optional stop hook and regain it before returning to collection.
 * RU: Один раз ждать flight под замком мультиплексора. Освободить замок
 * на время проверки отмены и восстановить его перед возвратом к ожиданию. */
static void wait_for_reply(StyxFlight *flight)
{
    StyxMux *mux = flight->mux;
    struct timespec check;
    if (!styxcancelpolling(flight->cancel) || clock_gettime(CLOCK_REALTIME, &check))
    {
        pthread_cond_wait(&flight->condition, &mux->lock);
        return;
    }
    check.tv_nsec += STYX_STOP_POLL_NS;
    if (check.tv_nsec >= 1000000000L)
    {
        check.tv_nsec -= 1000000000L;
        ++check.tv_sec;
    }
    if (pthread_cond_timedwait(&flight->condition, &mux->lock, &check) == ETIMEDOUT && !flight->done)
    {
        pthread_mutex_unlock(&mux->lock);
        styxcancelpoll(flight->cancel);
        pthread_mutex_lock(&mux->lock);
    }
}
