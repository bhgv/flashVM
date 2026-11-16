/* Copyright (c) 2026 bhgv
 * EN: Native cancellation capability; event-driven wakeup, no guest polling.
 * RU: Native cancellation capability; event-driven wakeup без guest polling.
 */
#include "mux.h"
#include <pthread.h>
#include <stdlib.h>


/* EN/RU: Caller ref plus one admitted native flight keep the token alive.
 * Caller ref и один admitted native flight удерживают token. */
struct StyxCancel
{
    pthread_mutex_t lock; /* Serialize request/bind/lifetime. */
    size_t references; /* Owner plus optional bound flight. */
    StyxMux *mux; /* Borrowed until unbind, which precedes engine retirement. */
    int requested; /* Sticky cancellation request. */
    int used; /* One admitted operation only; late control cannot target a later operation. */
};


/* EN/RU: See client.h; no connection or managed state is captured.
 * См. client.h; без connection или managed state. */
StyxCancel *styxcancelcreate(void)
{
    StyxCancel *cancel = calloc(1, sizeof(*cancel));
    if (cancel && pthread_mutex_init(&cancel->lock, NULL))
    {
        free(cancel);
        cancel = NULL;
    }
    if (cancel)
    {
        cancel->references = 1;
    }
    return cancel;
}


/* EN/RU: See client.h; native wake is protected against concurrent unbind/free.
 * См. client.h; native wake защищён от concurrent unbind/free. */
void styxcancelrequest(StyxCancel *cancel)
{
    if (cancel)
    {
        pthread_mutex_lock(&cancel->lock);
        cancel->requested = 1;
        if (cancel->mux)
        {
            styxmuxwake(cancel->mux);
        }
        pthread_mutex_unlock(&cancel->lock);
    }
}


/* EN/RU: See client.h; consuming caller ref does not cancel admitted work.
 * См. client.h; поглощение caller ref не отменяет admitted work. */
void styxcancelfree(StyxCancel *cancel)
{
    if (!cancel)
    {
        return;
    }
    pthread_mutex_lock(&cancel->lock);
    int last = --cancel->references == 0;
    pthread_mutex_unlock(&cancel->lock);
    if (last)
    {
        pthread_mutex_destroy(&cancel->lock);
        free(cancel);
    }
}


/* EN/RU: See mux.h; retain only an unused token, including already-requested ones.
 * См. mux.h; удержать только unbound token, включая already-requested. */
int styxcancelbind(StyxCancel *cancel, StyxMux *mux)
{
    if (!cancel)
    {
        return 1;
    }
    pthread_mutex_lock(&cancel->lock);
    int valid = !cancel->used && cancel->references != (size_t)-1;
    if (valid)
    {
        cancel->references++;
        cancel->used = 1;
        cancel->mux = mux;
    }
    pthread_mutex_unlock(&cancel->lock);
    return valid;
}


/* EN/RU: See mux.h; native owner checks only its event-driven transaction state.
 * См. mux.h; native owner проверяет event-driven transaction state. */
int styxcancelled(StyxCancel *cancel)
{
    if (!cancel)
    {
        return 0;
    }
    pthread_mutex_lock(&cancel->lock);
    int requested = cancel->requested;
    pthread_mutex_unlock(&cancel->lock);
    return requested;
}


/* EN/RU: See mux.h; clear borrowed engine before dropping the flight's token ref.
 * См. mux.h; очистить borrowed engine до release token ref. */
void styxcancelunbind(StyxCancel *cancel)
{
    if (cancel)
    {
        pthread_mutex_lock(&cancel->lock);
        cancel->mux = NULL;
        pthread_mutex_unlock(&cancel->lock);
        styxcancelfree(cancel);
    }
}
