"""Actual rooted HEADMP projection, source-list lifetime and guarded guest."""
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
#include "raise.h"
#include "shared_head.h"
#include "shared_mapped.h"
#include "managed_owner.h"
#include "root_batch.h"
#include "root_registry.h"
#include "gc_fields.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <stdlib.h>
#include <string.h>

static __thread void *root_blocks[64]; /* Test-only live metadata ledger, no runtime quota. */
static __thread unsigned int root_live; /* Actual reserved native blocks still owned. */
static __thread int root_depth; /* Only actual root reservation allocator calls. */
static __thread int active; /* Controller-only dedicated diagnostic. */
static __thread int fail_after = -1; /* Actual native preparation refusal. */
static __thread int resolver_error; /* Trusted fixture resolver error before data. */
static __thread int callback; /* One consuming old-owner callback. */
static __thread int release_error; /* Error after temporary list owner consumed. */
static List *node; /* Genuine cons allocation; never a forged Heap header. */
static String *value; /* Independent source owner, also retained by actual node. */
static void *old; /* Actual old destination owner, consumed exactly once. */
static Type *record; /* Actual dtype descriptor and separate caller owner. */
static Type *selected; /* Fixture resolver result, immutable through invocation. */
static void *source_cell; /* Borrowed unaligned native list slot. */
static void *target; /* Borrowed unaligned destination record. */
static WORD type_index; /* Native operand captured by the real driver. */
static unsigned char expected[3 * sizeof(void *)]; /* Independent exact head bytes. */
static DVM_GCWorkers *collectors; /* Actual runtime collector service. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers);
void __real_dvm_runtime_guarded_headmp(DVM_HeadTypeResolver resolver);
void __real_dvm_runtime_mapped_copy(const void *, void *, Type *, int);
void __real_destroy(void *object);
void __real_error(char *message);
void *__real_malloc(size_t bytes);
void *__real_calloc(size_t count, size_t bytes);
void __real_free(void *pointer);
DVM_RootBatch *__real_dvm_runtime_root_batch_create(size_t capacity);
int __real_dvm_runtime_root_batch_destroy(DVM_RootBatch **batch);
DVM_CellResult __real_dvm_managed_cells_begin(const void *, void *, DVM_CellAccess *);
int __real_dvm_cell_access_end(DVM_CellAccess *access);
int __real_dvm_runtime_root_batch_publish(DVM_RootBatch *, void *const *, size_t);
DVM_RefcountResult __real_dvm_refcount_retain_u(unsigned long *count);
DVM_RefcountResult __real_dvm_refcount_release_u(unsigned long *count);
int __real_dvm_gc_field_mark(int *, const int *, int *);


/* Stop before invalid owner/header/collector continuation, exact semantic 10. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL shared head: %s\n", message);
        exit(10);
    }
}


/* Observe attempts, not only allocation success. Fail each of seven stages. */
static int allocate(void)
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


/* Track real root-reservation allocations only, excluding the ordinary registry's
 * intentionally process-lifetime reusable nodes and unrelated VM/native storage. */
static void *remember(void *pointer)
{
    unsigned int index;
    if (active && root_depth && pointer)
    {
        for (index = 0; index < 64; index++)
        {
            if (!root_blocks[index])
            {
                root_blocks[index] = pointer;
                root_live++;
                return pointer;
            }
        }
        check(0, "native ledger test capacity");
    }
    return pointer;
}


/* Observe actual root preparation before any cell guard. The ledger owns no
 * object/reference itself and follows the real reservation's failure cleanup. */
DVM_RootBatch *__wrap_dvm_runtime_root_batch_create(size_t capacity)
{
    DVM_RootBatch *result;
    root_depth++;
    result = __real_dvm_runtime_root_batch_create(capacity);
    root_depth--;
    return result;
}


/* Native metadata leak control stops at a ledger baseline before any GC. */
int __wrap_dvm_runtime_root_batch_destroy(DVM_RootBatch **batch)
{
    if (active && CONTROL == 7 && batch && *batch)
    {
        *batch = NULL;
        return 1;
    }
    return __real_dvm_runtime_root_batch_destroy(batch);
}


