/* Copyright (c) 2026 bhgv
 * EN: Concurrent real-header Sys owner validation against atomic retain/drop.
 * RU: Конкурентная проверка Sys owners с реальным Heap и atomic retain/drop.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "sys_capture.h"
#include "disvm/refcount.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

char exNilref[] = "nil";        /* Mock error identity; production guard calls error. */
char exBounds[] = "bounds";     /* Mock frame-boundary error identity. */
char exHeap[] = "heap";         /* Mock owner-lifetime error identity. */
static Heap *source;            /* Native fixture with real Heap geometry; lives through join. */
static pthread_barrier_t start; /* Test-only rendezvous to overlap validation with mutation. */


/* EN/RU: Any guard error fails this live-owner fixture; message is borrowed.
 * Любая guard error завершает fixture; message заимствован. No VM callback. */
void error(char *message)
{
    fprintf(stderr, "FAIL concurrent Sys owner: %s\n", message);
    exit(1);
}


/* EN/RU: Mutate one spare native reference while two fixture owners stay live.
 * Изменять дополнительный native ref, пока два fixture owners живы. */
static void *change(void *unused)
{
    unsigned int index;
    (void)unused;
    pthread_barrier_wait(&start);
    for (index = 0; index < 200000; index++)
    {
        if (dvm_refcount_retain_u(&source->ref) != DVM_REFCOUNT_LIVE ||
            dvm_refcount_release_u(&source->ref) != DVM_REFCOUNT_LIVE)
        {
            error("unexpected counter transition");
        }
    }
    return NULL;
}


/* EN: Real header storage remains live until join; validate packed owner slots
 * and two aliases during overlapping mutation. No arbitrary root enters the GC.
 * RU: Реальный Heap живёт до join; проверять owner slot и два aliases во время
 * мутаций. Произвольные native roots не попадают в GC. */
int main(void)
{
    pthread_t writer;
    unsigned int index;
    void *value;
    void *owners[2];
    source = calloc(1, sizeof(*source) + sizeof(void *));
    if (!source || pthread_barrier_init(&start, NULL, 2) != 0)
    {
        return 1;
    }
    source->ref = 2;
    value = H2D(void *, source);
    owners[0] = value;
    owners[1] = value;
    if (pthread_create(&writer, NULL, change, NULL) != 0)
    {
        return 1;
    }
    pthread_barrier_wait(&start);
    for (index = 0; index < 200000; index++)
    {
        if (dvm_sys_owner(&value) != value)
        {
            error("owner identity changed");
        }
        dvm_sys_owners(owners, 2);
    }
    if (pthread_join(writer, NULL) != 0 || dvm_refcount_load_u(&source->ref) != 2)
    {
        return 1;
    }
    pthread_barrier_destroy(&start);
    free(source);
    puts("PASS concurrent Sys owner: 200000 validates and 200000 atomic retain/drop pairs");
    return 0;
}
