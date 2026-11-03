/* Execute the actual retained movp operation against real managed ownership. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "heap_counts.h"
#include "disvm/native_owner.h"
#include "disvm/gc_workers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Exact semantic predicate; fail before invalid lifetime use. condition/message
 * borrowed. Compiler/signal/sanitizer/timeout failures cannot count as controls. */
static void check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL opcode-count probe: %s\n", message);
        exit(10);
    }
}


/* [GC:] workers borrows live Fast service; caller owns ACTIVE controller REG.
 * Real movp retains before replacing destination, including self-assignment.
 * Save/restore all private REG fields before any admission leave. Source keeps
 * one distinct owner until final destroy; no pointer read follows final drop. */
void dvm_runtime_opcode_probe(DVM_GCWorkers *workers)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    String *value = c2string("opcode-owner", 12);
    void *source = value;
    void *destination = H;
    REG saved;
    check(value != H && value != nil && heapref(value) == 1, "real movp source owner");
    memcpy(&saved, &DVM_R, sizeof(saved));
    DVM_R.s = &source;
    DVM_R.d = &destination;
    movp();
    check(destination == value && heapref(value) == 2, "movp retains source");
    DVM_R.s = &destination;
    movp();
    check(destination == value && heapref(value) == 2, "movp self-assignment preserves count");
    source = H;
    DVM_R.s = &source;
    movp();
    check(destination == H && heapref(value) == 1, "movp nil drops previous owner");
    memcpy(&DVM_R, &saved, sizeof(saved));
    destroy(value);
    value = H;
    check(dvm_native_owner_leave(owner), "opcode controller leaves for retirement");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers), "real opcode Fast retirement");
    check(dvm_native_owner_enter(owner), "opcode controller reentry");
    puts("PASS opcode-count probe: actual movp retain/self/nil, restored REG, real Fast");
}