/* Follow exact native frees on normal/error paths, regardless of allocator scope. */
void __wrap_free(void *pointer)
{
    unsigned int index;
    if (active && pointer)
    {
        for (index = 0; index < 64; index++)
        {
            if (root_blocks[index] == pointer)
            {
                root_blocks[index] = NULL;
                root_live--;
                break;
            }
        }
    }
    __real_free(pointer);
}


/* Native metadata allocation under the actual runtime linker interception. */
void *__wrap_malloc(size_t bytes)
{
    return remember(allocate() ? __real_malloc(bytes) : NULL);
}


/* Reservation arrays have the same no-allocation-under-guards requirement. */
void *__wrap_calloc(size_t count, size_t bytes)
{
    return remember(allocate() ? __real_calloc(count, bytes) : NULL);
}


/* A captured List count must be retained while its real source slot is guarded. */
DVM_RefcountResult __wrap_dvm_refcount_retain_u(unsigned long *count)
{
    if (active && count == &D2H(node)->ref)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()) > 0, "list source retain guarded");
        if (CONTROL == 2)
        {
            return DVM_REFCOUNT_LIVE;
        }
    }
    return __real_dvm_refcount_retain_u(count);
}


/* Detect skipped retains before the mark/rollback could consume an old owner. */
int __wrap_dvm_gc_field_mark(int *color, const int *selector, int *pending)
{
    if (active && color == &D2H(node)->color)
    {
        check(heapref(node) == 2, "list retain precedes mark");
    }
    return __real_dvm_gc_field_mark(color, selector, pending);
}


/* Failed mark control omits only the newly acquired count rollback. */
DVM_RefcountResult __wrap_dvm_refcount_release_u(unsigned long *count)
{
    if (active && CONTROL == 6 && count == &D2H(node)->ref)
    {
        return DVM_REFCOUNT_LIVE;
    }
    return __real_dvm_refcount_release_u(count);
}


/* Missing source claim must stop before its actual retain. */
DVM_CellResult __wrap_dvm_managed_cells_begin(const void *source, void *destination, DVM_CellAccess *access)
{
    static Inst *retry_pc; /* Decoded HEADMP PC at the first failed source claim. */
    static int replayed; /* The actual guest has reached admission again. */

    if (CONTROL == 0 && !active && !replayed &&
        dvm_native_execution()->arithmetic_retry_opcode == IHEADMP && source == destination)
    {
        if (!retry_pc)
        {
            retry_pc = DVM_R.PC;
            return DVM_CELL_BUSY;
        }
        check(retry_pc == DVM_R.PC && !dvm_native_context_object_count(dvm_native_context_current()),
            "HEADMP retries before retaining its source");
        replayed = 1;
        print("PASS scheduled HEADMP source retry\n");
    }

    if (active && CONTROL == 1)
    {
        return DVM_CELL_ACQUIRED;
    }
    return __real_dvm_managed_cells_begin(source, destination, access);
}


/* Missing unlock must be caught before original metadata validation. */
int __wrap_dvm_cell_access_end(DVM_CellAccess *access)
{
    if (active && CONTROL == 4)
    {
        return 1;
    }
    return __real_dvm_cell_access_end(access);
}


/* Omit only this list root in a private control, never arbitrary guest roots. */
int __wrap_dvm_runtime_root_batch_publish(DVM_RootBatch *batch, void *const *keys, size_t count)
{
    if (active && CONTROL == 3 && count == 1 && keys[0] == D2H(node))
    {
        return 1;
    }
    return __real_dvm_runtime_root_batch_publish(batch, keys, count);
}


/* The mapped payload path must inherit independent rooted container lifetime. */
void __wrap_dvm_runtime_mapped_copy(const void *source, void *destination, Type *type, int size)
{
    if (active)
    {
        check(dvm_runtime_root_count(D2H(node)) == 2 && heapref(node) == 2,
            "list rooted before payload work");
    }
    __real_dvm_runtime_mapped_copy(source, destination, type, size);
}


/* No propagated VM error may span the source-cell or payload guards. */
void __wrap_error(char *message)
{
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "error callback unlocked");
    }
    __real_error(message);
}


