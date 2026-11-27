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
    size_t references; /* Owner plus optional flight or logical-operation scope. */
    StyxMux *mux; /* Borrowed until unbind, which precedes engine retirement. */
    int wait_admission; /* Immutable opt-in to wait for the initial mux tag. */
    int requested; /* Sticky cancellation request. */
    StyxCancel *child; /* Owned current fragment token for one logical walk scope. */
    int used; /* One admitted operation only; late control cannot target a later operation. */
    StyxClientStop stop; /* Immutable optional hook used only on the collecting caller. */
    void *state;         /* Borrowed hook data through synchronous collection. */
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


/* EN/RU: See client.h. / См. client.h. */
StyxCancel *styxcancelcreatewait(void)
{
    StyxCancel *cancel = styxcancelcreate();
    if (cancel)
    {
        cancel->wait_admission = 1;
    }
    return cancel;
}


/* EN/RU: See mux.h. / См. mux.h. */
StyxCancel *styxcancelcreatepoll(StyxClientStop stop, void *state)
{
    StyxCancel *cancel = styxcancelcreate();
    if (cancel)
    {
        cancel->stop = stop;
        cancel->state = state;
    }
    return cancel;
}


/* EN/RU: See mux.h. / См. mux.h. */
int styxcancelpolling(StyxCancel *cancel)
{
    return cancel && cancel->stop && !styxcancelled(cancel);
}


/* EN/RU: See mux.h. / См. mux.h. */
void styxcancelpoll(StyxCancel *cancel)
{
    if (styxcancelpolling(cancel) && cancel->stop(cancel->state))
    {
        styxcancelrequest(cancel);
    }
}


/* EN/RU: See mux.h. / См. mux.h. */
int styxcancelwaits(const StyxCancel *cancel)
{
    return cancel && cancel->wait_admission;
}


/* EN/RU: See client.h; native wake is protected against concurrent unbind/free.
 * См. client.h; native wake защищён от concurrent unbind/free. */
void styxcancelrequest(StyxCancel *cancel)
{
    if (cancel)
    {
        pthread_mutex_lock(&cancel->lock);
        cancel->requested = 1;
        if (cancel->child)
        {
            styxcancelrequest(cancel->child);
        }
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


/* EN/RU: See mux.h; reserve the outer token once for a whole logical operation.
 * См. mux.h; зарезервировать outer token один раз на всю logical operation. */
int styxcancelbegin(StyxCancel *cancel)
{
    if (!cancel)
    {
        return 1;
    }
    pthread_mutex_lock(&cancel->lock);
    int valid = !cancel->used && cancel->references != (size_t)-1;
    if (valid)
    {
        cancel->used = 1;
        cancel->references++;
    }
    pthread_mutex_unlock(&cancel->lock);
    return valid;
}


/* EN/RU: See mux.h; caller completed the previous flight before replacing its token.
 * См. mux.h; caller завершил previous flight до замены его token. */
StyxCancel *styxcancelstep(StyxCancel *cancel)
{
    StyxCancel *child = styxcancelwaits(cancel) ? styxcancelcreatewait() : styxcancelcreate();
    if (!child)
    {
        return NULL;
    }
    pthread_mutex_lock(&cancel->lock);
    StyxCancel *previous = cancel->child;
    cancel->child = child;
    if (cancel->requested)
    {
        styxcancelrequest(child);
    }
    pthread_mutex_unlock(&cancel->lock);
    styxcancelfree(previous);
    return child;
}


/* EN/RU: See mux.h; detach the final native fragment before consuming the scope pin.
 * См. mux.h; отсоединить final native fragment до поглощения scope pin. */
void styxcancelend(StyxCancel *cancel)
{
    if (!cancel)
    {
        return;
    }
    pthread_mutex_lock(&cancel->lock);
    StyxCancel *child = cancel->child;
    cancel->child = NULL;
    pthread_mutex_unlock(&cancel->lock);
    styxcancelfree(child);
    styxcancelfree(cancel);
}
