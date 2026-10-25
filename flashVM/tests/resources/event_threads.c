/* Real concurrent observers share the dispatcher's host-owned nonrecursive mutex. */
#include "flashvm/revents.h"
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>

#define EVENTS 500U
#define READERS 3U
#define FINISHED 2UL


typedef struct Reader
{
    FVM_RDispatch *dispatch;
    FVM_RWatch watch;
    unsigned long session, seen, missed, last;
    int failed;
} Reader;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t owner;


static void acquire(void *raw)
{
    (void)raw;
    pthread_mutex_lock(&mutex);
}


static void release(void *raw)
{
    (void)raw;
    pthread_mutex_unlock(&mutex);
}


static int is_owner(void *raw)
{
    (void)raw;
    return pthread_equal(owner, pthread_self());
}


static FVM_RResult handler(const FVM_RRequest *q, void *r, size_t n, size_t *used, void *p)
{
    (void)q;
    (void)r;
    (void)n;
    (void)p;
    *used = 0;
    return FVM_R_OK;
}


static void *consume(void *raw)
{
    Reader *r = (Reader *)raw;
    for (;;)
    {
        FVM_REvent event;
        FVM_RResult status = fvm_rdispatch_event_take(r->dispatch, r->session, r->watch, &event);
        if (status == FVM_R_PENDING)
        {
            sched_yield();
            continue;
        }
        if (status != FVM_R_OK)
        {
            r->failed = 1;
            return 0;
        }
        if (event.sequence <= r->last || event.size != 1 || event.data[0] != (unsigned char)event.sequence)
        {
            r->failed = 1;
        }
        r->last = event.sequence;
        ++r->seen;
        r->missed += event.missed;
        if (event.kind == FINISHED)
        {
            return 0;
        }
    }
}


int main(void)
{
    FVM_RPolicy policy;
    FVM_RDispatch *dispatch = 0;
    FVM_RHandle target;
    FVM_ROperation op = {1, FVM_R_READ};
    Reader readers[READERS];
    pthread_t threads[READERS];
    unsigned int i;
    owner = pthread_self();
    memset(&policy, 0, sizeof(policy));
    policy.struct_size = sizeof(policy);
    policy.lock = acquire;
    policy.unlock = release;
    policy.is_owner = is_owner;
    if (fvm_rdispatch_create(&policy, &dispatch) != FVM_R_OK ||
        fvm_rdispatch_register(dispatch, "/debug/events", &op, 1, handler, 0, &target) != FVM_R_OK)
    {
        return 1;
    }
    memset(readers, 0, sizeof(readers));
    for (i = 0; i < READERS; ++i)
    {
        Reader *r = readers + i;
        r->dispatch = dispatch;
        r->session = (unsigned long)i + 1;
        if (fvm_rdispatch_grant(dispatch, target, r->session, FVM_R_SUBSCRIBE) != FVM_R_OK ||
            fvm_rdispatch_subscribe(dispatch, r->session, target, &r->watch) != FVM_R_OK ||
            pthread_create(threads + i, 0, consume, r))
        {
            return 1;
        }
    }
    for (i = 1; i <= EVENTS + 1; ++i)
    {
        unsigned char byte = (unsigned char)i;
        unsigned long sequence;
        if (fvm_rdispatch_publish(dispatch, target, i == EVENTS + 1 ? FINISHED : 1, &byte, 1, &sequence) != FVM_R_OK ||
            sequence != i)
        {
            return 1;
        }
    }
    for (i = 0; i < READERS; ++i)
    {
        Reader *r = readers + i;
        if (pthread_join(threads[i], 0) || r->failed || r->last != EVENTS + 1 || r->seen + r->missed != EVENTS + 1 ||
            fvm_rdispatch_unsubscribe(dispatch, r->session, r->watch) != FVM_R_OK)
        {
            return 1;
        }
    }
    if (fvm_rdispatch_destroy(&dispatch) != FVM_R_OK)
    {
        return 1;
    }
    puts("PASS three independent observers: seen + reported loss = 501 each");
    return 0;
}
