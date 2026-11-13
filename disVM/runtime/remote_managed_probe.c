/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real managed exports survive native pins and concurrent collector passes.
 * RU: Настоящие managed exports переживают native pins и проходы конкурентного GC.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "remote_managed.h"
#include "root_registry.h"
#include "runtime_gc.h"
#include "gc/collector_passes.h"
#include "disvm/refcount.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REMOTE_PEERS 8U                 /* Native request peers never dereference managed roots. */

int dvm_remote_managed_probe_requested; /* Immutable before runtime/thread startup. */


/* EN/RU: Native-only request metadata and fixture synchronization.
 * Только native request metadata и синхронизация fixture. */
typedef struct
{
    DvmRemoteManaged *adapter;  /* Borrowed through every peer join. */
    DvmRemoteHandle target;     /* Managed export identity, not a value pointer. */
    DvmRemoteHandle argument;   /* Same variable used as two alias arguments. */
    pthread_barrier_t admitted; /* Test-only hold after calls become pinned. */
    pthread_barrier_t complete; /* Test-only release after rooted collector passes. */
    unsigned int failures;      /* Atomic native result counter. */
} Probe;


/* EN/RU: Per-thread immutable correlation ID and shared fixture lifetime.
 * Immutable correlation ID потока и время жизни общей fixture. */
typedef struct
{
    Probe *probe; /* Borrowed native metadata only. */
    uint64_t id;  /* Unique admitted operation ID. */
} Peer;


/* EN: Abort diagnostic before unsafe continuation; message is borrowed text.
 * RU: Завершить диагностику до unsafe continuation; message заимствован. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL remote managed roots: %s\n", message);
        exit(10);
    }
}


/* EN: On unregistered native peers reject heap API before pointer access, then
 * hold exact root pins across owner close and GC. No managed callback executes.
 * RU: На незарегистрированном native peer отвергать heap API до pointer access;
 * держать pins через owner close и GC. Managed callback не выполняется. */
static void *native_peer(void *argument)
{
    Peer *peer = argument;
    Probe *probe = peer->probe;
    DvmRemoteRequest request;
    DvmRemoteHandle invalid = {0, 0, 0};
    size_t released = 99;
    int done;
    memset(&request, 0, sizeof(request));
    request.id = peer->id;
    request.kind = DVM_REMOTE_FUNCTION;
    request.target = probe->target;
    request.ref_count = 2;
    request.refs[0] = probe->argument;
    request.refs[1] = probe->argument;
    int valid =
        dvm_remote_managed_publish(probe->adapter, DVM_REMOTE_VARIABLE, (void *)1, &invalid) == DVM_REMOTE_INVALID &&
        dvm_remote_managed_collect(probe->adapter, 1, &released) == DVM_REMOTE_INVALID && released == 99 &&
        dvm_remote_managed_destroy(probe->adapter) == DVM_REMOTE_INVALID &&
        dvm_remote_managed_retain(probe->adapter, probe->argument) == DVM_REMOTE_OK &&
        dvm_remote_managed_release(probe->adapter, probe->argument) == DVM_REMOTE_OK &&
        dvm_remote_managed_admit(probe->adapter, &request, &done) == DVM_REMOTE_OK;
    if (!valid)
    {
        __atomic_fetch_add(&probe->failures, 1, __ATOMIC_RELAXED);
    }
    pthread_barrier_wait(&probe->admitted);
    pthread_barrier_wait(&probe->complete);
    if (dvm_remote_managed_finish(probe->adapter, request.id) != DVM_REMOTE_OK)
    {
        __atomic_fetch_add(&probe->failures, 1, __ATOMIC_RELAXED);
    }
    return NULL;
}


/* [GC:] EN: Startup controller uses its real ACTIVE context, two actual String
 * heaps and native peers. Function-kind root tests lifetime only, not invocation.
 * RU: Startup controller использует реальный ACTIVE context, две String heaps
 * и native peers. Function-kind root проверяет lifetime, не invocation. */
