/* Native exchange history; no managed Heap or collector epoch is synthesized. */
#include "gc_fields.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WRITERS 8 /* Joined native writers share only one scalar and start gate. */
#define WRITES 2000 /* Each replacement has a globally unique positive token. */
#define TOKENS (WRITERS * WRITES) /* Bounded history owned until all writers join. */


/* Gate lifetime spans every native thread; history slots have one writer each. */
typedef struct History
{
    FVM_PortSync *gate; /* Private barrier, never nested with scalar registry lock. */
    int ready; /* Gate-protected arrival count. */
    int go; /* Gate-protected start notification. */
    int cell; /* All competing scalar accesses use GC field operations. */
    int old[TOKENS + 1]; /* Predecessor recorded at index equal to written token. */
} History;


/* Arguments are immutable after thread creation and borrowed through join. */
typedef struct Writer
{
    History *history; /* Shared fixture owner remains in main. */
    int number; /* Disjoint WRITES-token range for this writer. */
} Writer;


/* Exit at exact semantic10; compiler failures/signals are never evidence. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL GC exchange: %s\n", message);
        exit(10);
    }
}


/* Borrow argument through join; park only at a native start gate without locks.
 * Record each old value in a private slot. No VM payload/root is accessed. */
static void write_history(void *argument)
{
    Writer *writer = argument;
    History *history = writer->history;
    int step;
    fvm_port_sync_lock(history->gate);
    history->ready++;
    fvm_port_sync_signal_all(history->gate);
    while (!history->go)
    {
        fvm_port_sync_wait(history->gate);
    }
    fvm_port_sync_unlock(history->gate);
    for (step = 1; step <= WRITES; step++)
    {
        int token = writer->number * WRITES + step;
        check(dvm_gc_field_exchange(&history->cell, token, &history->old[token]), "worker exchange");
    }
}


/* Validate geometry and exact old/new semantics before unsafe concurrency.
 * Unique-token history must be one full chain, ending in original zero; this
 * rejects duplicate observations and lost replacements, not just a final value. */
int main(int argc, char **argv)
{
    History *history;
    Writer writers[WRITERS];
    FVM_PortThread *threads[WRITERS];
    unsigned char seen[TOKENS + 1] = {0};
    int expected[WRITERS];
    int cell = 7;
    int previous = 91;
    int final;
    int index;
    check(!dvm_gc_field_exchange(NULL, 0, &previous) && previous == 91,
        "null leaves output");
    check(!dvm_gc_field_exchange(&cell, 0, NULL) && cell == 7 &&
        !dvm_gc_field_exchange(&cell, 0, &cell) && cell == 7, "alias leaves cell");
    check(dvm_gc_field_exchange(&cell, 0, &previous) && previous == 7,
        "returns predecessor");
    check(cell == 0, "replacement visible");
    check(dvm_gc_field_store(&cell, 1) && dvm_gc_field_exchange(&cell, 0, &previous) &&
        previous == 1 && cell == 0, "pending consumed");
    check(dvm_gc_field_store(&cell, 1) && dvm_gc_field_load(&cell, &previous) &&
        previous == 1, "later publication survives");
    check(dvm_runtime_gc_exchange(&cell, 0) == 1 && cell == 0,
        "host wrapper consumes");
    if (argc > 1 && !strcmp(argv[1], "--serial-only"))
    {
        puts("PASS GC exchange serial boundaries");
        return 0;
    }
    history = calloc(1, sizeof(*history));
    check(history != NULL, "history allocation");
    history->gate = fvm_port_sync_create();
    check(history->gate != NULL, "start gate");
    for (index = 0; index < WRITERS; index++)
    {
        writers[index].history = history;
        writers[index].number = index;
        threads[index] = fvm_port_thread_start(write_history, &writers[index]);
        check(threads[index] != NULL, "native writer starts");
    }
    fvm_port_sync_lock(history->gate);
    while (history->ready != WRITERS)
    {
        fvm_port_sync_wait(history->gate);
    }
    history->go = 1;
    fvm_port_sync_signal_all(history->gate);
    fvm_port_sync_unlock(history->gate);
    for (index = 0; index < WRITERS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    check(dvm_gc_field_load(&history->cell, &final), "final scalar load");
    for (index = 0; index < WRITERS; index++)
    {
        expected[index] = WRITES;
    }
    for (index = 0; index < TOKENS; index++)
    {
        check(final > 0 && final <= TOKENS && !seen[final], "unique predecessor chain");
        check((final - 1) % WRITES + 1 == expected[(final - 1) / WRITES],
            "writer program order");
        expected[(final - 1) / WRITES]--;
        seen[final] = 1;
        final = history->old[final];
    }
    check(final == 0, "history reaches original value");
    fvm_port_sync_destroy(history->gate);
    free(history);
    puts("PASS GC exchange:8 OS writers,16000 unique replacements, complete linearized history");
    return 0;
}
