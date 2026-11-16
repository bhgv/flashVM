/* Copyright (c) 2026 bhgv
 * EN: Multi-producer local journal generations, scoped updates and explicit external sequence interop.
 * RU: Multi-producer local journal generations, scoped updates и external sequence interop.
 */
#include "changes.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: One producer owns its output range; main retains the journal until both join.
 * Один producer владеет output range; main удерживает journal до join обоих. */
typedef struct Work
{
    DVM_Changes *journal; /* Borrowed main owner. */
    unsigned kinds; /* Independent producer class. */
    uint64_t values[20]; /* Exact allocated generations. */
} Work;


/* EN/RU: Checks remain active in every build.
 * Проверки активны во всех build modes. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "local journal append: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Allocate independent updates without comparing unrelated producer generations.
 * Выделить independent updates без сравнения unrelated producer generations. */
static void *run(void *opaque)
{
    Work *work = opaque;
    for (unsigned i = 0; i < 20; i++)
    {
        work->values[i] = changespost(work->journal, work->kinds, 0, 0);
    }
    return NULL;
}


/* EN/RU: Verify exact allocation, invalid input preservation and ordinary cursor acknowledgement.
 * Проверить exact allocation, invalid input preservation и обычный cursor acknowledgement. */
int main(void)
{
    DVM_Changes *journal = dvm_changes_create();
    check(journal != NULL, "journal");
    check(!changespost(NULL, 1, 0, 0) && !changespost(journal, 128, 0, 0) &&
          !changespost(journal, DVM_CHANGE_CONNECTION, DVM_CHANGE_RESOURCE, 7), "invalid append accepted");
    Work a = {journal, DVM_CHANGE_CONTENT, {0}};
    Work b = {journal, DVM_CHANGE_CONNECTION, {0}};
    pthread_t first, second;
    check(!pthread_create(&first, NULL, run, &a) && !pthread_create(&second, NULL, run, &b), "producers");
    check(!pthread_join(first, NULL) && !pthread_join(second, NULL), "join");
    unsigned long long seen = 0;
    for (unsigned i = 0; i < 20; i++)
    {
        check(a.values[i] >= 2 && a.values[i] <= 41 && b.values[i] >= 2 && b.values[i] <= 41, "allocation bounds");
        unsigned long long bit = 1ULL << a.values[i];
        check(!(seen & bit), "duplicate generation A");
        seen |= bit;
        bit = 1ULL << b.values[i];
        check(!(seen & bit), "duplicate generation B");
        seen |= bit;
    }
    DVM_ChangeCursor cursor = {0};
    DVM_ChangeBatch batch;
    check(dvm_changes_pending(journal, &cursor, DVM_CHANGE_ALL, &batch) == 1 && batch.event.generation == 41, "merged watermark");
    check(dvm_changes_ack(journal, &cursor, &batch) == FVM_OK, "merged ack");
    check(dvm_changes_reconnect(journal, 1) == 2 && dvm_changes_pending(journal, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
          dvm_changes_ack(journal, &cursor, &batch) == FVM_OK, "fresh epoch ack");
    check(changespost(journal, DVM_CHANGE_RIGHTS, DVM_CHANGE_RESOURCE, 0xfedcba9876543210ULL) == 2, "scoped append");
    check(dvm_changes_pending(journal, &cursor, DVM_CHANGE_ALL, &batch) == 1 && batch.event.kinds == DVM_CHANGE_RIGHTS &&
          batch.event.flags == DVM_CHANGE_RESOURCE && batch.event.resource == 0xfedcba9876543210ULL, "scoped refresh lost");
    DVM_ChangeEvent external = {2, 100, DVM_CHANGE_LOAD, 0, 0};
    check(dvm_changes_notify(journal, &external) == 1 && changespost(journal, DVM_CHANGE_CONTENT, 0, 0) == 101, "external sequence collision");
    dvm_changes_destroy(journal);
    puts("PASS local multi-producer journal generations");
    return 0;
}
