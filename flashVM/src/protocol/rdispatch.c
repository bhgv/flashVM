/* Bounded copied control requests; only explicit owner pumping invokes registered resource code. */
#include "flashvm/revents.h"
#include <stdlib.h>
#include <string.h>
#define R_FREE 0U    /* Unoccupied request slot. */
#define R_QUEUED 1U  /* Accepted FIFO work, not yet executed. */
#define R_RUNNING 2U /* Callback owns this request until completion. */
#define R_DONE 3U    /* Reply retained until explicit take/acknowledgement. */
#define R_NONE (-1)  /* End marker in the bounded index-linked FIFO. */
/* One registered owner resource; no managed object root is implied by userdata. */
typedef struct Resource
{
    unsigned long generation;  /* Advances on release; zero means permanently retired. */
    int active, running;       /* Registration and execution lifetime guards. */
    char path[FVM_R_PATH + 1]; /* Copied exact namespace spelling. */
    FVM_ROperation operations[FVM_R_OPERATIONS]; /* Immutable required rights. */
    size_t count;                                /* Active operation descriptions. */
    FVM_RHandler handler;                        /* Trusted native owner callback. */
    void *userdata; /* Borrowed until unregister/destroy returns successfully. */
} Resource;
/* One capability association, scoped to dispatcher and generation. */
typedef struct Grant
{
    unsigned long session; /* Zero denotes an unused entry. */
    FVM_RHandle target;    /* Exact resource incarnation. */
    unsigned int rights;   /* Current server-granted operation classes. */
} Grant;
/* Internal request storage is never exposed to transport threads as mutable bytes. */
typedef struct Job
{
    unsigned int state;               /* FREE/QUEUED/RUNNING/DONE. */
    int next;                         /* FIFO index link, meaningful only while queued. */
    FVM_RRequest request;             /* Data pointer refers to this job's owned input bytes. */
    unsigned char input[FVM_R_BYTES]; /* Immutable copied request payload. */
    FVM_RReply reply;                 /* Retained copied response. */
} Job;
/* Per-reader native queue; guarded by the same dispatcher lock as grants. */
typedef struct Subscription
{
    unsigned long generation;             /* Nonzero incarnation, or retired zero. */
    unsigned long session;                /* Zero marks a free slot. */
    FVM_RHandle target;                   /* Exact observed resource generation. */
    size_t head, count;                   /* Independent circular queue extent. */
    unsigned long missed;                 /* Saturating loss since the last successful take. */
    FVM_REvent events[FVM_R_EVENT_QUEUE]; /* Fixed owned copies. */
} Subscription;
/* One allocation has explicit hard bounds and no implicit workers, sockets or VM contexts. */
struct FVM_RDispatch
{
    FVM_RPolicy policy;                  /* Immutable borrowed host synchronization policy. */
    Resource resources[FVM_R_RESOURCES]; /* Native registration table. */
    Grant grants[FVM_R_GRANTS];          /* Explicit authority table. */
    Job jobs[FVM_R_REQUESTS];            /* Accepted work and retained replies share capacity. */
    Subscription subscriptions[FVM_R_WATCHES]; /* Session-local observers. */
    unsigned long sequence;                    /* Shared publication sequence, never wrapped. */
    int head, tail, pumping;                   /* FIFO ends and non-reentrant owner guard. */
};
/* IDs use the same nonzero unsigned 32-bit domain on all native data models. */
static int valid_id(unsigned long id)
{
    return id && id <= FVM_SRECORD_U32;
}
/* Host policy owns its mutex lifetime and the implementation never invokes resource code locked. */
static void lock_dispatch(FVM_RDispatch *d)
{
    if (d->policy.lock)
        d->policy.lock(d->policy.userdata);
}
/* Match every acquisition on all public error paths. */
static void unlock_dispatch(FVM_RDispatch *d)
{
    if (d->policy.unlock)
        d->policy.unlock(d->policy.userdata);
}
/* Native owner check is optional only when the caller accepts the owner-thread precondition. */
static int owner(FVM_RDispatch *d)
{
    return !d->policy.is_owner || d->policy.is_owner(d->policy.userdata);
}
/* Reject aliases, control bytes and parent traversal rather than resolving an ambient OS path. */
static int path_valid(const char *path)
{
    size_t i, start = 1;
    if (!path || path[0] != '/')
        return 0;
    for (i = 1; i <= FVM_R_PATH; ++i)
    {
        unsigned char c = (unsigned char)path[i];
        if (c && (c < 0x20U || c == 0x7fU || c == '\\'))
            return 0;
        if (!c || c == '/')
        {
            size_t length = i - start;
            if (!length || (length == 1 && path[start] == '.') ||
                (length == 2 && path[start] == '.' && path[start + 1] == '.'))
                return 0;
            if (!c)
                return 1;
            start = i + 1;
        }
    }
    return 0;
}
/* Resolve a bounded generation-checked handle while holding the host lock. */
static Resource *resource(FVM_RDispatch *d, FVM_RHandle h)
{
    Resource *r;
    if (!h.id || h.id > FVM_R_RESOURCES || !valid_id(h.generation))
        return 0;
    r = d->resources + (size_t)h.id - 1;
    return r->active && r->generation == h.generation ? r : 0;
}
/* A handle is identity only; authority is an independent server entry. */
static unsigned int rights(FVM_RDispatch *d, FVM_RHandle h, unsigned long session)
{
    size_t i;
    for (i = 0; i < FVM_R_GRANTS; ++i)
        if (d->grants[i].session == session && d->grants[i].target.id == h.id &&
            d->grants[i].target.generation == h.generation)
            return d->grants[i].rights;
    return 0;
}
/* Retire an incarnation before its storage can be reused. Caller holds the lock. */
static void close_subscription(Subscription *s)
{
    unsigned long generation = s->generation;
    memset(s, 0, sizeof(*s));
    s->generation = generation == FVM_SRECORD_U32 ? 0 : generation + 1;
}
/* A rights removal must discard queued observations, even if later regranted. */
static void revoke_subscriptions(FVM_RDispatch *d, FVM_RHandle h, unsigned long session)
{
    size_t i;
    for (i = 0; i < FVM_R_WATCHES; ++i)
    {
        Subscription *s = d->subscriptions + i;
        if (s->session && (!session || s->session == session) && s->target.id == h.id &&
            s->target.generation == h.generation)
            close_subscription(s);
    }
}
/* Validate an observer handle independently of resource handles. Lock is held. */
static Subscription *subscription(FVM_RDispatch *d, FVM_RWatch h)
{
    Subscription *s;
    if (!h.id || h.id > FVM_R_WATCHES || !valid_id(h.generation))
        return 0;
    s = d->subscriptions + (size_t)h.id - 1;
    return s->session && s->generation == h.generation ? s : 0;
}
/* Server operation metadata, not a packet-supplied permission, controls authorization. */
static FVM_RResult authorize(FVM_RDispatch *d, const FVM_RRequest *q, Resource **out)
{
    Resource *r = resource(d, q->target);
    size_t i;
    if (!r)
        return FVM_R_UNKNOWN;
    for (i = 0; i < r->count; ++i)
        if (r->operations[i].id == q->operation)
        {
            if (!(rights(d, q->target, q->session) & r->operations[i].right))
                return FVM_R_DENIED;
            *out = r;
            return FVM_R_OK;
        }
    return FVM_R_UNKNOWN;
}
/* Search both queued/running and retained responses to enforce bounded deduplication. */
static int find_job(FVM_RDispatch *d, unsigned long session, unsigned long id)
{
    size_t i;
    for (i = 0; i < FVM_R_REQUESTS; ++i)
        if (d->jobs[i].state != R_FREE && d->jobs[i].request.session == session &&
            d->jobs[i].request.id == id)
            return (int)i;
    return R_NONE;
}
/* Validate and copy policy before allocating any native dispatcher storage. */
FVM_RResult fvm_rdispatch_create(const FVM_RPolicy *policy, FVM_RDispatch **out)
{
    FVM_RDispatch *d;
    size_t i;
    if (!out || (policy &&
                 (policy->struct_size != sizeof(*policy) || (!!policy->lock != !!policy->unlock))))
        return FVM_R_INVALID;
    if (policy && policy->is_owner && !policy->is_owner(policy->userdata))
        return FVM_R_DENIED;
    d = (FVM_RDispatch *)calloc(1, sizeof(*d));
    if (!d)
        return FVM_R_NOMEM;
    if (policy)
        d->policy = *policy;
    d->head = d->tail = R_NONE;
    for (i = 0; i < FVM_R_RESOURCES; ++i)
        d->resources[i].generation = 1;
    for (i = 0; i < FVM_R_WATCHES; ++i)
        d->subscriptions[i].generation = 1;
    *out = d;
    return FVM_R_OK;
}
/* Registration is owner-only; validation and duplicate checks precede publication. */
FVM_RResult fvm_rdispatch_register(FVM_RDispatch *d, const char *path, const FVM_ROperation *ops,
                                   size_t count, FVM_RHandler handler, void *userdata,
                                   FVM_RHandle *out)
{
    size_t i, j, slot = FVM_R_RESOURCES;
    Resource *r;
    if (!d || !path_valid(path) || !ops || !count || count > FVM_R_OPERATIONS || !handler || !out)
        return FVM_R_INVALID;
    if (!owner(d))
        return FVM_R_DENIED;
    for (i = 0; i < count; ++i)
    {
        if (!valid_id(ops[i].id) || !ops[i].right || (ops[i].right & ~FVM_R_RIGHTS) ||
            (ops[i].right & (ops[i].right - 1)))
            return FVM_R_INVALID;
        for (j = 0; j < i; ++j)
            if (ops[j].id == ops[i].id)
                return FVM_R_INVALID;
    }
    lock_dispatch(d);
    for (i = 0; i < FVM_R_RESOURCES; ++i)
    {
        r = d->resources + i;
        if (r->active && !strcmp(r->path, path))
        {
            unlock_dispatch(d);
            return FVM_R_CONFLICT;
        }
        if (!r->active && r->generation && slot == FVM_R_RESOURCES)
            slot = i;
    }
    if (slot == FVM_R_RESOURCES)
    {
        unlock_dispatch(d);
        return FVM_R_LIMIT;
    }
    r = d->resources + slot;
    r->active = 1;
    r->count = count;
    strcpy(r->path, path);
    memcpy(r->operations, ops, count * sizeof(*ops));
    r->handler = handler;
    r->userdata = userdata;
    out->id = (unsigned long)slot + 1;
    out->generation = r->generation;
    unlock_dispatch(d);
    return FVM_R_OK;
}
/* Remove authority immediately and prevent stale queued work from touching replacement userdata. */
FVM_RResult fvm_rdispatch_unregister(FVM_RDispatch *d, FVM_RHandle h)
{
    Resource *r;
    size_t i;
    if (!d)
        return FVM_R_INVALID;
    if (!owner(d))
        return FVM_R_DENIED;
    lock_dispatch(d);
    r = resource(d, h);
    if (!r)
    {
        unlock_dispatch(d);
        return FVM_R_UNKNOWN;
    }
    if (r->running)
    {
        unlock_dispatch(d);
        return FVM_R_BUSY;
    }
    revoke_subscriptions(d, h, 0);
    r->active = 0;
    r->handler = 0;
    r->userdata = 0;
    r->generation = r->generation == FVM_SRECORD_U32 ? 0 : r->generation + 1;
    for (i = 0; i < FVM_R_GRANTS; ++i)
        if (d->grants[i].target.id == h.id)
            d->grants[i].session = 0;
    unlock_dispatch(d);
    return FVM_R_OK;
}
/* Update an existing grant in place or allocate one bounded native entry. */
FVM_RResult fvm_rdispatch_grant(FVM_RDispatch *d, FVM_RHandle h, unsigned long session,
                                unsigned int mask)
{
    size_t i, slot = FVM_R_GRANTS;
    if (!d || !valid_id(session) || (mask & ~FVM_R_RIGHTS))
        return FVM_R_INVALID;
    if (!owner(d))
        return FVM_R_DENIED;
    lock_dispatch(d);
    if (!resource(d, h))
    {
        unlock_dispatch(d);
        return FVM_R_UNKNOWN;
    }
    for (i = 0; i < FVM_R_GRANTS; ++i)
    {
        Grant *g = d->grants + i;
        if (g->session == session && g->target.id == h.id && g->target.generation == h.generation)
        {
            if (!(mask & FVM_R_SUBSCRIBE))
                revoke_subscriptions(d, h, session);
            g->rights = mask;
            if (!mask)
                g->session = 0;
            unlock_dispatch(d);
            return FVM_R_OK;
        }
        if (!g->session && slot == FVM_R_GRANTS)
            slot = i;
    }
    if (!mask)
    {
        unlock_dispatch(d);
        return FVM_R_OK;
    }
    if (slot == FVM_R_GRANTS)
    {
        unlock_dispatch(d);
        return FVM_R_LIMIT;
    }
    d->grants[slot].session = session;
    d->grants[slot].target = h;
    d->grants[slot].rights = mask;
    unlock_dispatch(d);
    return FVM_R_OK;
}
/* Resolve only the exact registered byte path; no implicit OS filesystem traversal. */
FVM_RResult fvm_rdispatch_lookup(FVM_RDispatch *d, unsigned long session, const char *path,
                                 FVM_RHandle *out)
{
    size_t i;
    if (!d || !valid_id(session) || !path_valid(path) || !out)
        return FVM_R_INVALID;
    lock_dispatch(d);
    for (i = 0; i < FVM_R_RESOURCES; ++i)
        if (d->resources[i].active && !strcmp(d->resources[i].path, path))
        {
            FVM_RHandle h;
            h.id = (unsigned long)i + 1;
            h.generation = d->resources[i].generation;
            if (!rights(d, h, session))
            {
                unlock_dispatch(d);
                return FVM_R_DENIED;
            }
            *out = h;
            unlock_dispatch(d);
            return FVM_R_OK;
        }
    unlock_dispatch(d);
    return FVM_R_UNKNOWN;
}
/* Producer post copies active bytes under lock; callback execution is deliberately separate. */
FVM_RResult fvm_rdispatch_post(FVM_RDispatch *d, const FVM_RRequest *q)
{
    size_t i;
    Resource *r;
    FVM_RResult result;
    Job *job;
    if (!d || !q || !valid_id(q->session) || !valid_id(q->id) || !valid_id(q->operation) ||
        (!q->data && q->size))
        return FVM_R_INVALID;
    if (q->size > FVM_R_BYTES)
        return FVM_R_LIMIT;
    lock_dispatch(d);
    if (find_job(d, q->session, q->id) != R_NONE)
    {
        unlock_dispatch(d);
        return FVM_R_CONFLICT;
    }
    result = authorize(d, q, &r);
    if (result != FVM_R_OK)
    {
        unlock_dispatch(d);
        return result;
    }
    for (i = 0; i < FVM_R_REQUESTS && d->jobs[i].state != R_FREE; ++i)
    {
    }
    if (i == FVM_R_REQUESTS)
    {
        unlock_dispatch(d);
        return FVM_R_LIMIT;
    }
    job = d->jobs + i;
    memset(job, 0, sizeof(*job));
    job->state = R_QUEUED;
    job->next = R_NONE;
    job->request = *q;
    if (q->size)
        memcpy(job->input, q->data, q->size);
    job->request.data = job->input;
    if (d->tail == R_NONE)
        d->head = (int)i;
    else
        d->jobs[d->tail].next = (int)i;
    d->tail = (int)i;
    unlock_dispatch(d);
    return FVM_R_OK;
}
/* Pop one FIFO job, revalidate authority, invoke owner code unlocked, then retain the reply. */
FVM_RResult fvm_rdispatch_pump(FVM_RDispatch *d, size_t budget, size_t *completed)
{
    size_t done = 0;
    if (!d || !completed || !budget || budget > FVM_R_REQUESTS)
        return FVM_R_INVALID;
    if (!owner(d))
        return FVM_R_DENIED;
    lock_dispatch(d);
    if (d->pumping)
    {
        unlock_dispatch(d);
        return FVM_R_BUSY;
    }
    d->pumping = 1;
    while (done < budget && d->head != R_NONE)
    {
        Job *job = d->jobs + d->head;
        Resource *r = 0;
        FVM_RResult result;
        size_t size = 0;
        d->head = job->next;
        if (d->head == R_NONE)
            d->tail = R_NONE;
        job->state = R_RUNNING;
        result = authorize(d, &job->request, &r);
        if (result == FVM_R_OK)
            r->running = 1;
        unlock_dispatch(d);
        if (result == FVM_R_OK)
        {
            result = r->handler(&job->request, job->reply.data, FVM_R_BYTES, &size, r->userdata);
            if (result < FVM_R_OK || result > FVM_R_HANDLER || result == FVM_R_PENDING ||
                size > FVM_R_BYTES)
            {
                result = FVM_R_HANDLER;
                size = 0;
            }
        }
        lock_dispatch(d);
        if (r)
            r->running = 0;
        job->reply.result = result;
        job->reply.size = size;
        job->state = R_DONE;
        ++done;
    }
    d->pumping = 0;
    *completed = done;
    unlock_dispatch(d);
    return FVM_R_OK;
}
/* Unlink cancelled queued work before allowing take to recycle its physical slot. */
FVM_RResult fvm_rdispatch_cancel(FVM_RDispatch *d, unsigned long session, unsigned long id)
{
    int slot, current, previous = R_NONE;
    Job *job;
    if (!d || !valid_id(session) || !valid_id(id))
        return FVM_R_INVALID;
    lock_dispatch(d);
    slot = find_job(d, session, id);
    if (slot == R_NONE)
    {
        unlock_dispatch(d);
        return FVM_R_UNKNOWN;
    }
    job = d->jobs + slot;
    if (job->state == R_RUNNING)
    {
        unlock_dispatch(d);
        return FVM_R_BUSY;
    }
    if (job->state == R_DONE)
    {
        unlock_dispatch(d);
        return FVM_R_CONFLICT;
    }
    current = d->head;
    while (current != slot)
    {
        previous = current;
        current = d->jobs[current].next;
    }
    if (previous == R_NONE)
        d->head = job->next;
    else
        d->jobs[previous].next = job->next;
    if (d->tail == slot)
        d->tail = previous;
    job->state = R_DONE;
    job->reply.result = FVM_R_CANCELLED;
    job->reply.size = 0;
    unlock_dispatch(d);
    return FVM_R_OK;
}
/* Result acknowledgement frees capacity only after copying the complete native reply. */
FVM_RResult fvm_rdispatch_take(FVM_RDispatch *d, unsigned long session, unsigned long id,
                               FVM_RReply *out)
{
    int slot;
    Job *job;
    if (!d || !out || !valid_id(session) || !valid_id(id))
        return FVM_R_INVALID;
    lock_dispatch(d);
    slot = find_job(d, session, id);
    if (slot == R_NONE)
    {
        unlock_dispatch(d);
        return FVM_R_UNKNOWN;
    }
    job = d->jobs + slot;
    if (job->state != R_DONE)
    {
        unlock_dispatch(d);
        return FVM_R_PENDING;
    }
    *out = job->reply;
    memset(job, 0, sizeof(*job));
    unlock_dispatch(d);
    return FVM_R_OK;
}
/* Copy discoverable metadata only after checking the entire required capacity. */
FVM_RResult fvm_rdispatch_resources(FVM_RDispatch *d, unsigned long session, FVM_RResource *rows,
                                    size_t capacity, size_t *count)
{
    size_t i, j, needed = 0;
    if (!d || !valid_id(session) || !count || (!rows && capacity))
        return FVM_R_INVALID;
    lock_dispatch(d);
    for (i = 0; i < FVM_R_RESOURCES; ++i)
    {
        FVM_RHandle h;
        h.id = (unsigned long)i + 1;
        h.generation = d->resources[i].generation;
        if (d->resources[i].active && rights(d, h, session))
            ++needed;
    }
    if (rows && capacity < needed)
    {
        unlock_dispatch(d);
        return FVM_R_LIMIT;
    }
    if (rows)
    {
        size_t index = 0;
        for (i = 0; i < FVM_R_RESOURCES; ++i)
        {
            Resource *r = d->resources + i;
            FVM_RHandle h;
            FVM_RResource *row;
            unsigned int mask;
            h.id = (unsigned long)i + 1;
            h.generation = r->generation;
            mask = rights(d, h, session);
            if (!r->active || !mask)
                continue;
            row = rows + index++;
            memset(row, 0, sizeof(*row));
            row->target = h;
            strcpy(row->path, r->path);
            row->rights = mask;
            for (j = 0; j < r->count; ++j)
                if (mask & r->operations[j].right)
                    row->operations[row->count++] = r->operations[j];
        }
    }
    *count = needed;
    unlock_dispatch(d);
    return FVM_R_OK;
}
/* Subscription creation is native and never enters resource/VM code. */
FVM_RResult fvm_rdispatch_subscribe(FVM_RDispatch *d, unsigned long session, FVM_RHandle target,
                                    FVM_RWatch *out)
{
    size_t i;
    if (!d || !valid_id(session) || !out)
        return FVM_R_INVALID;
    lock_dispatch(d);
    if (!resource(d, target))
    {
        unlock_dispatch(d);
        return FVM_R_UNKNOWN;
    }
    if (!(rights(d, target, session) & FVM_R_SUBSCRIBE))
    {
        unlock_dispatch(d);
        return FVM_R_DENIED;
    }
    for (i = 0; i < FVM_R_WATCHES; ++i)
    {
        Subscription *s = d->subscriptions + i;
        if (!s->session && s->generation)
        {
            s->session = session;
            s->target = target;
            out->id = (unsigned long)i + 1;
            out->generation = s->generation;
            unlock_dispatch(d);
            return FVM_R_OK;
        }
    }
    unlock_dispatch(d);
    return FVM_R_LIMIT;
}
/* Slow readers lose only their own oldest copy; owner publication never waits for them. */
FVM_RResult fvm_rdispatch_publish(FVM_RDispatch *d, FVM_RHandle target, unsigned long kind,
                                  const void *data, size_t size, unsigned long *sequence)
{
    size_t i;
    if (!d || !valid_id(kind) || !sequence || (!data && size))
        return FVM_R_INVALID;
    if (size > FVM_R_BYTES)
        return FVM_R_LIMIT;
    if (!owner(d))
        return FVM_R_DENIED;
    lock_dispatch(d);
    if (!resource(d, target))
    {
        unlock_dispatch(d);
        return FVM_R_UNKNOWN;
    }
    if (d->sequence == FVM_SRECORD_U32)
    {
        unlock_dispatch(d);
        return FVM_R_LIMIT;
    }
    ++d->sequence;
    for (i = 0; i < FVM_R_WATCHES; ++i)
    {
        Subscription *s = d->subscriptions + i;
        FVM_REvent *event;
        if (!s->session || s->target.id != target.id || s->target.generation != target.generation)
            continue;
        if (s->count == FVM_R_EVENT_QUEUE)
        {
            s->head = (s->head + 1) % FVM_R_EVENT_QUEUE;
            --s->count;
            if (s->missed != FVM_SRECORD_U32)
                ++s->missed;
        }
        event = s->events + (s->head + s->count) % FVM_R_EVENT_QUEUE;
        memset(event, 0, sizeof(*event));
        event->target = target;
        event->sequence = d->sequence;
        event->kind = kind;
        event->size = size;
        if (size)
            memcpy(event->data, data, size);
        ++s->count;
    }
    *sequence = d->sequence;
    unlock_dispatch(d);
    return FVM_R_OK;
}
/* Session identity and generation are checked before exposing or consuming a copy. */
FVM_RResult fvm_rdispatch_event_take(FVM_RDispatch *d, unsigned long session, FVM_RWatch h,
                                     FVM_REvent *out)
{
    Subscription *s;
    if (!d || !valid_id(session) || !out)
        return FVM_R_INVALID;
    lock_dispatch(d);
    s = subscription(d, h);
    if (!s)
    {
        unlock_dispatch(d);
        return FVM_R_UNKNOWN;
    }
    if (s->session != session)
    {
        unlock_dispatch(d);
        return FVM_R_DENIED;
    }
    if (!s->count)
    {
        unlock_dispatch(d);
        return FVM_R_PENDING;
    }
    *out = s->events[s->head];
    out->missed = s->missed;
    s->missed = 0;
    s->head = (s->head + 1) % FVM_R_EVENT_QUEUE;
    --s->count;
    unlock_dispatch(d);
    return FVM_R_OK;
}
/* Closing discards only one reader, not any other observer or committed command. */
FVM_RResult fvm_rdispatch_unsubscribe(FVM_RDispatch *d, unsigned long session, FVM_RWatch h)
{
    Subscription *s;
    if (!d || !valid_id(session))
        return FVM_R_INVALID;
    lock_dispatch(d);
    s = subscription(d, h);
    if (!s)
    {
        unlock_dispatch(d);
        return FVM_R_UNKNOWN;
    }
    if (s->session != session)
    {
        unlock_dispatch(d);
        return FVM_R_DENIED;
    }
    close_subscription(s);
    unlock_dispatch(d);
    return FVM_R_OK;
}
/* Remove one quiesced session atomically without reordering other producers' jobs. */
FVM_RResult fvm_rdispatch_disconnect(FVM_RDispatch *d, unsigned long session)
{
    size_t i;
    int current, previous = R_NONE;
    if (!d || !valid_id(session))
        return FVM_R_INVALID;
    if (!owner(d))
        return FVM_R_DENIED;
    lock_dispatch(d);
    for (i = 0; i < FVM_R_REQUESTS; ++i)
        if (d->jobs[i].state == R_RUNNING && d->jobs[i].request.session == session)
        {
            unlock_dispatch(d);
            return FVM_R_BUSY;
        }
    current = d->head;
    while (current != R_NONE)
    {
        Job *job = d->jobs + current;
        int next = job->next;
        if (job->request.session == session)
        {
            if (previous == R_NONE)
                d->head = next;
            else
                d->jobs[previous].next = next;
            if (d->tail == current)
                d->tail = previous;
            memset(job, 0, sizeof(*job));
        }
        else
            previous = current;
        current = next;
    }
    for (i = 0; i < FVM_R_REQUESTS; ++i)
        if (d->jobs[i].state == R_DONE && d->jobs[i].request.session == session)
            memset(d->jobs + i, 0, sizeof(d->jobs[i]));
    for (i = 0; i < FVM_R_GRANTS; ++i)
        if (d->grants[i].session == session)
            d->grants[i].session = 0;
    for (i = 0; i < FVM_R_WATCHES; ++i)
        if (d->subscriptions[i].session == session)
            close_subscription(d->subscriptions + i);
    unlock_dispatch(d);
    return FVM_R_OK;
}
/* Caller has quiesced other native users; accepted requests must all be acknowledged first. */
FVM_RResult fvm_rdispatch_destroy(FVM_RDispatch **pointer)
{
    FVM_RDispatch *d;
    size_t i;
    if (!pointer || !*pointer)
        return FVM_R_INVALID;
    d = *pointer;
    if (!owner(d))
        return FVM_R_DENIED;
    lock_dispatch(d);
    if (d->pumping)
    {
        unlock_dispatch(d);
        return FVM_R_BUSY;
    }
    for (i = 0; i < FVM_R_REQUESTS; ++i)
        if (d->jobs[i].state != R_FREE)
        {
            unlock_dispatch(d);
            return FVM_R_BUSY;
        }
    unlock_dispatch(d);
    free(d);
    *pointer = 0;
    return FVM_R_OK;
}
