/* Copyright (c) 2026 bhgv
 * EN: Snapshot tag counts and wait for bounded native mux admission.
 * RU: Снимок тегов и ограниченное ожидание допуска mux.
 */


/* EN/RU: See mux.h. / См. mux.h. */
void styxmuxtagcounts(StyxMux *mux, unsigned *occupied, unsigned *capacity)
{
    pthread_mutex_lock(&mux->lock);
    *occupied = 0;
    *capacity = mux->limit;
    for (unsigned i = 0; i < mux->limit; i++)
    {
        *occupied += mux->slots[i] != NULL;
    }
    pthread_mutex_unlock(&mux->lock);
}


/* EN: Find a free tag under the mux lock. A waiting caller holds the reusable
 * client frame, while the mux engine releases tags independently.
 * RU: Найти свободный тег под замком mux. Ожидающий удерживает буфер клиента.
 * Поток mux освобождает теги независимо. */
static unsigned admission_slot(StyxMux *mux, StyxCancel *cancel, int wait)
{
    double end = wait ? styxmuxtime() + mux->timeout : 0;
    for (;;)
    {
        unsigned index = 0;
        while (index < mux->limit && mux->slots[index])
        {
            index++;
        }
        if (mux->failed || mux->stopping || (wait && styxcancelled(cancel)))
        {
            return mux->limit;
        }
        if (!wait || index < mux->limit || styxmuxtime() >= end)
        {
            return index;
        }
        struct timespec poll;
        if (clock_gettime(CLOCK_REALTIME, &poll))
        {
            return mux->limit;
        }
        /* A token waiting for its first tag is not yet bound to the mux. */
        poll.tv_nsec += 5 * 1000 * 1000;
        if (poll.tv_nsec >= 1000 * 1000 * 1000)
        {
            poll.tv_sec++;
            poll.tv_nsec -= 1000 * 1000 * 1000;
        }
        mux->admission_waiters++;
        pthread_cond_timedwait(&mux->ended, &mux->lock, &poll);
        mux->admission_waiters--;
    }
}
