/* Real POSIX producers share copied control requests with exactly one owner-thread pump. */
#define _POSIX_C_SOURCE 199309L
#include "flashvm/rdispatch.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define WORKERS 4U
#define JOBS 50U
/* Native synchronization and identity remain owned by the embedding host. */
typedef struct Shared
{
    pthread_mutex_t mutex;
    pthread_t owner;
    FVM_RDispatch *dispatch;
    FVM_RHandle target;
    unsigned int calls;
    int failed;
} Shared;
/* Each producer has its own authenticated-session stand-in and thread-local payload bytes. */
typedef struct Producer
{
    Shared *shared;
    unsigned int session;
} Producer;
/* Policy hooks are plain host mutex operations, never VM entrypoints. */
static void acquire(void *raw)
{
    pthread_mutex_lock(&((Shared *)raw)->mutex);
}
/* Same mutex protects queue metadata and the fixture failure flag. */
static void release(void *raw)
{
    pthread_mutex_unlock(&((Shared *)raw)->mutex);
}
/* Ownership is compared by native thread identity, not by a caller-provided packet field. */
static int owner(void *raw)
{
    return pthread_equal(pthread_self(), ((Shared *)raw)->owner);
}
/* Sleep yields to backpressure; it is not part of dispatcher behavior. */
static void yield_thread(void)
{
    struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = 1000000;
    nanosleep(&delay, 0);
}
/* Handler must run on owner with the policy mutex released and a stable copied input byte. */
static FVM_RResult handle(const FVM_RRequest *request, void *out, size_t capacity, size_t *used,
                          void *raw)
{
    Shared *s = (Shared *)raw;
    /* Taking the same nonrecursive mutex would deadlock if pump retained it here. */
    acquire(s);
    if (!owner(s) || capacity != FVM_R_BYTES || request->size != 1 ||
        ((const unsigned char *)request->data)[0] != (unsigned char)request->session)
        s->failed = 1;
    release(s);
    ((unsigned char *)out)[0] = ((const unsigned char *)request->data)[0];
    *used = 1;
    ++s->calls;
    return FVM_R_OK;
}
/* Worker posts only; attempting owner pumping first must be denied without output publication. */
static void *produce(void *raw)
{
    Producer *p = (Producer *)raw;
    Shared *s = p->shared;
    FVM_RRequest q;
    unsigned int i;
    unsigned char byte;
    size_t done = 999;
    FVM_RResult result;
    if (fvm_rdispatch_pump(s->dispatch, 1, &done) != FVM_R_DENIED || done != 999)
    {
        acquire(s);
        s->failed = 1;
        release(s);
        return 0;
    }
    memset(&q, 0, sizeof(q));
    q.session = p->session;
    q.target = s->target;
    q.operation = 1;
    q.data = &byte;
    q.size = 1;
    for (i = 1; i <= JOBS; ++i)
    {
        q.id = i;
        byte = (unsigned char)p->session;
        do
        {
            result = fvm_rdispatch_post(s->dispatch, &q);
            if (result == FVM_R_LIMIT)
                yield_thread();
        } while (result == FVM_R_LIMIT);
        if (result != FVM_R_OK)
        {
            acquire(s);
            s->failed = 1;
            release(s);
            return 0;
        }
        byte = 0xa5; /* Mutation after post must not affect the owner callback's copied input. */
    }
    return 0;
}
/* A bounded owner loop drains retained replies while producers apply ordinary queue backpressure.
 */
int main(void)
{
    Shared s;
    Producer producers[WORKERS];
    pthread_t threads[WORKERS];
    FVM_RPolicy policy;
    FVM_ROperation operation = {1, FVM_R_READ};
    FVM_RReply reply;
    FVM_RResult result;
    size_t done;
    unsigned int i, j, received = 0;
    int failed;
    time_t deadline;
    memset(&s, 0, sizeof(s));
    s.owner = pthread_self();
    if (pthread_mutex_init(&s.mutex, 0) != 0)
        return 1;
    memset(&policy, 0, sizeof(policy));
    policy.struct_size = sizeof(policy);
    policy.lock = acquire;
    policy.unlock = release;
    policy.is_owner = owner;
    policy.userdata = &s;
    if (fvm_rdispatch_create(&policy, &s.dispatch) != FVM_R_OK ||
        fvm_rdispatch_register(s.dispatch, "/scene/root/value", &operation, 1, handle, &s,
                               &s.target) != FVM_R_OK)
        return 1;
    for (i = 0; i < WORKERS; ++i)
    {
        producers[i].shared = &s;
        producers[i].session = i + 1;
        if (fvm_rdispatch_grant(s.dispatch, s.target, i + 1, FVM_R_READ) != FVM_R_OK ||
            pthread_create(&threads[i], 0, produce, &producers[i]) != 0)
            return 1;
    }
    deadline = time(0) + 20;
    while (received < WORKERS * JOBS && time(0) <= deadline)
    {
        if (fvm_rdispatch_pump(s.dispatch, FVM_R_REQUESTS, &done) != FVM_R_OK)
            return 1;
        for (i = 1; i <= WORKERS; ++i)
            for (j = 1; j <= JOBS; ++j)
            {
                result = fvm_rdispatch_take(s.dispatch, i, j, &reply);
                if (result == FVM_R_OK)
                {
                    if (reply.result != FVM_R_OK || reply.size != 1 || reply.data[0] != i)
                        return 1;
                    ++received;
                }
                else if (result != FVM_R_PENDING && result != FVM_R_UNKNOWN)
                    return 1;
            }
        acquire(&s);
        failed = s.failed;
        release(&s);
        if (failed)
            return 1;
        if (!done)
            yield_thread();
    }
    if (received != WORKERS * JOBS)
        return 1;
    for (i = 0; i < WORKERS; ++i)
        if (pthread_join(threads[i], 0) != 0)
            return 1;
    if (s.calls != WORKERS * JOBS || s.failed || fvm_rdispatch_destroy(&s.dispatch) != FVM_R_OK ||
        s.dispatch)
        return 1;
    if (pthread_mutex_destroy(&s.mutex) != 0)
        return 1;
    puts("PASS four real producers, 200 copied requests, owner-only callbacks, unlocked handlers "
         "and retained-reply backpressure");
    return 0;
}
