/* Copyright (c) 2026 bhgv
 * EN: Private refcounted subscription, bounded projected queue and retained encoded head.
 * RU: Private refcounted subscription, bounded projected queue и encoded head.
 */
#ifndef STYX_EVENTS_INTERNAL_H
#define STYX_EVENTS_INTERNAL_H
#define _POSIX_C_SOURCE 200809L
#include "../events_provider.h"
#include <pthread.h>
#define EVENT_QUEUE 32U /* Tail queue; one additional retained head may exist. */


/* EN/RU: No provider/guest references; native tap independently pins this allocation.
 * Без provider/guest references; native tap независимо удерживает allocation. */
struct StyxEvents
{
    pthread_mutex_t lock; /* Protect queue, lifetime, source cursor, finish and readiness. */
    pthread_mutex_t reader; /* Serialize formatter/cache/consumption across aliases. */
    size_t references; /* Owner references plus one attached/in-flight tap. */
    size_t owners; /* Public capabilities/provider references, no cycle with source. */
    DVM_Changes *source; /* Retained independent journal until final cleanup. */
    DVM_NoticeTap *tap; /* Owned subscription, detached before final owner disappears. */
    StyxEventPolicy policy; /* Adopted native host policy. */
    StyxEvent queue[EVENT_QUEUE]; /* Bounded copied projected records. */
    StyxEvent head; /* Retained record across short buffers or encode failure. */
    unsigned first; /* Tail ring first index. */
    unsigned queued; /* Tail ring count. */
    int have; /* Shared head exists, protected by queue lock. */
    int closed; /* Source finished; admitted callbacks no longer append. */
    unsigned long long session; /* Highest admitted source epoch. */
    unsigned long long generation; /* Highest admitted source generation. */
    unsigned long long identity; /* Process-local resource identity, never a pointer. */
    unsigned char *encoded; /* Fixed-capacity native formatter storage. */
    size_t length; /* Cached head length, protected by reader mutex. */
    int wake[2]; /* Owned nonblocking CLOEXEC readiness pipe. */
};


/* EN/RU: Under queue lock, signal native readiness without provider/transport I/O.
 * Под queue lock сигнализировать native readiness без provider/transport I/O. */
void styxeventsignal(StyxEvents *events);


/* EN/RU: Under queue lock, drain/rearm level readiness from current queue/finish state.
 * Под queue lock drain/rearm readiness по queue/finish state. */
void styxeventlevel(StyxEvents *events);


/* EN/RU: Consume private ref after source/caller retirement; last release frees native storage.
 * Поглотить private ref после retirement; последняя освобождает native storage. */
void styxeventdrop(StyxEvents *events);
#endif
