/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real ACTIVE owners overlap in a domain that rejects collector exclusion.
 * RU: Настоящие ACTIVE-владельцы пересекаются в домене с запретом эксклюзива GC.
 */
#include "disvm/native_world.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* [GC:] EN: Main owns this state until both native threads join.
 * RU: Состояние принадлежит main до join обоих нативных потоков. */
typedef struct Test
{
    DVM_NativeWorld *world; /* Borrowed domain until both joins. */
    FVM_PortSync *sync;     /* Fixture readiness only. */
    unsigned entered;       /* ACTIVE owners, under sync. */
    int release;            /* Main permits both owners to leave. */
} Test;


/* EN: Exit on false condition with borrowed message; no recovery.
 * RU: При ложном condition завершиться с заимствованным message без восстановления. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL concurrent world: %s\n", message);
        exit(12);
    }
}


/* [GC:] EN: Borrow argument through join; own a context and ACTIVE participant
 * until main releases both simultaneous owners. Never substitute a process.
 * RU: Заимствовать argument до join; владеть контекстом и ACTIVE-участником
 * до разрешения main обоим одновременным владельцам. Процесс не подменяется. */
static void owner(void *argument)
{
    Test *test = argument;
    DVM_NativeContext *context = dvm_native_context_create(sizeof(unsigned long), NULL);
    require(context && dvm_native_context_bind(context), "bound owner");
    DVM_WorldParticipant *participant = dvm_world_join(test->world, context);
    require(participant && dvm_world_enter(participant), "ordinary concurrent admission");
    fvm_port_sync_lock(test->sync);
    test->entered++;
    fvm_port_sync_signal_all(test->sync);
    while (!test->release)
    {
        fvm_port_sync_wait(test->sync);
    }
    fvm_port_sync_unlock(test->sync);
    require(dvm_world_leave(participant) && dvm_world_detach(participant), "owner detaches");
    require(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "owner context released");
}


/* [GC:] EN: Borrow CLI mode/poll; own domain and native threads. Negative stop
 * modes must terminate at the production guard with status 70 before mutation.
 * RU: Заимствовать режим/poll из CLI; владеть доменом и потоками. Отрицательные
 * stop-режимы должны завершиться кодом 70 на настоящей защите до изменений. */
int main(int argc, char **argv)
{
    require(argc == 3, "mode and polling arguments");
    Test test = {0};
    test.world = dvm_world_create_concurrent(atoi(argv[2]));
    test.sync = fvm_port_sync_create();
    require(test.world && test.sync && dvm_world_stops_forbidden(test.world), "immutable concurrent domain");
    if (strcmp(argv[1], "run"))
    {
        DVM_NativeContext *collector = dvm_native_context_create(sizeof(unsigned long), NULL);
        require(collector && dvm_native_context_bind(collector), "bound collector");
        if (!strcmp(argv[1], "stop"))
        {
            dvm_world_stop_begin(test.world, collector);
        }
        else
        {
            dvm_world_try_quiescent(test.world, collector);
        }
        require(0, "collector exclusion guard missing");
    }
    FVM_PortThread *first = fvm_port_thread_start(owner, &test);
    FVM_PortThread *second = fvm_port_thread_start(owner, &test);
    require(first && second, "both native threads started");
    fvm_port_sync_lock(test.sync);
    while (test.entered != 2)
    {
        fvm_port_sync_wait(test.sync);
    }
    DVM_WorldState state;
    require(dvm_world_state(test.world, &state) && state.active == 2 && !state.parked && !state.stopping &&
                !state.stopped,
            "two ACTIVE owners without exclusion");
    test.release = 1;
    fvm_port_sync_signal_all(test.sync);
    fvm_port_sync_unlock(test.sync);
    fvm_port_thread_join(first);
    fvm_port_thread_join(second);
    require(dvm_world_destroy(test.world), "world teardown");
    fvm_port_sync_destroy(test.sync);
    puts("PASS concurrent world: ACTIVE=2 parked=0 exclusive=forbidden");
    return 0;
}
