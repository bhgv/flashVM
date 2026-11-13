/* Copyright (c) 2026 bhgv
 * EN: Concurrent proxy admission/ref ownership on native pthreads, without VM stop.
 * RU: Конкурентное proxy admission/ref ownership на pthreads без остановки VM.
 */
#include "session.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define THREADS 8U  /* Independent native callers sharing one session. */
#define CALLS 1000U /* Unique admitted effects per native caller. */


/* EN/RU: Shared immutable fixture plus atomic effect/failure counters.
 * Общий immutable fixture и atomic счётчики effects/failures. */
typedef struct
{
    DvmRemoteSession *session; /* Borrowed until every thread joins. */
    DvmRemoteHandle function;  /* Shared original function identity. */
    DvmRemoteHandle variable;  /* Shared original variable identity, aliased twice. */
    unsigned int effects;      /* Atomic proof that duplicates never execute twice. */
    unsigned int failures;     /* Atomic unexpected result count. */
} Fixture;


/* EN/RU: One worker owns its ID range; fixture remains borrowed through join.
 * Worker владеет диапазоном IDs; fixture заимствован до join. */
typedef struct
{
    Fixture *fixture;   /* Shared native roots/session, not managed pointers. */
    unsigned int index; /* Non-overlapping request ID range selector. */
} Worker;


/* EN/RU: A test-only admission rendezvous, never a production VM/world barrier.
 * Test-only rendezvous после admission, не production VM/world barrier. */
typedef struct
{
    Fixture *fixture;           /* Borrowed shared session and checked identities. */
    pthread_barrier_t admitted; /* Hold fixture calls active until close is checked. */
    pthread_barrier_t finish;   /* Allow fixture workers to finish after disconnect. */
} Disconnect;


/* EN/RU: Worker-local disconnected call ID and shared test rendezvous.
 * Локальный ID call и общий test rendezvous. */
typedef struct
{
    Disconnect *disconnect; /* Borrowed through thread join. */
    uint64_t id;            /* Unique pre-admitted call ID. */
} ClosingWorker;


/* EN: Check worker result without assert; atomically report any mismatch.
 * RU: Проверить result без assert; атомарно сообщить несовпадение. */
static void check(Fixture *fixture, int condition)
{
    if (!condition)
    {
        __atomic_fetch_add(&fixture->failures, 1, __ATOMIC_RELAXED);
    }
}


/* EN: Exercise one original function and repeated live ref identities from many
 * threads. Native atomic effects stand in for future owner execution, not VM GC.
 * RU: Вызывать один function target с repeated live refs из многих потоков.
 * Native atomic effects моделируют owner execution; это не проверка VM GC. */
static void *run(void *argument)
{
    Worker *worker = argument;
    Fixture *fixture = worker->fixture;
    unsigned int index;
    for (index = 0; index < CALLS; index++)
    {
        int done;
        DvmRemoteRequest request;
        memset(&request, 0, sizeof(request));
        request.id = 1 + worker->index * CALLS + index;
        request.target = fixture->function;
        request.kind = DVM_REMOTE_FUNCTION;
        request.ref_count = 2;
        request.refs[0] = fixture->variable;
        request.refs[1] = fixture->variable;
        check(fixture, dvm_remote_retain(fixture->session, fixture->variable) == DVM_REMOTE_OK);
        if (dvm_remote_admit(fixture->session, &request, &done) == DVM_REMOTE_OK)
        {
            check(fixture, dvm_remote_admit(fixture->session, &request, &done) == DVM_REMOTE_DUPLICATE && done == 0);
            __atomic_fetch_add(&fixture->effects, 1, __ATOMIC_RELAXED);
            check(fixture, dvm_remote_finish(fixture->session, request.id) == DVM_REMOTE_OK);
            check(fixture, dvm_remote_admit(fixture->session, &request, &done) == DVM_REMOTE_DUPLICATE && done == 1);
        }
        else
        {
            check(fixture, 0);
        }
        check(fixture, dvm_remote_release(fixture->session, fixture->variable) == DVM_REMOTE_OK);
    }
    return NULL;
}


