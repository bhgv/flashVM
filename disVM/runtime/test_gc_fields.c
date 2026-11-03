/* Native scalar-field stress; no managed object/root is forged by this test. */
#include "gc_fields.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORKERS 8 /* Independent native scalar writers, not admitted VM mutators. */
#define ROUNDS 2000 /* Mark repetitions by each joined OS writer. */


/* Native stack-owned scalar cells remain live through every join. */
typedef struct Fields
{
    int color; /* Shared scalar color cell, not a Heap header. */
    int selector; /* Immutable during all concurrent work. */
    int pending; /* Sticky flag, cleared only before creation/after joins. */
} Fields;


/* Exact pre-unsafe semantic stop; no signals or compiler failures accepted. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL GC fields: %s\n", message);
        exit(10);
    }
}


/* Native metadata only; protected Cells identity is borrowed through join. */
static void mark(void *argument)
{
    Fields *fields = argument;
    unsigned int index;
    for (index = 0; index < ROUNDS; index++)
    {
        int color;
        check(dvm_gc_field_mark(&fields->color, &fields->selector, &fields->pending), "worker mark");
        check(dvm_gc_field_load(&fields->color, &color) && color == 3, "propagator publication");
    }
}


/* Serial refusal/conditional behavior precedes threads so source controls stop
 * deterministically before unsafe work. No foreign pointer/header access. */
int main(int argc, char **argv)
{
    Fields fields = {2, 1, 0};
    FVM_PortThread *threads[WORKERS];
    unsigned int index;
    int result = 77;
    check(!dvm_gc_field_load(NULL, &result) && result == 77 &&
        !dvm_gc_field_load(&fields.color, &fields.color), "invalid load unchanged");
    check(!dvm_gc_field_store(NULL, 1) &&
        !dvm_gc_field_mark(&fields.color, &fields.selector, &fields.color), "aliased fields refuse");
    check(dvm_gc_field_store(&fields.color, 1) &&
        dvm_gc_field_mark(&fields.color, &fields.selector, &fields.pending) &&
        dvm_gc_field_load(&fields.color, &result) && result == 1 && fields.pending == 0,
        "mutator color stays unchanged");
    check(dvm_gc_field_store(&fields.color, 2) &&
        dvm_gc_field_mark(&fields.color, &fields.selector, &fields.pending) &&
        dvm_gc_field_load(&fields.color, &result) && result == 3, "mark sets propagator");
    check(dvm_gc_field_load(&fields.pending, &result) && result == 1, "mark publishes pending");
    fields.selector = 3;
    check(!dvm_gc_field_mark(&fields.color, &fields.selector, &fields.pending), "invalid selector refuses");
    fields.selector = 1;
    if (argc > 1 && !strcmp(argv[1], "--serial-only"))
    {
        puts("PASS GC scalar serial boundaries");
        return 0;
    }
    check(dvm_gc_field_store(&fields.pending, 0) && dvm_gc_field_store(&fields.color, 2), "native fixture reset");
    for (index = 0; index < WORKERS; index++)
    {
        threads[index] = fvm_port_thread_start(mark, &fields);
        check(threads[index] != NULL, "worker starts");
    }
    for (index = 0; index < WORKERS; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    check(dvm_gc_field_load(&fields.color, &result) && result == 3, "joined color");
    check(dvm_gc_field_load(&fields.pending, &result) && result == 1, "joined pending");
    puts("PASS GC scalar fields:8 joined OS writers,16000 marks, sticky flag");
    return 0;
}
