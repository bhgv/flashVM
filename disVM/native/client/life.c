/* Copyright (c) 2026 bhgv
 * EN: Sticky native terminal events with detached callback pins and nonwaiting unsubscribe.
 * RU: Sticky native terminal events с detached callback pins и nonwaiting unsubscribe.
 */
#include "life.h"
#include <pthread.h>
#include <stdlib.h>


/* EN/RU: Caller and attached/in-flight reference independently retain callback state.
 * Caller и attached/in-flight reference независимо удерживают callback state. */
struct StyxWatch
{
    StyxLife *life; /* Retained source lock lifetime. */
    struct StyxWatch *next; /* Borrowed attached chain, immutable after source detach. */
    size_t references; /* Caller plus attached/in-flight callback. */
    int active; /* A new callback may be admitted. */
    StyxTerminal notify; /* Nonwaiting native recipient. */
    void (*release)(void *); /* Consumes state after last reference, outside source lock. */
    void *state; /* Owned native recipient state. */
};


/* EN/RU: Two native owners and at most256 watcher nodes bound storage and references.
 * Два native owners и максимум256 watcher nodes ограничивают storage/references. */
struct StyxLife
{
    pthread_mutex_t lock; /* Protect source state, node admission and references. */
    size_t references; /* Native owners, nodes and active emission. */
    unsigned watchers; /* All live caller/detached watcher nodes, bounded256. */
    StyxWatch *head; /* Attached chain owns one reference to each member. */
    int reason; /* First immutable terminal reason,0 live. */
    int quiet; /* Final owner retired; no new subscription/callback admission. */
};


/* EN/RU: See life.h; allocate/publish no incomplete native lock state.
 * См. life.h; не публиковать incomplete native lock state. */
StyxLife *styxlifecreate(void)
{
    StyxLife *life = calloc(1, sizeof(*life));
    if (life && pthread_mutex_init(&life->lock, NULL))
    {
        free(life);
        life = NULL;
    }
    if (life)
    {
        life->references = 1;
    }
    return life;
}


/* EN/RU: See life.h; caller already pins a live source through the retain.
 * См. life.h; caller уже удерживает live source на время retain. */
int styxliferetain(StyxLife *life)
{
    if (!life)
    {
        return 0;
    }
    pthread_mutex_lock(&life->lock);
    int valid = life->references != (size_t)-1;
    if (valid)
    {
        life->references++;
    }
    pthread_mutex_unlock(&life->lock);
    return valid;
}


/* EN/RU: See life.h; only native source storage is retired here.
 * См. life.h; освобождается только native source storage. */
void styxlifedrop(StyxLife *life)
{
    if (life)
    {
        pthread_mutex_lock(&life->lock);
        int last = --life->references == 0;
        pthread_mutex_unlock(&life->lock);
        if (last)
        {
            pthread_mutex_destroy(&life->lock);
            free(life);
        }
    }
}


/* EN/RU: Drop one watcher reference, preserving source through final context release.
 * Поглотить watcher ref, сохранив source до final context release. */
static void drop(StyxWatch *watch)
{
    StyxLife *life = watch->life;
    pthread_mutex_lock(&life->lock);
    int last = --watch->references == 0;
    if (last)
    {
        life->watchers--;
    }
    pthread_mutex_unlock(&life->lock);
    if (last)
    {
        if (watch->release)
        {
            watch->release(watch->state);
        }
        free(watch);
        styxlifedrop(life);
    }
}


/* EN/RU: See client.h; unlink before releasing caller/attached references, no callback wait.
 * См. client.h; unlink до release caller/attached refs, без callback wait. */
void styxclientunwatch(StyxWatch *watch)
{
    if (!watch)
    {
        return;
    }
    StyxLife *life = watch->life;
    int attached = 0;
    pthread_mutex_lock(&life->lock);
    for (StyxWatch **link = &life->head; *link; link = &(*link)->next)
    {
        if (*link == watch)
        {
            *link = watch->next;
            attached = 1;
            break;
        }
    }
    watch->active = 0;
    pthread_mutex_unlock(&life->lock);
    if (attached)
    {
        drop(watch);
    }
    drop(watch);
}


/* EN/RU: See life.h; first event detaches the chain so concurrent detach cannot free its links.
 * См. life.h; первый event отделяет chain, concurrent detach не освобождает links. */
void styxlifeemit(StyxLife *life, int reason)
{
    if (!life || (reason != STYX_LINK_FAILED && reason != STYX_LINK_CLOSED))
    {
        return;
    }
    pthread_mutex_lock(&life->lock);
    if (life->quiet || life->reason)
    {
        pthread_mutex_unlock(&life->lock);
        return;
    }
    life->reason = reason;
    life->references++;
    StyxWatch *watch = life->head;
    life->head = NULL;
    pthread_mutex_unlock(&life->lock);
    while (watch)
    {
        StyxWatch *next = watch->next;
        pthread_mutex_lock(&life->lock);
        int active = watch->active && !life->quiet;
        pthread_mutex_unlock(&life->lock);
        if (active)
        {
            watch->notify(watch->state, reason);
        }
        drop(watch);
        watch = next;
    }
    styxlifedrop(life);
}


/* EN/RU: See life.h; in-flight callback refs outlive quiet; pending callbacks are suppressed.
 * См. life.h; in-flight callback refs переживают quiet, pending callbacks подавляются. */
void styxlifequiet(StyxLife *life)
{
    if (!life)
    {
        return;
    }
    pthread_mutex_lock(&life->lock);
    life->quiet = 1;
    StyxWatch *watch = life->head;
    life->head = NULL;
    for (StyxWatch *node = watch; node; node = node->next)
    {
        node->active = 0;
    }
    pthread_mutex_unlock(&life->lock);
    while (watch)
    {
        StyxWatch *next = watch->next;
        drop(watch);
        watch = next;
    }
}


/* EN/RU: See life.h; terminal replay owns a temporary callback reference before publication.
 * См. life.h; terminal replay удерживает temporary callback ref до publication. */
StyxWatch *styxlifewatch(StyxLife *life, StyxTerminal notify, void *state, void (*release)(void *))
{
    if (!life || !notify)
    {
        return NULL;
    }
    StyxWatch *watch = calloc(1, sizeof(*watch));
    if (!watch)
    {
        return NULL;
    }
    pthread_mutex_lock(&life->lock);
    if (life->quiet || life->watchers == 256 || life->references == (size_t)-1)
    {
        pthread_mutex_unlock(&life->lock);
        free(watch);
        return NULL;
    }
    watch->life = life;
    watch->notify = notify;
    watch->release = release;
    watch->state = state;
    watch->references = 2;
    watch->active = 1;
    life->references++;
    life->watchers++;
    int reason = life->reason;
    if (!reason)
    {
        watch->next = life->head;
        life->head = watch;
    }
    pthread_mutex_unlock(&life->lock);
    if (reason)
    {
        pthread_mutex_lock(&life->lock);
        int active = watch->active && !life->quiet;
        pthread_mutex_unlock(&life->lock);
        if (active)
        {
            notify(state, reason);
        }
        drop(watch);
    }
    return watch;
}
