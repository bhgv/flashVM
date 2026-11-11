# Copyright (c) 2026 bhgv
"""EN: Actual mapped replacement, retained/rooted retirement and guarded MOVMP guest.
RU: Проверки копирования значений по карте ссылок.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink
from test_aggregate_tail import GUEST

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "isa.h"
#include "disvm/native_process.h"
#include "shared_mapped.h"
#include "managed_owner.h"
#include "root_batch.h"
#include "root_registry.h"
#include "gc_fields.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static __thread int active; /* Dedicated ACTIVE diagnostic interval only. */
static __thread int fail_after = -1; /* Native preparation allocation refusal. */
static __thread int retire_error; /* Throw once AFTER consuming a real old owner. */
static __thread int collecting; /* Scope callback-driven collector exercise. */
static __thread int retirement_checks; /* Enable destination/root retirement oracle. */
static String *first; /* Genuine independently rooted source owner. */
static String *second; /* Genuine independently rooted source owner. */
static void *old_first; /* Original outgoing value; never dereference after release. */
static void *old_second; /* Remaining outgoing value; rooted by the actual transaction. */
static unsigned char *target; /* Borrowed unaligned native destination bytes. */
static unsigned char expected[3 * sizeof(void *)]; /* Independent expected publication. */
static Type *record_type; /* Genuine dtype result, owned until diagnostic finish. */
static const void *input; /* Borrowed actual source bytes, valid through invoke. */
static DVM_GCWorkers *collectors; /* Borrowed real collectors through probe. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers);
void dvm_runtime_mapped_copy(const void *, void *, Type *, int);
void *__real_malloc(size_t bytes);
void *__real_calloc(size_t count, size_t bytes);
void __real_destroy(void *value);
void __real_error(char *message);
void __real_dvm_runtime_byte_transfer(void *destination, const void *source, size_t size);
DVM_CellResult __real_dvm_managed_ranges_begin(const void *, size_t, void *, size_t, DVM_RangeAccess *);
int __real_dvm_range_access_end(DVM_RangeAccess *access);
int __real_dvm_runtime_root_batch_publish(DVM_RootBatch *, void *const *, size_t);
int __real_dvm_runtime_root_batch_remove(DVM_RootBatch *, size_t);
DVM_RefcountResult __real_dvm_refcount_retain_u(unsigned long *value);
DVM_RefcountResult __real_dvm_refcount_release_u(unsigned long *value);


/* Stop at semantic 10 before invalid GC, count use or guarded callback. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL shared mapped: %s\n", message);
        exit(10);
    }
}


/* Monitor attempted allocation, including warmed metadata; inject each refusal. */
static int allocation(void)
{
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "allocation unlocked");
        if (fail_after == 0)
        {
            fail_after = -1;
            return 0;
        }
        if (fail_after > 0)
        {
            fail_after--;
        }
    }
    return 1;
}


/* Native preparation only; ordinary runtime allocations remain unchanged. */
void *__wrap_malloc(size_t bytes)
{
    return allocation() ? __real_malloc(bytes) : NULL;
}


/* Actual native reservation allocation, refused before ownership effects. */
void *__wrap_calloc(size_t count, size_t bytes)
{
    return allocation() ? __real_calloc(count, bytes) : NULL;
}


/* Source retain must occur inside a real complete range claim. */
DVM_RefcountResult __wrap_dvm_refcount_retain_u(unsigned long *value)
{
    if (active && (value == &D2H(first)->ref || value == &D2H(second)->ref))
    {
        check(dvm_native_context_object_count(dvm_native_context_current()) > 0, "source retain guarded");
    }
    return __real_dvm_refcount_retain_u(value);
}


/* Omit only actual rollback for a private control; stop before collector entry. */
DVM_RefcountResult __wrap_dvm_refcount_release_u(unsigned long *value)
{
    if (active && CONTROL == 2 && value == &D2H(second)->ref)
    {
        return DVM_REFCOUNT_LIVE;
    }
    return __real_dvm_refcount_release_u(value);
}


/* Private missing-claim control is detected before source count mutation. */
DVM_CellResult __wrap_dvm_managed_ranges_begin(const void *source, size_t source_size,
    void *destination, size_t destination_size, DVM_RangeAccess *access)
{
    static Inst *retry_pc[2]; /* Decoded PC for metadata and payload contention. */
    static int replayed[2]; /* One replay per acquisition stage. */
    int stage = destination != NULL;

    if (CONTROL == 0 && !active && !replayed[stage] &&
        dvm_native_execution()->arithmetic_retry_opcode == IMOVMP)
    {
        if (!retry_pc[stage])
        {
            retry_pc[stage] = DVM_R.PC;
            return DVM_CELL_BUSY;
        }
        check(retry_pc[stage] == DVM_R.PC && !dvm_native_context_object_count(dvm_native_context_current()),
            "MOVMP retries with no partial claim");
        replayed[stage] = 1;
        puts(stage ? "PASS scheduled MOVMP payload retry" : "PASS scheduled MOVMP descriptor retry");
    }

    if (active && CONTROL == 1)
    {
        return DVM_CELL_ACQUIRED;
    }
    return __real_dvm_managed_ranges_begin(source, source_size, destination, destination_size, access);
}


/* Private missing-unlock control must fail at the following error/callback. */
int __wrap_dvm_range_access_end(DVM_RangeAccess *access)
{
    if (active && CONTROL == 5)
    {
        return 1;
    }
    return __real_dvm_range_access_end(access);
}


/* Missing root publication is detected before the destination is modified. */
int __wrap_dvm_runtime_root_batch_publish(DVM_RootBatch *batch, void *const *keys, size_t count)
{
    if (active && CONTROL == 3)
    {
        return 1;
    }
    return __real_dvm_runtime_root_batch_publish(batch, keys, count);
}


/* Withdrawal must occur before each consuming destructor, not at batch end. */
int __wrap_dvm_runtime_root_batch_remove(DVM_RootBatch *batch, size_t index)
{
    if (active && CONTROL == 4)
    {
        return 1;
    }
    return __real_dvm_runtime_root_batch_remove(batch, index);
}


/* Observe the real single publication with displaced roots already visible. */
void __wrap_dvm_runtime_byte_transfer(void *destination, const void *source, size_t size)
{
    if (active && retirement_checks)
    {
        check(dvm_runtime_root_count(D2H(old_first)) == 1 &&
            dvm_runtime_root_count(D2H(old_second)) == 1, "displaced roots precede publication");
        check(dvm_native_context_object_count(dvm_native_context_current()) > 0,
            "publication guarded");
        if (CONTROL == 6)
        {
            return;
        }
    }
    __real_dvm_runtime_byte_transfer(destination, source, size);
}


/* Actual VM errors may propagate only after guard/native-preparation cleanup. */
void __wrap_error(char *message)
{
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "error callback unlocked");
    }
    __real_error(message);
}


/* [GC:] A consuming callback can trigger a real collector AFTER its current
 * owner was consumed. Other displaced owners must still be rooted. Native
 * expected-address comparisons never dereference an already released header. */
void __wrap_destroy(void *value)
{
    if (active && retirement_checks)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "retirement unlocked");
        check(!memcmp(target, expected, sizeof(expected)), "publication precedes retirement");
        check(!dvm_runtime_root_count(D2H(value)), "current root withdrawn before consume");
    }
    __real_destroy(value);
    if (active && collecting && value == old_second)
    {
        DVM_HeapOwner *owner = dvm_native_owner_current();
        int index;
        check(dvm_runtime_root_count(D2H(old_first)) == 1, "remaining owner stays rooted");
        check(dvm_native_owner_leave(owner), "retirement callback leaves ACTIVE");
        for (index = 0; index < 8; index++)
        {
            check(dvm_gc_workers_request(collectors, DVM_GC_CYCLES) &&
                dvm_gc_workers_wait_idle(collectors), "actual retirement callback Cycles");
        }
        check(dvm_native_owner_enter(owner), "retirement callback reentry");
        if (retire_error)
        {
            retire_error = 0;
            error("mapped consumed-owner callback error");
        }
    }
}


/* Dedicated actual helper checkpoint; errors are caught by production kprotect. */
static int invoke(void *unused)
{
    (void)unused;
    dvm_runtime_mapped_copy(input, target, record_type, sizeof(expected));
    return 1;
}


/* Guest uses actual adapted MOVMP, not this fixture's direct helper path. */
void __wrap_dvm_runtime_mapped_move_opcode(const void *source, void *destination, Type *type, int size)
{
    static int observed;
    extern void __real_dvm_runtime_mapped_move_opcode(const void *, void *, Type *, int);

    __real_dvm_runtime_mapped_move_opcode(source, destination, type, size);
    if (!observed++)
    {
        puts("PASS actual guarded MOVMP guest route");
    }
}


/* [GC:] Real dtype, Strings and collectors; native unaligned records borrow
 * source owners. No fake Heap/Module headers are constructed by this test. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    unsigned char source[sizeof(expected) + 1];
    unsigned char destination[sizeof(expected) + 1];
    unsigned char before[sizeof(expected)];
    unsigned char map = 0xc0;
    WORD scalar = 731;
    int baseline;
    int color;
    int index;
    char previous[ERRMAX];
    __real_dvm_runtime_root_registry_probe(workers);
    memcpy(previous, getup()->env->errstr, sizeof(previous));
    baseline = dvm_refcount_load_i(&Tstring.ref);
    collectors = workers;
    record_type = dtype(freeheap, sizeof(expected), &map, 1);
    check(record_type != NULL, "real mapped dtype");
    first = c2string("mapped-first", 12);
    second = c2string("mapped-second", 13);
    ptradd(D2H(first));
    ptradd(D2H(second));
    memset(source, 0xa5, sizeof(source));
    memset(destination, 0x5a, sizeof(destination));
    memcpy(source + 1, &first, sizeof(first));
    memcpy(source + 1 + sizeof(void *), &second, sizeof(second));
    memcpy(source + 1 + 2 * sizeof(void *), &scalar, sizeof(scalar));
    old_first = c2string("old-first", 9);
    old_second = c2string("old-second", 10);
    memcpy(destination + 1, &old_first, sizeof(old_first));
    memcpy(destination + 1 + sizeof(void *), &old_second, sizeof(old_second));
    input = source + 1;
    target = destination + 1;
    memcpy(expected, input, sizeof(expected));
    memcpy(before, target, sizeof(before));
    active = 1;
    for (index = 0; index < 5; index++)
    {
        fail_after = index;
        check(!kprotect(invoke, NULL) && fail_after == -1 && !memcmp(before, target, sizeof(before)) &&
            heapref(first) == 1 && heapref(second) == 1 &&
            !dvm_runtime_root_count(D2H(old_first)) && !dvm_runtime_root_count(D2H(old_second)),
            "preparation refusal preserves owners and destination");
    }
    check(dvm_gc_field_load(&D2H(first)->color, &color) &&
        dvm_gc_field_store(&D2H(first)->color, -1), "inject actual invalid mark");
    check(!kprotect(invoke, NULL), "mark refusal catches actual VM error");
    check(dvm_gc_field_store(&D2H(first)->color, color), "restore valid mark before further checks");
    check(heapref(first) == 1 && heapref(second) == 1 && !memcmp(before, target, sizeof(before)),
        "failed transaction rolls back retains");
    retirement_checks = 1;
    collecting = 1;
    retire_error = 1;
    index = kprotect(invoke, NULL);
    check(!index && strstr(getup()->env->errstr, "consumed-owner callback error") &&
        !memcmp(expected, target, sizeof(expected)) && heapref(first) == 2 && heapref(second) == 2,
        "committed copy drains remaining owners after callback error");
    retirement_checks = 0;
    collecting = 0;
    input = target;
    check(kprotect(invoke, NULL) && heapref(first) == 2 && heapref(second) == 2 &&
        destination[0] == 0x5a, "self alias and boundary canary");
    memcpy(source + 1 + sizeof(void *), &first, sizeof(first));
    input = source + 1;
    check(kprotect(invoke, NULL) && heapref(first) == 3 && heapref(second) == 1,
        "duplicate incoming and displaced owner counts");
    {
        void *empty = H;
        memcpy(source + 1, &empty, sizeof(empty));
        memcpy(source + 1 + sizeof(void *), &empty, sizeof(empty));
    }
    check(kprotect(invoke, NULL) && heapref(first) == 1 && heapref(second) == 1,
        "nil replacement retires duplicate outgoing occurrences");
    active = 0;
    freeptrs(target, record_type);
    ptrdel(D2H(first));
    ptrdel(D2H(second));
    destroy(first);
    destroy(second);
    freetype(record_type);
    check(dvm_native_owner_leave(dvm_native_owner_current()), "leave for final Fast");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) && dvm_gc_workers_wait_idle(workers), "final actual Fast");
    check(dvm_native_owner_enter(dvm_native_owner_current()) && dvm_refcount_load_i(&Tstring.ref) == baseline,
        "final source Type baseline");
    memcpy(getup()->env->errstr, previous, sizeof(previous));
    puts("PASS actual mapped replacement: OOM,rollback,rooted callback Cycles,error drain,self alias,Fast baseline");
}
'''


def main():
    """Run genuine hosted C/JIT guests and six boundary controls on both polls."""
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    markers = {1: 'source retain guarded', 2: 'failed transaction rolls back retains',
               3: 'displaced roots precede publication', 4: 'current root withdrawn before consume',
               5: 'error callback unlocked', 6: 'publication precedes retirement'}
    with tempfile.TemporaryDirectory(prefix='actual shared mapped ') as directory:
        work = Path(directory)
        source = work / 'main.b'
        image = work / 'main.dis'
        source.write_text(GUEST)
        built = subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)],
                               capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError(built.stdout+built.stderr)
        wrapper = work / 'wrapper.c'
        wrapper.write_text(WRAPPER)
        for variant in ([0] if '--positive-only' in sys.argv else [0, *markers]):
            binary = relink(runtime, work, wrapper, variant,
                            ['dvm_runtime_root_registry_probe', 'dvm_runtime_mapped_move_opcode', 'malloc', 'calloc',
                             'destroy', 'error', 'dvm_runtime_byte_transfer', 'dvm_managed_ranges_begin',
                             'dvm_range_access_end', 'dvm_runtime_root_batch_publish',
                             'dvm_runtime_root_batch_remove', 'dvm_refcount_retain_u', 'dvm_refcount_release_u'])
            for mode in ('--jit', '--no-jit'):
                for locked in (False, True):
                    command = [str(binary), mode, '--gc-worker', '--guarded-copies', '--managed-owner-probe']
                    if locked:
                        command += ['--gc-locked-poll']
                    run = subprocess.run(command+[str(image)], capture_output=True, text=True, timeout=45,
                                         env=dict(os.environ, TMPDIR=str(work)))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant:
                        if run.returncode != 10 or 'FAIL shared mapped: '+markers[variant] not in output:
                            raise RuntimeError('mapped boundary control missed '+str(variant)+': '+output)
                    elif run.returncode or 'PASS actual mapped replacement:' not in output or \
                            'PASS actual guarded MOVMP guest route' not in output or \
                            'PASS scheduled MOVMP payload retry' not in output or \
                            'PASS scheduled MOVMP descriptor retry' not in output:
                        raise RuntimeError('mapped positive failed '+str(run.returncode)+': '+output)
    print('PASS actual shared mapped C/JIT,both polls'+
          (' positives only' if '--positive-only' in sys.argv else '; six semantic boundary controls'))


if __name__ == '__main__':
    main()