/* Trusted fixture resolver supplies a real dtype, not fake module metadata.
 * The separate compiled guest exercises the original module/index validator. */
static Type *resolve(void *module, const void *index)
{
    (void)module;
    check(!dvm_native_context_object_count(dvm_native_context_current()), "metadata callback unlocked");
    WORD captured;
    memmove(&captured, index, sizeof(captured));
    check(index != &type_index && captured == type_index, "index operand captured into a private slot");
    if (resolver_error)
    {
        error("head descriptor diagnostic");
    }
    return selected;
}


/* Consume old output, then remove its source list's original owner and run real
 * Cycles. The temporary hold alone must preserve the container and payload. */
void __wrap_destroy(void *object)
{
    int releasing = active && object == node && release_error;
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "retirement unlocked");
        if (object == node && CONTROL == 5)
        {
            return;
        }
    }
    __real_destroy(object);
    if (active && object == old && callback)
    {
        void *empty = H;
        DVM_HeapOwner *owner = dvm_native_owner_current();
        int index;
        callback = 0;
        check(!memcmp(target, expected, sizeof(expected)), "head published before callback");
        memmove(source_cell, &empty, sizeof(empty));
        ptrdel(D2H(node));
        destroy(node);
        check(dvm_runtime_root_count(D2H(node)) == 1 && heapref(node) == 1,
            "temporary list is sole rooted owner");
        check(dvm_native_owner_leave(owner), "callback leave for real Cycles");
        for (index = 0; index < 32; index++)
        {
            check(dvm_gc_workers_request(collectors, DVM_GC_CYCLES) &&
                dvm_gc_workers_wait_idle(collectors), "actual list payload Cycles");
        }
        check(dvm_native_owner_enter(owner) && !memcmp(node->data, expected, sizeof(expected)),
            "held list payload survives Cycles");
        DVM_R.s = H;
        DVM_R.d = H;
        DVM_R.m = H;
        release_error = 1;
        error("head consumed-output callback error");
    }
    if (releasing)
    {
        release_error = 0;
        error("head release diagnostic");
    }
}


/* Reset original borrowed operands for each real driver invocation. */
static int invoke(void *unused)
{
    (void)unused;
    DVM_R.s = source_cell;
    DVM_R.d = target;
    DVM_R.m = &type_index;
    __real_dvm_runtime_guarded_headmp(resolve);
    return 1;
}


/* Only compiled guest calls reach this wrapper; native fixture uses __real. */
void __wrap_dvm_runtime_guarded_headmp(DVM_HeadTypeResolver resolver)
{
    static int reached;
    __real_dvm_runtime_guarded_headmp(resolver);
    if (!reached++)
    {
        puts("PASS actual guarded HEADMP guest route");
    }
}


