/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of life.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* Copyright (c) 2026 bhgv
 * EN: Native terminal and health observations with detached callback pins and nonwaiting unsubscribe.
 * RU: Native terminal/health observations с detached callback pins и nonwaiting unsubscribe.
 */
#include "life.h"
#include <pthread.h>
#include <stdlib.h>

/* EN/RU: Caller and attached/in-flight reference independently retain callback state.
 * Caller и attached/in-flight reference независимо удерживают callback state. */
struct StyxWatch
{
    StyxLife *life;          /* Retained source lock lifetime. */
    struct StyxWatch *next;  /* Borrowed attached chain, immutable after source detach. */
    size_t references;       /* Caller plus attached/in-flight callback. */
    int active;              /* A new callback may be admitted. */
    StyxLinkReceive observe; /* Optional persistent snapshot recipient. */
    StyxTerminal notify;     /* Nonwaiting native recipient. */
    void (*release)(void *); /* Consumes state after last reference, outside source lock. */
    void *state;             /* Owned native recipient state. */
};

/* EN/RU: Two native owners and at most256 watcher nodes bound storage and references.
 * Два native owners и максимум256 watcher nodes ограничивают storage/references. */
struct StyxLife
{
    pthread_mutex_t lock; /* Protect source state, node admission and references. */
    size_t references;    /* Native owners, nodes and active emission. */
    unsigned watchers;    /* All live caller/detached watcher nodes, bounded256. */
    StyxLinkEvent event;  /* Latest persistent observation. */
    StyxWatch *observers; /* Attached persistent subscriptions. */
    StyxWatch *head;      /* Attached chain owns one reference to each member. */
    int reason;           /* First immutable terminal reason,0 live. */
    int quiet;            /* Final owner retired; no new subscription/callback admission. */
};

/* EN/RU: See life.h; allocate/publish no incomplete native lock state.
 * См. life.h; не публиковать incomplete native lock state. */


/* [standalone-contract:styxlifecreate]
 * EN: styxlifecreate: native client life operations and lifetime.
 * RU: styxlifecreate: нативные client-операции life и время жизни.
 * Parameters: none. Параметры: нет.
 * Return: StyxLife *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: StyxLife *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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
        life->event.generation = 1;
        life->event.state = STYX_LINK_READY;
    }
    return life;
}

/* EN/RU: See life.h; caller already pins a live source through the retain.
 * См. life.h; caller уже удерживает live source на время retain. */


/* [standalone-contract:styxliferetain]
 * EN: styxliferetain: native client life operations and lifetime.
 * RU: styxliferetain: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:styxlifedrop]
 * EN: styxlifedrop: native client life operations and lifetime.
 * RU: styxlifedrop: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:drop]
 * EN: drop: native client life operations and lifetime.
 * RU: drop: нативные client-операции life и время жизни.
 * watch (StyxWatch *watch): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:styxclientunwatch]
 * EN: styxclientunwatch: native client life operations and lifetime.
 * RU: styxclientunwatch: нативные client-операции life и время жизни.
 * watch (StyxWatch *watch): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclientunwatch(StyxWatch *watch)
{
    if (!watch)
    {
        return;
    }
    StyxLife *life = watch->life;
    int attached = 0;
    pthread_mutex_lock(&life->lock);
    for (StyxWatch **link = watch->observe ? &life->observers : &life->head; *link; link = &(*link)->next)
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


/* [standalone-contract:styxlifeemit]
 * EN: styxlifeemit: native client life operations and lifetime.
 * RU: styxlifeemit: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * reason (int reason): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxlifeemit(StyxLife *life, int reason)
{
    if (!life || (reason != STYX_LINK_FAILED && reason != STYX_LINK_CLOSED && reason != STYX_LINK_TIMEOUT))
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
    styxlifechange(life, reason);
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


/* [standalone-contract:styxlifequiet]
 * EN: styxlifequiet: native client life operations and lifetime.
 * RU: styxlifequiet: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxlifequiet(StyxLife *life)
{
    if (!life)
    {
        return;
    }
    pthread_mutex_lock(&life->lock);
    life->quiet = 1;
    StyxWatch *watch = life->head;
    if (!watch)
    {
        watch = life->observers;
    }
    else
    {
        StyxWatch *tail = watch;
        while (tail->next)
        {
            tail = tail->next;
        }
        tail->next = life->observers;
    }
    life->head = NULL;
    life->observers = NULL;
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


/* [standalone-contract:styxlifewatch]
 * EN: styxlifewatch: native client life operations and lifetime.
 * RU: styxlifewatch: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * notify (StyxTerminal notify): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * void (*release)(void *) (void (*release)(void *)): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxWatch *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxWatch *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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

/* EN/RU: See life.h; callback snapshot pins every selected node before leaving lock.
 * См. life.h; snapshot удерживает каждый selected node до выхода из lock. */


/* [standalone-contract:styxlifechange]
 * EN: styxlifechange: native client life operations and lifetime.
 * RU: styxlifechange: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * state (int state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxlifechange(StyxLife *life, int state)
{
    StyxWatch *selected[256];
    unsigned count = 0;
    pthread_mutex_lock(&life->lock);
    if (life->quiet || life->event.state == state || (life->reason && life->reason != state) ||
        life->event.generation == ~0ULL)
    {
        pthread_mutex_unlock(&life->lock);
        return;
    }
    life->event.state = state;
    life->event.generation++;
    StyxLinkEvent event = life->event;
    life->references++;
    for (StyxWatch *node = life->observers; node; node = node->next)
    {
        node->references++;
        selected[count++] = node;
    }
    pthread_mutex_unlock(&life->lock);
    for (unsigned i = 0; i < count; i++)
    {
        StyxWatch *watch = selected[i];
        pthread_mutex_lock(&life->lock);
        int active = watch->active && !life->quiet;
        pthread_mutex_unlock(&life->lock);
        if (active)
        {
            watch->observe(watch->state, &event);
        }
        drop(watch);
    }
    styxlifedrop(life);
}

/* EN/RU: See life.h; caller already pins source, replay owns independent callback reference.
 * См. life.h; caller удерживает source, replay имеет независимую callback reference. */