/* EN: Hold an admitted call across disconnect; effects remain allowed after
 * transport close, and completion still releases its original target/ref pins.
 * RU: Удержать admitted call через disconnect; эффекты допустимы после close,
 * completion снимает pins исходных target/refs. Fixture barriers only. */
static void *run_disconnected(void *argument)
{
    ClosingWorker *worker = argument;
    Disconnect *disconnect = worker->disconnect;
    Fixture *fixture = disconnect->fixture;
    DvmRemoteRequest request;
    int done;
    memset(&request, 0, sizeof(request));
    request.id = worker->id;
    request.target = fixture->function;
    request.kind = DVM_REMOTE_FUNCTION;
    request.ref_count = 2;
    request.refs[0] = fixture->variable;
    request.refs[1] = fixture->variable;
    check(fixture, dvm_remote_admit(fixture->session, &request, &done) == DVM_REMOTE_OK);
    pthread_barrier_wait(&disconnect->admitted);
    pthread_barrier_wait(&disconnect->finish);
    __atomic_fetch_add(&fixture->effects, 1, __ATOMIC_RELAXED);
    check(fixture, dvm_remote_finish(fixture->session, request.id) == DVM_REMOTE_OK);
    return NULL;
}


/* EN: All eight calls are pinned when close runs. A second independent session
 * admits/completes work while those calls are deliberately held unfinished.
 * RU: Все восемь calls pinned в момент close. Вторая независимая сессия принимает
 * и завершает work, пока первые calls намеренно не завершаются. */
static void concurrent_disconnect(void)
{
    Fixture fixture;
    Disconnect disconnect;
    ClosingWorker workers[THREADS];
    pthread_t threads[THREADS];
    DvmRemoteSession *other;
    DvmRemoteRequest request;
    DvmRemoteRoot root;
    int tokens[3] = {0, 1, 2};
    int done;
    unsigned int index;
    memset(&fixture, 0, sizeof(fixture));
    memset(&request, 0, sizeof(request));
    fixture.session = dvm_remote_session_new(2, THREADS + 1, 100);
    other = dvm_remote_session_new(1, 1, 100);
    if (fixture.session == NULL || other == NULL ||
        dvm_remote_publish(fixture.session, DVM_REMOTE_FUNCTION, &tokens[0], &fixture.function) != DVM_REMOTE_OK ||
        dvm_remote_publish(fixture.session, DVM_REMOTE_VARIABLE, &tokens[1], &fixture.variable) != DVM_REMOTE_OK ||
        pthread_barrier_init(&disconnect.admitted, NULL, THREADS + 1) != 0 ||
        pthread_barrier_init(&disconnect.finish, NULL, THREADS + 1) != 0)
    {
        fputs("FAIL disconnect fixture\n", stderr);
        exit(1);
    }
    disconnect.fixture = &fixture;
    for (index = 0; index < THREADS; index++)
    {
        workers[index].disconnect = &disconnect;
        workers[index].id = index + 1;
        if (pthread_create(&threads[index], NULL, run_disconnected, &workers[index]) != 0)
        {
            exit(1);
        }
    }
    pthread_barrier_wait(&disconnect.admitted);
    dvm_remote_session_close(fixture.session);
    check(&fixture, dvm_remote_collect(fixture.session, &root) == DVM_REMOTE_STALE);
    check(&fixture, dvm_remote_session_destroy(fixture.session) == DVM_REMOTE_BUSY);
    check(&fixture, dvm_remote_publish(other, DVM_REMOTE_FUNCTION, &tokens[2], &request.target) == DVM_REMOTE_OK);
    request.id = 1;
    request.kind = DVM_REMOTE_FUNCTION;
    check(&fixture, dvm_remote_admit(other, &request, &done) == DVM_REMOTE_OK);
    check(&fixture, dvm_remote_finish(other, 1) == DVM_REMOTE_OK);
    dvm_remote_session_close(other);
    check(&fixture, dvm_remote_collect(other, &root) == DVM_REMOTE_OK && root.token == &tokens[2]);
    check(&fixture, dvm_remote_session_destroy(other) == DVM_REMOTE_OK);
    pthread_barrier_wait(&disconnect.finish);
    for (index = 0; index < THREADS; index++)
    {
        if (pthread_join(threads[index], NULL) != 0)
        {
            exit(1);
        }
    }
    check(&fixture, fixture.effects == THREADS);
    check(&fixture, dvm_remote_collect(fixture.session, &root) == DVM_REMOTE_OK && root.token == &tokens[0]);
    check(&fixture, dvm_remote_collect(fixture.session, &root) == DVM_REMOTE_OK && root.token == &tokens[1]);
    check(&fixture, dvm_remote_session_destroy(fixture.session) == DVM_REMOTE_OK);
    pthread_barrier_destroy(&disconnect.admitted);
    pthread_barrier_destroy(&disconnect.finish);
    if (fixture.failures != 0)
    {
        fprintf(stderr, "FAIL disconnect operations: %u\n", fixture.failures);
        exit(1);
    }
    puts("PASS remote disconnect: eight active calls retain roots, independent session progresses");
}