/* [GC:] Actual cons/dtype/incmem APIs initialize a genuine managed list. Native
 * byte views are unaligned caller-owned slots. All lifetimes end after Fast. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    REG saved;
    List *slot = H;
    unsigned char list_cell[sizeof(expected) + 2];
    unsigned char destination[sizeof(expected) + 2];
    unsigned char before[sizeof(expected)];
    unsigned char map = 0x80;
    WORD number = 731;
    Type *wrong;
    int strings;
    int lists;
    int index;
    int color;
    char previous[ERRMAX];
    __real_dvm_runtime_root_registry_probe(workers);
    memcpy(&saved, &DVM_R, sizeof(saved));
    memcpy(previous, getup()->env->errstr, sizeof(previous));
    strings = dvm_refcount_load_i(&Tstring.ref);
    lists = dvm_refcount_load_i(&Tlist.ref);
    collectors = workers;
    record = dtype(destroy, sizeof(expected), &map, 1);
    check(record != NULL, "real dtype");
    selected = record;
    value = c2string("head-data", 9);
    ptradd(D2H(value));
    memset(expected, 0xa5, sizeof(expected));
    memcpy(expected, &value, sizeof(value));
    memcpy(expected + sizeof(void *), &number, sizeof(number));
    node = cons(sizeof(expected), &slot);
    check(dvm_refcount_retain_i(&record->ref) == DVM_REFCOUNT_LIVE, "node descriptor owner");
    node->t = record;
    incmem(expected, record);
    memcpy(node->data, expected, sizeof(expected));
    ptradd(D2H(node));
    source_cell = list_cell + 1;
    memcpy(source_cell, &node, sizeof(node));
    old = c2string("head-old", 8);
    memset(destination, 0x5a, sizeof(destination));
    target = destination + 1;
    memcpy(target, &old, sizeof(old));
    memcpy(before, target, sizeof(before));
    active = 1;
    check(dvm_gc_field_load(&D2H(node)->color, &color) &&
        dvm_gc_field_store(&D2H(node)->color, -1), "inject list mark refusal");
    check(!kprotect(invoke, NULL), "actual mark error caught");
    check(dvm_gc_field_store(&D2H(node)->color, color), "restore list mark");
    check(heapref(node) == 1 && dvm_runtime_root_count(D2H(node)) == 1,
        "failed list hold rolls back extra owner");
    check(!root_live, "list native metadata baseline");
    for (index = 0; index < 7; index++)
    {
        fail_after = index;
        check(!kprotect(invoke, NULL) && fail_after == -1 && !memcmp(before, target, sizeof(before)),
            "seven preparation refusals preserve destination");
        check(!root_live && heapref(node) == 1 && dvm_runtime_root_count(D2H(node)) == 1 && heapref(value) == 2,
            "temporary owner baseline");
    }
    resolver_error = 1;
    release_error = 1;
    check(!kprotect(invoke, NULL) && strstr(getup()->env->errstr, "head descriptor diagnostic") &&
        heapref(node) == 1 && dvm_runtime_root_count(D2H(node)) == 1,
        "descriptor error survives release error");
    check(!release_error, "release error was actually exercised");
    resolver_error = 0;
    release_error = 0;
    wrong = dtype(destroy, sizeof(expected) + 4096, NULL, 0);
    check(wrong != NULL, "real oversized descriptor");
    selected = wrong;
    check(!kprotect(invoke, NULL) && !memcmp(before, target, sizeof(before)) && heapref(node) == 1,
        "physical list extent refusal cleans hold");
    selected = record;
    freetype(wrong);
    {
        unsigned char different_map = 0x40;
        wrong = dtype(destroy, sizeof(expected), &different_map, 1);
        check(wrong != NULL, "real incompatible map descriptor");
        selected = wrong;
        check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exType) &&
            !memcmp(before, target, sizeof(before)) && heapref(node) == 1,
            "map incompatibility refuses before mapped payload access");
        selected = record;
        freetype(wrong);
    }
    wrong = dtype(destroy, 0, NULL, 0);
    check(wrong != NULL, "real empty descriptor");
    selected = wrong;
    {
        void *original = target;
        target = H;
        check(kprotect(invoke, NULL) && heapref(node) == 1, "empty head ignores destination storage");
        release_error = 1;
        check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, "head release diagnostic") &&
            !release_error && heapref(node) == 1, "successful empty body propagates release error");
        target = original;
    }
    selected = record;
    freetype(wrong);
    {
        void *empty = H;
        resolver_error = 1;
        memcpy(source_cell, &empty, sizeof(empty));
        check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exNilref),
            "nil head precedes descriptor error");
        memcpy(source_cell, &node, sizeof(node));
        resolver_error = 0;
    }
    callback = 1;
    check(!kprotect(invoke, NULL) && strstr(getup()->env->errstr, "head consumed-output callback error") &&
        !memcmp(target, expected, sizeof(expected)) && !release_error &&
        destination[0] == 0x5a && destination[sizeof(destination) - 1] == 0x5a,
        "committed head and first callback error survive list release");
    check(!root_live, "list native metadata baseline");
    active = 0;
    check(dvm_native_owner_leave(dvm_native_owner_current()), "leave for node Fast");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) && dvm_gc_workers_wait_idle(workers), "actual node Fast");
    check(dvm_native_owner_enter(dvm_native_owner_current()) && heapref(value) == 2 &&
        dvm_refcount_load_i(&Tlist.ref) == lists && dvm_refcount_load_i(&record->ref) == 1,
        "node and descriptor baseline after Fast");
    freeptrs(target, record);
    selected = dtype(destroy, sizeof(expected), NULL, 0);
    check(selected != NULL, "real raw descriptor");
    slot = H;
    node = cons(sizeof(expected), &slot);
    memset(expected, 0x37, sizeof(expected));
    memcpy(node->data, expected, sizeof(expected));
    ptradd(D2H(node));
    memcpy(source_cell, &node, sizeof(node));
    active = 1;
    check(kprotect(invoke, NULL) && !memcmp(target, expected, sizeof(expected)) && heapref(node) == 1,
        "raw head projection with independent allocated extent");
    {
        unsigned char zero_map = 0;
        Type *plain = selected;
        selected = dtype(destroy, sizeof(expected), &zero_map, 1);
        check(selected != NULL && kprotect(invoke, NULL) && !memcmp(target, expected, sizeof(expected)),
            "zero-tail maps are compatible with raw list storage");
        freetype(selected);
        selected = plain;
    }
    check(!root_live, "list native metadata baseline");
    active = 0;
    ptrdel(D2H(node));
    destroy(node);
    freetype(selected);
    selected = record;
    slot = H;
    node = cons(sizeof(expected), &slot);
    check(dvm_refcount_retain_i(&record->ref) == DVM_REFCOUNT_LIVE, "aliased node descriptor owner");
    node->t = record;
    memcpy(expected, &value, sizeof(value));
    incmem(expected, record);
    memcpy(node->data, expected, sizeof(expected));
    ptradd(D2H(node));
    memcpy(source_cell, &node, sizeof(node));
    target = source_cell;
    active = 1;
    check(kprotect(invoke, NULL) && !memcmp(target, expected, sizeof(expected)),
        "source list slot may be replaced by projected record");
    check(!root_live, "list native metadata baseline");
    active = 0;
    ptrdel(D2H(node));
    freeptrs(target, record);
    ptrdel(D2H(value));
    destroy(value);
    freetype(record);
    check(dvm_native_owner_leave(dvm_native_owner_current()), "leave for final Fast");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) && dvm_gc_workers_wait_idle(workers), "final actual Fast");
    check(dvm_native_owner_enter(dvm_native_owner_current()) && dvm_refcount_load_i(&Tstring.ref) == strings &&
        dvm_refcount_load_i(&Tlist.ref) == lists, "source Type baseline");
    memcpy(&DVM_R, &saved, sizeof(saved));
    memcpy(getup()->env->errstr, previous, sizeof(previous));
    puts("PASS actual head projection: seven refusals,rollback,extent,sole-root Cycles,REG/error cleanup,Fast baselines");
}
'''


def main():
    """Run actual C/JIT and both polls, requiring seven safe semantic controls."""
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    markers = {1: 'list source retain guarded', 2: 'list retain precedes mark',
               3: 'list rooted before payload work', 4: 'error callback unlocked',
               5: 'temporary owner baseline', 6: 'failed list hold rolls back extra owner', 7: 'list native metadata baseline'}
    with tempfile.TemporaryDirectory(prefix='actual shared head ') as directory:
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
                            ['dvm_runtime_root_registry_probe', 'dvm_runtime_guarded_headmp',
                             'dvm_runtime_mapped_copy', 'destroy', 'error', 'malloc', 'calloc',
                             'dvm_managed_cells_begin', 'dvm_cell_access_end',
                             'dvm_runtime_root_batch_publish', 'dvm_runtime_root_batch_create',
                             'dvm_runtime_root_batch_destroy', 'free', 'dvm_refcount_retain_u',
                             'dvm_refcount_release_u', 'dvm_gc_field_mark'])
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
                        if run.returncode != 10 or 'FAIL shared head: '+markers[variant] not in output:
                            raise RuntimeError('head boundary control missed '+str(variant)+': '+output)
                    elif run.returncode or 'PASS actual head projection:' not in output or \
                            'PASS actual guarded HEADMP guest route' not in output or \
                            'PASS scheduled HEADMP source retry' not in output:
                        raise RuntimeError('head positive failed '+str(run.returncode)+': '+output)
    print('PASS actual shared head C/JIT,both polls'+
          (' positives only' if '--positive-only' in sys.argv else '; seven semantic boundary controls'))


if __name__ == '__main__':
    main()
