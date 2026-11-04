"""Actual guarded LENL: stable chains, Brent cycles, real roots and no allocation."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = '''implement ListLength;
include "sys.m";
ListLength: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN list length\\n");
    values: list of int;
    names: list of string;
    for (i := 0; i < 32; i++) {
        values = i :: values;
        names = "kept" :: names;
        if (len values != i+1 || len names != i+1) {
            raise "list length mismatch";
        }
    }
    empty: list of int;
    if (len empty != 0) {
        raise "empty list length";
    }
    sys->print("PASS list length32\\n");
}
'''

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "shared_list_length.h"
#include "list_length_value.h"
#include "managed_owner.h"
#include "root_registry.h"
#include "heap_counts.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern void lenl(void); /* Actual hosted instruction. */
static __thread int active; /* Serialized fixture controller observes helpers. */
static __thread unsigned long allocations; /* Native allocation/free inside LENL. */
static __thread unsigned int depth; /* Independent union of captured native fields. */
static __thread unsigned char selected[256]; /* Current canonical stripe membership. */
static __thread const void *next_cell; /* Source, then expected next reachable tail. */
static __thread unsigned long reads; /* Actual guarded pointer snapshots. */
static List *head; /* Real cons chain, independently caller-rooted while live. */
static const void *source_cell; /* Complete borrowed reference slot. */
static void *output_cell; /* Complete writable native WORD output. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
DVM_CellResult __real_dvm_managed_ranges_begin(const void *, size_t, void *, size_t, DVM_RangeAccess *);
DVM_CellResult __real_dvm_range_access_extend(DVM_RangeAccess *, const void *, size_t);
void *__real_dvm_runtime_list_length_pointer(const void *);
void __real_dvm_runtime_list_length_value(uint64_t, void *);
void __real_error(char *);
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void __real_free(void *);


/* Count direct native allocation in the guarded interval; forward libc malloc. */
void *__wrap_malloc(size_t size)
{
    if (active)
    {
        allocations++;
    }
    return __real_malloc(size);
}


/* Count allocator requests without replacing storage or ownership semantics. */
void *__wrap_calloc(size_t count, size_t size)
{
    if (active)
    {
        allocations++;
    }
    return __real_calloc(count, size);
}


/* Count retirement inside an operation; actual libc release remains unchanged. */
void __wrap_free(void *value)
{
    if (active)
    {
        allocations++;
    }
    __real_free(value);
}


/* Semantic10 before controlled missing guards can reach unsafe payload access.
 * valid/message are an independent predicate and borrowed diagnostic. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL list length: %s\n", message);
        exit(10);
    }
}


/* Borrow one pointer-sized mapped slot; update an independent numeric union.
 * This cardinality is not a provenance, mapping or managed-root capability. */
static void add_slot(const void *slot)
{
    unsigned int edge;
    for (edge = 0; edge < 2; edge++)
    {
        uintptr_t address = (uintptr_t)slot+(edge ? sizeof(void *)-1 : 0);
        unsigned int key = (unsigned int)((address/sizeof(void *))%256);
        if (!selected[key])
        {
            selected[key] = 1;
            depth++;
        }
    }
}


/* Initial source/output are excluded before the first pointer snapshot. */
DVM_CellResult __wrap_dvm_managed_ranges_begin(const void *source, size_t source_size,
    void *output, size_t output_size, DVM_RangeAccess *access)
{
    if (active)
    {
        check(source == source_cell && output == output_cell && source_size == sizeof(List *) &&
            output_size == sizeof(WORD), "initial source/output spans");
        memset(selected, 0, sizeof(selected));
        depth = 0;
        add_slot(source);
        add_slot(output);
        next_cell = source;
        if (CONTROL == 1)
        {
            return DVM_CELL_ACQUIRED;
        }
    }
    return __real_dvm_managed_ranges_begin(source, source_size, output, output_size, access);
}


/* Every discovered link is added without dropping the old guarded subset. */
DVM_CellResult __wrap_dvm_range_access_extend(DVM_RangeAccess *access, const void *field, size_t size)
{
    if (active)
    {
        check(field == next_cell && size == sizeof(List *) && access->held == depth &&
            access->count == depth && dvm_native_context_object_count(dvm_native_context_current()) == depth,
            "continuous visited subset");
        add_slot(field);
        if (CONTROL == 2)
        {
            return DVM_CELL_ACQUIRED;
        }
        if (CONTROL == 4)
        {
            return DVM_CELL_BUSY;
        }
    }
    return __real_dvm_range_access_extend(access, field, size);
}


/* Observe real source/tail reads only after exact independent guard depth.
 * No fixture model performs the payload snapshot or root/owner publication. */
void *__wrap_dvm_runtime_list_length_pointer(const void *cell)
{
    void *value;
    if (active)
    {
        check(cell == next_cell && depth &&
            dvm_native_context_object_count(dvm_native_context_current()) == depth, "pointer read guarded");
    }
    value = __real_dvm_runtime_list_length_pointer(cell);
    if (active)
    {
        reads++;
        next_cell = value && value != H ? (const unsigned char *)value+offsetof(List, tail) : NULL;
    }
    return value;
}


/* Entire visited chain remains excluded at scalar publication. Known fixture
 * ref/root counts check unchanged owners, never infer COW uniqueness. */
void __wrap_dvm_runtime_list_length_value(uint64_t count, void *destination)
{
    if (active)
    {
        check(destination == output_cell && depth &&
            dvm_native_context_object_count(dvm_native_context_current()) == depth, "publication guarded");
        if (head && head != H)
        {
            check(dvm_refcount_load_u(&D2H(head)->ref) == 1 &&
                dvm_runtime_root_count(D2H(head)) == 1, "unchanged caller owners and roots");
        }
        check(!dvm_native_owner_leave(dvm_native_owner_current()), "guards exclude world release");
        if (CONTROL == 3)
        {
            return;
        }
    }
    __real_dvm_runtime_list_length_value(count, destination);
}


/* Actual VM exception entry must have no original or added guards. */
void __wrap_error(char *message)
{
    check(!dvm_native_context_object_count(dvm_native_context_current()), "error after complete unlock");
    __real_error(message);
    abort();
}


/* Invoke one actual LENL through real Proc labels and native cleanup. */
static int invoke(void *unused)
{
    (void)unused;
    lenl();
    return 1;
}


/* Call once from current ACTIVE fixture; globals borrow independently live
 * slots. Return hosted status after proving zero allocation and guard depth. */
static int operation(void)
{
    int result;
    allocations = 0;
    reads = 0;
    DVM_R.s = (void *)source_cell;
    DVM_R.d = output_cell;
    active = 1;
    result = kprotect(invoke, NULL);
    active = 0;
    check(!allocations && !dvm_native_context_object_count(dvm_native_context_current()), "allocation-free interval");
    return result;
}


/* Own a real cons chain of length nodes. Root each new head before withdrawing
 * the prior head root; transfer its initial owner into the new tail. No fake
 * headers and no unrooted allocation boundary. Return one caller-rooted owner. */
static List *make_chain(unsigned int length)
{
    List *value = H;
    unsigned int index;
    for (index = 0; index < length; index++)
    {
        List *previous = value;
        List *next = cons(sizeof(WORD), &value);
        WORD payload = index;
        memmove(next->data, &payload, sizeof(payload));
        ptradd(D2H(next));
        if (previous != H)
        {
            ptrdel(D2H(previous));
        }
    }
    return value;
}


/* Consume one independently rooted fixture chain after all accesses end. */
static void drop_chain(List *value)
{
    if (value != H)
    {
        ptrdel(D2H(value));
        destroy(value);
    }
}


/* workers is the actual service. Own every real chain through complete access;
 * restore modified test topology before any collector interval or retirement. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    static const unsigned int sizes[] = {1, 2, 3, 7, 15, 16, 32, 63, 64, 127, 128, 255, 256, 511, 512}; /* Brent/hash boundaries. */
    REG saved;
    char message[ERRMAX];
    unsigned char bytes[80];
    unsigned char wanted[80];
    unsigned int length;
    unsigned int skew;
    unsigned int sample;
    unsigned long cases = 0;
    int delta;
    __real_dvm_runtime_root_registry_probe(workers);
    check(sizeof(WORD) == sizeof(void *), "native fixture width");
    memcpy(&saved, &DVM_R, sizeof(saved));
    memcpy(message, getup()->env->errstr, sizeof(message));
    for (length = 0; length < 32; length++)
    {
        WORD expected = length;
        head = make_chain(length);
        for (skew = 0; skew < 8; skew++)
        {
            for (delta = -7; delta <= 7; delta++)
            {
                memset(bytes, 0xa5, sizeof(bytes));
                source_cell = bytes+24+skew;
                output_cell = bytes+24+skew+delta;
                memcpy((void *)source_cell, &head, sizeof(head));
                memcpy(wanted, bytes, sizeof(bytes));
                if (CONTROL == 4 && length)
                {
                    check(!operation() && !strcmp(getup()->env->errstr, exHeap) &&
                        !memcmp(bytes, wanted, sizeof(bytes)) && dvm_refcount_load_u(&D2H(head)->ref) == 1 &&
                        dvm_runtime_root_count(D2H(head)) == 1, "extension refusal unchanged");
                    drop_chain(head);
                    goto finish;
                }
                memcpy(wanted+24+skew+delta, &expected, sizeof(expected));
                check(operation() && !memcmp(bytes, wanted, sizeof(bytes)) && reads == length+1,
                    "exact length and canaries");
                cases++;
            }
        }
        drop_chain(head);
    }
    check(cases == 3840, "all chain alias cases");
    for (sample = 0; sample < sizeof(sizes)/sizeof(sizes[0]); sample++)
    {
        WORD output = -1;
        List *last;
        head = make_chain(sizes[sample]);
        source_cell = &head;
        output_cell = &output;
        check(operation() && output == (WORD)sizes[sample] && reads == sizes[sample]+1,
            "long-chain boundaries");
        last = head;
        while (last->tail != H)
        {
            last = last->tail;
        }
        /* Add a genuine cycle owner, then restore it before any GC callback. */
        dvm_runtime_heap_retain(&D2H(head)->ref);
        last->tail = head;
        output = -1;
        check(!operation() && !strcmp(getup()->env->errstr, exHeap) && output == -1 &&
            reads < 4*sizes[sample]+4, "bounded Brent cycle refusal");
        last->tail = H;
        destroy(head); /* Consume only the now-unlinked extra cycle owner. */
        check(dvm_refcount_load_u(&D2H(head)->ref) == 1, "cycle owner restored");
        {
            List *kept = head->tail;
            if (kept != H)
            {
                ptradd(D2H(kept));
                head->tail = NULL; /* Transfer tail owner to this temporary C keeper. */
                check(!operation() && !strcmp(getup()->env->errstr, exNilref) && output == -1,
                    "nil tail diagnostic unchanged");
                head->tail = kept;
                ptrdel(D2H(kept)); /* Transfer owner back; do not destroy it. */
            }
        }
        drop_chain(head);
    }
    {
        List *bad = NULL;
        WORD output = 0x1234;
        source_cell = &bad;
        output_cell = &output;
        check(!operation() && !strcmp(getup()->env->errstr, exNilref) && output == 0x1234,
            "nil head remains error");
        DVM_R.s = H;
        DVM_R.d = &output;
        check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exBounds), "bad source slot");
        DVM_R.s = &bad;
        DVM_R.d = H;
        check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exBounds), "bad output slot");
    }
    head = make_chain(64);
    {
        DVM_HeapOwner *owner = dvm_native_owner_current();
        WORD output = -1;
        source_cell = &head;
        output_cell = &output;
        check(operation() && output == 64, "caller-rooted chain length");
        memcpy(&DVM_R, &saved, sizeof(saved));
        check(dvm_native_owner_leave(owner), "leave after complete LENL");
        for (sample = 0; sample < 32; sample++)
        {
            check(dvm_gc_workers_request(workers, DVM_GC_CYCLES) &&
                dvm_gc_workers_wait_idle(workers), "actual list Cycles");
        }
        check(dvm_native_owner_enter(owner) && operation() && output == 64 &&
            dvm_refcount_load_u(&D2H(head)->ref) == 1 && dvm_runtime_root_count(D2H(head)) == 1,
            "caller chain survives Cycles");
        drop_chain(head);
    }
finish:
    head = H;
    memcpy(&DVM_R, &saved, sizeof(saved));
    memcpy(getup()->env->errstr, message, sizeof(message));
    {
        DVM_HeapOwner *owner = dvm_native_owner_current();
        check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers, DVM_GC_FAST) &&
            dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner), "actual final Fast");
    }
    print("PASS list length probe:3840 aliases,real chains/cycles or explicit extension refusal\n");
}
'''


def main():
    """Use real matching objects, exact controls and both C/JIT poll modes."""
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='guarded list length ') as directory:
        work = Path(directory)
        source, image = work/'main.b', work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)], check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text(WRAPPER)
        for variant in ((0, 4) if '--positive-only' in sys.argv else (0, 1, 2, 3, 4)):
            binary = relink(runtime, work, wrapper, variant,
                ['dvm_runtime_root_registry_probe', 'dvm_managed_ranges_begin',
                 'dvm_range_access_extend', 'dvm_runtime_list_length_pointer',
                 'dvm_runtime_list_length_value', 'error', 'malloc', 'calloc', 'free'])
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    args = [str(binary), mode, '--gc-worker', '--guarded-copies', '--managed-owner-probe']
                    if locked:
                        args += ['--gc-locked-poll']
                    run = subprocess.run(args+[str(image)], capture_output=True, text=True, timeout=90,
                        env=dict(os.environ, TMPDIR=str(work)))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant in (1, 2, 3):
                        marker = 'exact length and canaries' if variant == 3 else 'pointer read guarded'
                        if run.returncode != 10 or 'FAIL list length: '+marker not in output:
                            raise RuntimeError('list-length control missed: '+output)
                    elif run.returncode or 'PASS list length probe:' not in output or 'PASS list length32' not in output:
                        raise RuntimeError('list-length positive failed: '+output)
    print('PASS actual LENL:3840 aliases,real chains/cycles,caller-rooted Cycles/Fast,no allocation,both polls')


if __name__ == '__main__':
    main()