void dvm_remote_managed_probe(void)
{
    Probe probe;
    Peer peers[REMOTE_PEERS];
    pthread_t threads[REMOTE_PEERS];
    String *target;
    String *argument;
    DvmRemoteManaged *closed;
    DvmRemoteHandle untouched = {11, 22, 33};
    size_t released;
    unsigned int index;
    unsigned int pressure = 0;
    int baseline;
    check(dvm_runtime_gc_concurrent_drain(), "initial concurrent drain");
    baseline = dvm_refcount_load_i(&Tstring.ref);
    memset(&probe, 0, sizeof(probe));
    probe.adapter = dvm_remote_managed_new(2, REMOTE_PEERS + 1, 100);
    closed = dvm_remote_managed_new(1, 1, 100);
    check(probe.adapter && closed, "ACTIVE adapter creation");
    target = c2string("proxy-root", 10);
    ptradd(D2H(target));
    check(dvm_remote_managed_publish(probe.adapter, DVM_REMOTE_FUNCTION, target, &probe.target) == DVM_REMOTE_OK,
          "target root publication");
    check(dvm_refcount_load_u(&D2H(target)->ref) == 2 && dvm_runtime_root_count(D2H(target)) == 2,
          "independent target root and strong reference");
    dvm_remote_managed_close(closed);
    check(dvm_remote_managed_publish(closed, DVM_REMOTE_VARIABLE, target, &untouched) == DVM_REMOTE_CLOSED &&
              untouched.session == 11 && untouched.generation == 22 && untouched.slot == 33 &&
              dvm_refcount_load_u(&D2H(target)->ref) == 2 && dvm_runtime_root_count(D2H(target)) == 2,
          "failed closed publish exact rollback");
    check(dvm_remote_managed_destroy(closed) == DVM_REMOTE_OK, "closed empty adapter cleanup");
    ptrdel(D2H(target));
    destroy(target);
    argument = c2string("argument-root", 13);
    ptradd(D2H(argument));
    check(dvm_remote_managed_publish(probe.adapter, DVM_REMOTE_VARIABLE, argument, &probe.argument) == DVM_REMOTE_OK,
          "variable root publication");
    check(dvm_remote_managed_publish(probe.adapter, DVM_REMOTE_VARIABLE, argument, &untouched) == DVM_REMOTE_CAPACITY,
          "root quota refuses additional export");
    ptrdel(D2H(argument));
    destroy(argument);
    check(pthread_barrier_init(&probe.admitted, NULL, REMOTE_PEERS + 1) == 0 &&
              pthread_barrier_init(&probe.complete, NULL, REMOTE_PEERS + 1) == 0,
          "fixture barriers");
    for (index = 0; index < REMOTE_PEERS; index++)
    {
        peers[index].probe = &probe;
        peers[index].id = index + 1;
        check(pthread_create(&threads[index], NULL, native_peer, &peers[index]) == 0, "native peer creation");
    }
    pthread_barrier_wait(&probe.admitted);
    check(!__atomic_load_n(&probe.failures, __ATOMIC_RELAXED), "native peer admission and role refusal");
    check(dvm_remote_managed_tick(probe.adapter, 100, 200) == DVM_REMOTE_CLOSED,
          "expired lease cannot renew while calls retain roots");
    dvm_remote_managed_close(probe.adapter);
    check(dvm_remote_managed_collect(probe.adapter, 2, &released) == DVM_REMOTE_OK && released == 0,
          "close preserves admitted roots");
    for (index = 0; index < 8; index++)
    {
        check(dvm_runtime_gc_concurrent_drain(), "rooted concurrent collector passes");
    }
    check(dvm_runtime_root_count(D2H(target)) == 1 && dvm_runtime_root_count(D2H(argument)) == 1 && target->len == 10 &&
              argument->len == 13 && !memcmp(target->Sascii, "proxy-root", 10) &&
              !memcmp(argument->Sascii, "argument-root", 13),
          "actual managed sources survive close and collection");
    pthread_barrier_wait(&probe.complete);
    for (index = 0; index < REMOTE_PEERS; index++)
    {
        check(pthread_join(threads[index], NULL) == 0, "native peer join");
    }
    check(!probe.failures, "native peer completion");
    DvmRemoteStatus status = dvm_remote_managed_collect(probe.adapter, 1, &released);
    while (status == DVM_REMOTE_CAPACITY && pressure < 32)
    {
        pressure++;
        check(released == 0 && dvm_remote_managed_destroy(probe.adapter) == DVM_REMOTE_BUSY &&
                  dvm_runtime_root_count(D2H(target)) == 1 && dvm_runtime_root_count(D2H(argument)) == 1,
              "refused retirement preserves both roots and adapter");
        check(dvm_remote_managed_publish(probe.adapter, DVM_REMOTE_VARIABLE, (void *)1, &untouched) ==
                  DVM_REMOTE_CAPACITY,
              "detached roots still consume export quota before heap access");
        check(dvm_runtime_gc_concurrent_drain(), "collector passes with detached root retained");
        status = dvm_remote_managed_collect(probe.adapter, 1, &released);
    }
    check(status == DVM_REMOTE_OK && released == 1, "one-root retirement budget");
    check(dvm_remote_managed_destroy(probe.adapter) == DVM_REMOTE_BUSY, "second root remains owned");
    check(dvm_remote_managed_collect(probe.adapter, 1, &released) == DVM_REMOTE_OK && released == 1,
          "final root transfer to concurrent Fast");
    check(dvm_remote_managed_destroy(probe.adapter) == DVM_REMOTE_OK, "adapter cleanup after all pins finish");
    pthread_barrier_destroy(&probe.admitted);
    pthread_barrier_destroy(&probe.complete);
    for (index = 0; index < 8; index++)
    {
        check(dvm_runtime_gc_concurrent_drain(), "final rooted retirement drain");
    }
    check(dvm_refcount_load_i(&Tstring.ref) == baseline, "String Type baseline restored");
    printf("REMOTEROOTS pressure=%u pending=0 owner_thread=exact\n", pressure);
    puts("PASS remote managed roots: 8 native peers, aliases, close, concurrent GC, rollback, exact Type baseline");
}
