/* Copyright (c) 2026 bhgv
 * EN: Native proxy root ownership, non-revivable sessions and monotonic leases.
 * RU: Native-владение корнями proxy, невозобновляемые сессии и monotonic leases.
 */
#include "session_internal.h"
#include <stdlib.h>

static uint64_t next_identity = 1; /* Atomic allocation; zero permanently means exhausted. */


/* EN: Allocate a never-repeated process identity with relaxed atomic CAS; zero
 * means exhaustion. No lock, allocation or cross-owner heap access is needed.
 * RU: Выдать неповторяющуюся process identity через relaxed CAS; ноль — исчерпание.
 * Нет lock, allocation или обращения к чужому heap. */
static uint64_t allocate_identity(void)
{
    uint64_t value = __atomic_load_n(&next_identity, __ATOMIC_RELAXED);
    while (value != 0)
    {
        uint64_t next = value == UINT64_MAX ? 0 : value + 1;
        if (__atomic_compare_exchange_n(&next_identity, &value, next, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            return value;
        }
    }
    return 0;
}


/* EN: Allocate fixed tables and one mutex; failure transfers no owner roots.
 * RU: Выделить фиксированные таблицы и mutex; при отказе owner roots не передаются. */
DvmRemoteSession *dvm_remote_session_new(size_t objects, size_t requests, uint64_t expiry)
{
    DvmRemoteSession *session;
    if (objects == 0 || requests == 0 || expiry == 0 || objects > SIZE_MAX / sizeof(DvmRemoteObject) ||
        requests > SIZE_MAX / sizeof(DvmRemoteAdmission))
    {
        return NULL;
    }
    session = calloc(1, sizeof(*session));
    if (session == NULL)
    {
        return NULL;
    }
    session->objects = calloc(objects, sizeof(*session->objects));
    session->requests = calloc(requests, sizeof(*session->requests));
    session->identity = allocate_identity();
    if (session->objects == NULL || session->requests == NULL || session->identity == 0)
    {
        free(session->objects);
        free(session->requests);
        free(session);
        return NULL;
    }
    if (pthread_mutex_init(&session->mutex, NULL) != 0)
    {
        free(session->objects);
        free(session->requests);
        free(session);
        return NULL;
    }
    session->object_count = objects;
    session->request_count = requests;
    session->expiry = expiry;
    return session;
}


/* EN: With callers joined, refuse destruction until roots and operations end.
 * RU: После join callers отказать в уничтожении, пока остаются roots/operations. */
DvmRemoteStatus dvm_remote_session_destroy(DvmRemoteSession *session)
{
    size_t index;
    if (session == NULL)
    {
        return DVM_REMOTE_INVALID;
    }
    pthread_mutex_lock(&session->mutex);
    for (index = 0; index < session->object_count; index++)
    {
        if (session->objects[index].root.token != NULL)
        {
            pthread_mutex_unlock(&session->mutex);
            return DVM_REMOTE_BUSY;
        }
    }
    for (index = 0; index < session->request_count; index++)
    {
        if (session->requests[index].active)
        {
            pthread_mutex_unlock(&session->mutex);
            return DVM_REMOTE_BUSY;
        }
    }
    pthread_mutex_unlock(&session->mutex);
    pthread_mutex_destroy(&session->mutex);
    free(session->requests);
    free(session->objects);
    free(session);
    return DVM_REMOTE_OK;
}


/* EN: Caller holds mutex. Revoke future admissions and peer references only.
 * RU: Caller держит mutex. Отозвать новые admissions и только peer references. */
void dvm_remote_close_locked(DvmRemoteSession *session)
{
    size_t index;
    session->closed = 1;
    for (index = 0; index < session->object_count; index++)
    {
        session->objects[index].references = 0;
    }
}


/* EN: Close is idempotent and leaves all accepted operation pins intact.
 * RU: Close идемпотентен и сохраняет pins всех принятых операций. */
void dvm_remote_session_close(DvmRemoteSession *session)
{
    if (session != NULL)
    {
        pthread_mutex_lock(&session->mutex);
        dvm_remote_close_locked(session);
        pthread_mutex_unlock(&session->mutex);
    }
}


/* EN: Reject backward time and renewal after expiration; no hidden wall clock.
 * RU: Отказать времени назад и renewal после expiry; скрытых wall-clock reads нет. */
DvmRemoteStatus dvm_remote_session_tick(DvmRemoteSession *session, uint64_t now, uint64_t expiry)
{
    DvmRemoteStatus result = DVM_REMOTE_OK;
    if (session == NULL)
    {
        return DVM_REMOTE_INVALID;
    }
    pthread_mutex_lock(&session->mutex);
    if (now < session->now)
    {
        result = DVM_REMOTE_INVALID;
    }
    else
    {
        session->now = now;
        if (now >= session->expiry)
        {
            dvm_remote_close_locked(session);
        }
        if (session->closed)
        {
            result = DVM_REMOTE_CLOSED;
        }
        else if (expiry != 0 && expiry <= now)
        {
            result = DVM_REMOTE_INVALID;
        }
        else if (expiry != 0)
        {
            session->expiry = expiry;
        }
    }
    pthread_mutex_unlock(&session->mutex);
    return result;
}


/* EN: Resolve live root identity under lock; peer ref count checked by callers.
 * RU: Под lock найти живой root по identity; peer ref count проверяют callers. */
DvmRemoteObject *dvm_remote_object_locked(DvmRemoteSession *session, DvmRemoteHandle handle)
{
    DvmRemoteObject *object;
    if (handle.session != session->identity || handle.slot >= session->object_count)
    {
        return NULL;
    }
    object = &session->objects[handle.slot];
    if (object->root.token == NULL || object->generation != handle.generation)
    {
        return NULL;
    }
    return object;
}


/* EN: Publish transfers a root only after reserving a reusable generation slot.
 * RU: Publish передаёт root только после резервирования reusable generation slot. */
DvmRemoteStatus dvm_remote_publish(DvmRemoteSession *session, DvmRemoteKind kind, void *token, DvmRemoteHandle *output)
{
    size_t index;
    DvmRemoteStatus result = DVM_REMOTE_CAPACITY;
    if (session == NULL || token == NULL || output == NULL ||
        (kind != DVM_REMOTE_VARIABLE && kind != DVM_REMOTE_FUNCTION))
    {
        return DVM_REMOTE_INVALID;
    }
    pthread_mutex_lock(&session->mutex);
    if (session->closed)
    {
        result = DVM_REMOTE_CLOSED;
    }
    else
    {
        for (index = 0; index < session->object_count; index++)
        {
            DvmRemoteObject *object = &session->objects[index];
            if (object->root.token == NULL && object->generation != UINT64_MAX)
            {
                object->generation++;
                object->root.token = token;
                object->root.kind = kind;
                object->references = 1;
                output->session = session->identity;
                output->slot = index;
                output->generation = object->generation;
                result = DVM_REMOTE_OK;
                break;
            }
        }
    }
    pthread_mutex_unlock(&session->mutex);
    return result;
}


/* EN: Adjust a trusted peer reference under lock; never resurrect a zero count.
 * RU: Под lock изменить trusted peer ref; нулевой count не возобновляется. */
static DvmRemoteStatus change_reference(DvmRemoteSession *session, DvmRemoteHandle handle, int add)
{
    DvmRemoteStatus result = DVM_REMOTE_OK;
    DvmRemoteObject *object;
    if (session == NULL)
    {
        return DVM_REMOTE_INVALID;
    }
    pthread_mutex_lock(&session->mutex);
    object = dvm_remote_object_locked(session, handle);
    if (session->closed)
    {
        result = DVM_REMOTE_CLOSED;
    }
    else if (object == NULL || object->references == 0)
    {
        result = DVM_REMOTE_STALE;
    }
    else if (add && object->references == SIZE_MAX)
    {
        result = DVM_REMOTE_CAPACITY;
    }
    else if (add)
    {
        object->references++;
    }
    else
    {
        object->references--;
    }
    pthread_mutex_unlock(&session->mutex);
    return result;
}


/* EN/RU: Add one validated peer owner; session/handle are borrowed, no callback.
 * Добавить peer owner; session/handle заимствованы, callback не выполняется. */
DvmRemoteStatus dvm_remote_retain(DvmRemoteSession *session, DvmRemoteHandle handle)
{
    return change_reference(session, handle, 1);
}


/* EN/RU: Remove one validated peer owner, independently of admission pins.
 * Снять peer owner независимо от admission pins. */
DvmRemoteStatus dvm_remote_release(DvmRemoteSession *session, DvmRemoteHandle handle)
{
    return change_reference(session, handle, 0);
}


/* EN: Transfer one unreferenced, unpinned root back to its owner; no GC inside.
 * RU: Вернуть owner один root без refs/pins; внутри GC не вызывается. */
DvmRemoteStatus dvm_remote_collect(DvmRemoteSession *session, DvmRemoteRoot *output)
{
    size_t index;
    DvmRemoteStatus result = DVM_REMOTE_STALE;
    if (session == NULL || output == NULL)
    {
        return DVM_REMOTE_INVALID;
    }
    pthread_mutex_lock(&session->mutex);
    for (index = 0; index < session->object_count; index++)
    {
        DvmRemoteObject *object = &session->objects[index];
        if (object->root.token != NULL && object->references == 0 && object->pins == 0)
        {
            *output = object->root;
            object->root.token = NULL;
            result = DVM_REMOTE_OK;
            break;
        }
    }
    pthread_mutex_unlock(&session->mutex);
    return result;
}
