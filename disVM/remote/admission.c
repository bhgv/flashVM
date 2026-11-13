/* Copyright (c) 2026 bhgv
 * EN: Atomic bounded admission, exact deduplication and admitted-root pin release.
 * RU: Атомарный bounded admission, точная дедупликация и снятие pins принятых roots.
 */
#include "session_internal.h"
#include <string.h>


/* EN: Compare significant handle fields only, never C struct padding.
 * RU: Сравнить значимые поля handle, не C struct padding. Inputs borrowed. */
static int same_handle(DvmRemoteHandle left, DvmRemoteHandle right)
{
    return left.session == right.session && left.slot == right.slot && left.generation == right.generation;
}


/* EN: Compare operation, ordered aliases and snapshot bytes without dereference.
 * RU: Сравнить operation, порядок aliases и snapshot bytes без dereference. */
static int same_request(const DvmRemoteRequest *left, const DvmRemoteRequest *right)
{
    size_t index;
    if (!same_handle(left->target, right->target) || left->kind != right->kind || left->operation != right->operation ||
        left->ref_count != right->ref_count || left->payload_size != right->payload_size)
    {
        return 0;
    }
    for (index = 0; index < left->ref_count; index++)
    {
        if (!same_handle(left->refs[index], right->refs[index]))
        {
            return 0;
        }
    }
    return memcmp(left->payload, right->payload, left->payload_size) == 0;
}


/* EN: Validate all roots and overflow before mutation; repeated refs need one pin
 * per occurrence. Output is borrowed under caller's session lock, never published.
 * RU: Проверить roots/overflow до изменений; repeated refs требуют pin на occurrence.
 * Output заимствован под lock caller и не публикуется наружу. */
static DvmRemoteStatus
resolve_roots(DvmRemoteSession *session, const DvmRemoteRequest *request, DvmRemoteObject **roots)
{
    size_t index;
    roots[0] = dvm_remote_object_locked(session, request->target);
    if (roots[0] == NULL || roots[0]->references == 0)
    {
        return DVM_REMOTE_STALE;
    }
    if (roots[0]->root.kind != request->kind)
    {
        return DVM_REMOTE_INVALID;
    }
    for (index = 0; index < request->ref_count; index++)
    {
        roots[index + 1] = dvm_remote_object_locked(session, request->refs[index]);
        if (roots[index + 1] == NULL || roots[index + 1]->references == 0)
        {
            return DVM_REMOTE_STALE;
        }
        if (roots[index + 1]->root.kind != DVM_REMOTE_VARIABLE)
        {
            return DVM_REMOTE_INVALID;
        }
    }
    for (index = 0; index <= request->ref_count; index++)
    {
        size_t other;
        size_t occurrences = 0;
        for (other = 0; other <= request->ref_count; other++)
        {
            if (roots[other] == roots[index])
            {
                occurrences++;
            }
        }
        if (roots[index]->pins > SIZE_MAX - occurrences)
        {
            return DVM_REMOTE_CAPACITY;
        }
    }
    return DVM_REMOTE_OK;
}


/* EN: Entire validation/reservation/pin transaction uses this session's mutex.
 * RU: Вся validation/reservation/pin transaction использует mutex только этой сессии. */
DvmRemoteStatus dvm_remote_admit(DvmRemoteSession *session, const DvmRemoteRequest *request, int *done)
{
    DvmRemoteStatus result;
    DvmRemoteAdmission *entry;
    DvmRemoteObject *roots[DVM_REMOTE_REFS + 1];
    size_t index;
    if (session == NULL || request == NULL || done == NULL || request->id == 0 ||
        request->ref_count > DVM_REMOTE_REFS || request->payload_size > DVM_REMOTE_PAYLOAD ||
        (request->kind != DVM_REMOTE_VARIABLE && request->kind != DVM_REMOTE_FUNCTION))
    {
        return DVM_REMOTE_INVALID;
    }
    pthread_mutex_lock(&session->mutex);
    entry = &session->requests[request->id % session->request_count];
    if (session->closed)
    {
        result = DVM_REMOTE_CLOSED;
    }
    else if (entry->request.id == request->id)
    {
        result = same_request(request, &entry->request) ? DVM_REMOTE_DUPLICATE : DVM_REMOTE_CONFLICT;
        if (result == DVM_REMOTE_DUPLICATE)
        {
            *done = !entry->active;
        }
    }
    else if (request->id <= session->highest && session->highest - request->id >= session->request_count)
    {
        result = DVM_REMOTE_STALE;
    }
    else if (entry->active)
    {
        result = DVM_REMOTE_CAPACITY;
    }
    else
    {
        result = resolve_roots(session, request, roots);
        if (result == DVM_REMOTE_OK)
        {
            for (index = 0; index <= request->ref_count; index++)
            {
                roots[index]->pins++;
            }
            entry->request = *request;
            entry->active = 1;
            if (request->id > session->highest)
            {
                session->highest = request->id;
            }
            *done = 0;
        }
    }
    pthread_mutex_unlock(&session->mutex);
    return result;
}


/* EN: Exact active ID resolves pins even after close/expiry; no root callback.
 * RU: Точный active ID снимает pins даже после close/expiry; root callback нет. */
DvmRemoteStatus dvm_remote_finish(DvmRemoteSession *session, uint64_t id)
{
    DvmRemoteAdmission *entry;
    size_t index;
    if (session == NULL || id == 0)
    {
        return DVM_REMOTE_INVALID;
    }
    pthread_mutex_lock(&session->mutex);
    entry = &session->requests[id % session->request_count];
    if (entry->request.id != id || !entry->active)
    {
        pthread_mutex_unlock(&session->mutex);
        return DVM_REMOTE_STALE;
    }
    dvm_remote_object_locked(session, entry->request.target)->pins--;
    for (index = 0; index < entry->request.ref_count; index++)
    {
        dvm_remote_object_locked(session, entry->request.refs[index])->pins--;
    }
    entry->active = 0;
    pthread_mutex_unlock(&session->mutex);
    return DVM_REMOTE_OK;
}
