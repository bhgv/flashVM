/* Copyright (c) 2026 bhgv
 * EN: Scalar native event-resource reads/readiness; record encoding belongs to embedding host.
 * RU: Scalar native event-resource reads/readiness; кодирование записи задаёт host.
 */
#ifndef STYX_EVENTS_H
#define STYX_EVENTS_H
#include <stddef.h>
typedef struct StyxEvents StyxEvents; /* Owned reference to one shared publication inbox. */
#define STYX_EVENT_RIGHTS 1U /* Effective access policy may have changed. */
#define STYX_EVENT_NAMESPACE 2U /* Resource membership may have changed. */
#define STYX_EVENT_CONTENT 4U /* Resource bytes/attributes may have changed. */
#define STYX_EVENT_CONNECTION 8U /* Availability/connection observations changed. */
#define STYX_EVENT_LOAD 16U /* Provider load observations changed. */
#define STYX_EVENT_NETWORK 32U /* Client-path network observations changed. */
#define STYX_EVENT_ALL 63U /* Current six provider change classes. */
#define STYX_EVENT_RESOURCE 1U /* Resource identity is meaningful. */
#define STYX_EVENT_RECORD 1 /* One entire record copied and consumed. */
#define STYX_EVENT_EMPTY 0 /* Live source has no record; defer, never send fake EOF. */
#define STYX_EVENT_EOF 2 /* Finished source drained completely. */
#define STYX_EVENT_ERROR (-1) /* Encoding/invalid input failure, head preserved. */
#define STYX_EVENT_SMALL (-2) /* Caller capacity too small; encoded head preserved. */


/* EN/RU: Copied provider-local invalidation, no names, authority or native pointers.
 * Копия provider-local invalidation без имён, полномочий и native pointers. */
typedef struct StyxEvent
{
    unsigned long long session; /* Source journal epoch. */
    unsigned long long generation; /* Source report generation, not a permission grant. */
    unsigned kinds; /* Selected changed classes. */
    unsigned flags; /* Optional RESOURCE scope. */
    unsigned long long resource; /* Provider-local identity, only with explicit exposure. */
} StyxEvent;


/* EN/RU: Explicit immutable host policy; callbacks native-only, no guest/provider/network I/O.
 * Allow runs on publishers and readers, including cached-head permission rechecks.
 * Callbacks must not reenter this inbox; encode runs on the serialized read caller.
 * Политика host: allow на publishers/readers; без reentry, encode на serialized reader. */
typedef struct StyxEventPolicy
{
    unsigned kinds; /* Nonzero subset of ALL; must match granted view. */
    int resources; /*0 hides resource identities,1 explicitly exports them. */
    size_t capacity; /* Maximum encoded record1..65536 bytes. */
    int (*allow)(void *, const StyxEvent *); /* Mandatory nonwaiting admission; only1 allows, every other result drops. */
    int (*encode)(void *, const StyxEvent *, void *, size_t, size_t *); /* Mandatory bounded formatter;1 and nonzero length. */
    void *state; /* Adopted only on successful source creation, immutable/thread-safe callbacks. */
    void (*release)(void *); /* Final nonwaiting cleanup after all readers/callbacks retire. */
} StyxEventPolicy;


/* EN/RU: Retain live borrowed inbox;1/0, NULL refuses. No callbacks/I/O.
 * Удержать live inbox;1/0, NULL отказ. Без callbacks/I/O. */
int styxeventsretain(StyxEvents *events);


/* EN/RU: Consume reference; final owner detaches source without waiting for callbacks.
 * Поглотить reference; последний owner отсоединяет source без ожидания callbacks. */
void styxeventsfree(StyxEvents *events);


/* EN/RU: Stop subscription nonwaiting; queued/head records remain drainable then EOF.
 * Остановить subscription без ожидания; queued/head записи читаются до EOF. */
void styxeventsfinish(StyxEvents *events);


/* EN/RU: Borrow nonblocking readiness fd while holding an owner ref; -1 invalid.
 * Register with styxnotifyfd/poll; do not read/close it, event reads maintain readiness.
 * Заимствовать readiness fd под owner ref; не читать/закрывать, read обновляет готовность. */
int styxeventsfd(StyxEvents *events);


/* EN/RU: Copy/consume exactly one complete record; initialized count on all outcomes.
 * Shared head/cache survives short buffers, encode failure and FD close/reopen.
 * Count is required; bytes may be NULL only for size0. No waiting for events/guest.
 * Скопировать/поглотить одну запись; short buffer/error сохраняет head, без ожидания. */
int styxeventsread(StyxEvents *events, void *bytes, size_t size, size_t *count);
#endif
