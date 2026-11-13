/* Copyright (c) 2026 bhgv
 * EN: Session-private native table state; all fields use the session mutex.
 * RU: Приватное состояние native-таблиц; все поля защищены mutex сессии.
 */
#ifndef DVM_REMOTE_SESSION_INTERNAL_H
#define DVM_REMOTE_SESSION_INTERNAL_H
#include "session.h"
#include <pthread.h>


/* EN/RU: One registered owner root and its independent peer/admission lifetime.
 * Зарегистрированный root с отдельными сроками жизни peer/admission. */
typedef struct
{
    DvmRemoteRoot root;  /* Owner registration; cleared only by owner collect. */
    uint64_t generation; /* Exhausted slots are retired permanently. */
    size_t references;   /* Peer ownership, revoked on close. */
    size_t pins;         /* Accepted targets/arguments; never revoked on close. */
} DvmRemoteObject;


/* EN/RU: Replay entry stays until an eligible newer ID replaces it.
 * Replay-запись живёт до допустимой замены более новым ID. */
typedef struct
{
    DvmRemoteRequest request; /* Significant fields compared without C padding. */
    int active;               /* Exactly one executor may complete this entry. */
} DvmRemoteAdmission;


/* EN/RU: Per-session synchronization, never a VM/world lock; no callbacks inside.
 * Lock отдельной сессии, не VM/world lock; callbacks внутри не вызываются. */
struct DvmRemoteSession
{
    pthread_mutex_t mutex;        /* Protect every field and both native tables. */
    uint64_t identity;            /* Process-unique local session identity. */
    uint64_t now;                 /* Last observed monotonic clock. */
    uint64_t expiry;              /* Absolute lease expiration in caller units. */
    uint64_t highest;             /* Highest successfully admitted request ID. */
    int closed;                   /* Irreversible admission/peer revocation. */
    size_t object_count;          /* Allocated object slots. */
    size_t request_count;         /* Allocated admission/replay slots. */
    DvmRemoteObject *objects;     /* Native storage, not managed heap pointers. */
    DvmRemoteAdmission *requests; /* Bounded exact replay records. */
};


/* EN: With session locked, revoke peer ownership; retain admitted pins.
 * RU: Под lock сессии отозвать peer ownership; сохранить admitted pins. */
void dvm_remote_close_locked(DvmRemoteSession *session);


/* EN: With lock held, resolve full identity; NULL for missing/retired root.
 * RU: Под lock проверить полную identity; NULL для отсутствующего/retired root. */
DvmRemoteObject *dvm_remote_object_locked(DvmRemoteSession *session, DvmRemoteHandle handle);
#endif