/* EN: Run eight actual OS threads, then prove exact cleanup on their owner.
 * RU: Запустить восемь OS threads, затем проверить точную очистку у owner. */
int main(void)
{
    Fixture fixture;
    Worker workers[THREADS];
    pthread_t threads[THREADS];
    DvmRemoteRoot root;
    unsigned int index;
    int tokens[2] = {0, 1};
    memset(&fixture, 0, sizeof(fixture));
    fixture.session = dvm_remote_session_new(2, THREADS * CALLS + 1, 100);
    if (fixture.session == NULL ||
        dvm_remote_publish(fixture.session, DVM_REMOTE_FUNCTION, &tokens[0], &fixture.function) != DVM_REMOTE_OK ||
        dvm_remote_publish(fixture.session, DVM_REMOTE_VARIABLE, &tokens[1], &fixture.variable) != DVM_REMOTE_OK)
    {
        fputs("FAIL concurrent fixture allocation\n", stderr);
        return 1;
    }
    for (index = 0; index < THREADS; index++)
    {
        workers[index].fixture = &fixture;
        workers[index].index = index;
        if (pthread_create(&threads[index], NULL, run, &workers[index]) != 0)
        {
            fputs("FAIL pthread_create\n", stderr);
            exit(1);
        }
    }
    for (index = 0; index < THREADS; index++)
    {
        if (pthread_join(threads[index], NULL) != 0)
        {
            fputs("FAIL pthread_join\n", stderr);
            exit(1);
        }
    }
    check(&fixture, fixture.effects == THREADS * CALLS);
    check(&fixture, dvm_remote_collect(fixture.session, &root) == DVM_REMOTE_STALE);
    dvm_remote_session_close(fixture.session);
    check(&fixture, dvm_remote_collect(fixture.session, &root) == DVM_REMOTE_OK && root.token == &tokens[0]);
    check(&fixture, dvm_remote_collect(fixture.session, &root) == DVM_REMOTE_OK && root.token == &tokens[1]);
    check(&fixture, dvm_remote_collect(fixture.session, &root) == DVM_REMOTE_STALE);
    check(&fixture, dvm_remote_session_destroy(fixture.session) == DVM_REMOTE_OK);
    if (fixture.failures != 0)
    {
        fprintf(stderr, "FAIL concurrent operations: %u\n", fixture.failures);
        return 1;
    }
    printf("PASS remote session: %u native threads, %u effects, aliases and exact cleanup\n", THREADS, fixture.effects);
    concurrent_disconnect();
    return 0;
}
