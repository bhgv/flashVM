/* Copyright (c) 2026 bhgv
 * EN: Actual multiplexer admission keeps one ordinary RPC slot despite multiple watch callers.
 * RU: Настоящий multiplexer admission сохраняет ordinary RPC slot при нескольких watch callers.
 */
#include "../../client/mux_internal.h"
#include <time.h>


/* EN/RU: Native fixture checks remain active in optimized builds.
 * Native fixture checks активны в optimized builds. */
static void check(int valid, char *why)
{
    if (!valid)
    {
        fprint(2, "watch quota: %s\n", why);
        exit(2);
    }
}


/* EN: Own one admission attempt from an independent test thread.
 * RU: Запустить одну попытку допуска в независимом тестовом потоке. */
typedef struct Admission
{
    StyxMux *mux; /* Borrowed until worker joins. */
    const uchar *bytes; /* Borrowed encoded frame. */
    unsigned size; /* Request byte count. */
    StyxCancel *cancel; /* Owned by the test coordinator. */
    StyxFlight *flight; /* Owned result after thread join, or NULL. */
} Admission;


/* EN: Wait for an initial tag with the request still unsent.
 * RU: Ждать начальный тег до отправки запроса. */
static void *wait_for_tag(void *state)
{
    Admission *job = state;
    job->flight = styxmuxsubmit(job->mux, job->bytes, job->size, 256, job->cancel, 0);
    return NULL;
}


/* EN: Observe the exact queued admission before freeing a tag or cancelling it.
 * RU: Дождаться постановки запроса в ожидание тега перед освобождением или отменой. */
static int awaiting_slot(StyxMux *mux)
{
    double end = styxmuxtime() + mux->timeout;
    do
    {
        pthread_mutex_lock(&mux->lock);
        unsigned waiters = mux->admission_waiters;
        pthread_mutex_unlock(&mux->lock);
        if (waiters)
        {
            return 1;
        }
        struct timespec pause = {0, 1000 * 1000};
        nanosleep(&pause, NULL);
    } while (styxmuxtime() < end);
    return 0;
}


/* EN/RU: Verify watch quota refusal before effects and ordinary admission/retirement.
 * Проверить watch quota refusal до effects и ordinary admission/retirement. */
int main(void)
{
    StyxMux mux;
    memset(&mux, 0, sizeof(mux));
    StyxFlight *slots[2] = {nil, nil};
    mux.slots = slots;
    mux.limit = 2;
    mux.capacity = 256;
    mux.timeout = 1000;
    check(!pthread_mutex_init(&mux.lock, nil) && !pthread_cond_init(&mux.ended, nil), "native lock");
    Fcall request;
    uchar bytes[256];
    memset(&request, 0, sizeof(request));
    request.type = Tread;
    request.fid = 2;
    request.count = 16;
    unsigned size = convS2M(&request, bytes, sizeof(bytes));
    StyxFlight *first = styxmuxsubmit(&mux, bytes, size, 256, nil, 1);
    check(first != nil && first->watching, "initial watch admission");
    check(styxmuxsubmit(&mux, bytes, size, 256, nil, 1) == nil && !mux.failed, "watch consumed final ordinary slot");
    StyxFlight *ordinary = styxmuxsubmit(&mux, bytes, size, 256, nil, 0);
    check(ordinary != nil && !ordinary->watching && ordinary->tag != first->tag, "ordinary work starved by subscription");
    check(!styxmuxsubmit(&mux, bytes, size, 256, nil, 0), "raw quota stopped rejecting");
    Admission job = {.mux = &mux, .bytes = bytes, .size = size, .cancel = styxcancelcreatewait()};
    pthread_t worker;
    check(job.cancel && !pthread_create(&worker, NULL, wait_for_tag, &job) &&
        awaiting_slot(&mux), "cancellable admission did not wait");
    styxcancelrequest(job.cancel);
    check(!pthread_join(worker, NULL) && !job.flight, "cancelled admission obtained a tag");
    styxcancelfree(job.cancel);
    job.cancel = styxcancelcreatewait();
    check(job.cancel && !pthread_create(&worker, NULL, wait_for_tag, &job) &&
        awaiting_slot(&mux), "second admission did not wait");
    pthread_mutex_lock(&mux.lock);
    styxmuxfinish(&mux, ordinary);
    pthread_mutex_unlock(&mux.lock);
    check(!pthread_join(worker, NULL) && job.flight && job.flight->tag == ordinary->tag,
        "released tag did not admit waiting work");
    pthread_mutex_lock(&mux.lock);
    styxmuxbroken(&mux);
    pthread_mutex_unlock(&mux.lock);
    StyxAnswer a = styxmuxcollect(first);
    StyxAnswer b = styxmuxcollect(ordinary);
    StyxAnswer c = styxmuxcollect(job.flight);
    check(!a.attempted && !b.attempted && !a.length && !b.length, "unsent retirement invented effects");
    free(a.bytes);
    free(b.bytes);
    free(c.bytes);
    styxcancelfree(job.cancel);
    pthread_cond_destroy(&mux.ended);
    pthread_mutex_destroy(&mux.lock);
    print("PASS native watch quota keeps ordinary RPC capacity\n");
    return 0;
}
