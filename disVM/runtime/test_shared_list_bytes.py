"""Actual scalar/raw list projection, allocation extents and rooted cleanup."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink
from test_list_bytes_tail import GUEST

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "isa.h"
#include "interp.h"
#include "pool.h"
#include "raise.h"
#include "shared_list_bytes.h"
#include "shared_bytes.h"
#include "list_hold.h"
#include "managed_owner.h"
#include "root_registry.h"
#include "gc_fields.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <fenv.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PAYLOAD 6000 /* Actual cons payload size, independent extent oracle. */
static __thread int active; /* Controller-only diagnostic. */
static __thread int fail_after = -1; /* Two actual root allocation refusals. */
static __thread int collect; /* One post-copy-return native GC/error injection. */
static __thread int release_error; /* Throw after consuming temporary source owner. */
static List *node; /* Genuine retained cons allocation. */
static void *source_cell; /* Native unaligned borrowed list-reference cell. */
static void *target; /* Native destination, independently writable. */
static const void *length_cell; /* Captured HEADM WORD operand. */
static WORD length; /* Actual native length value. */
static int opcode; /* One actual HEADB/W/L/F/M opcode. */
static size_t width; /* Independent expected byte-transfer width. */
static unsigned char wanted[PAYLOAD]; /* Independently generated expected bytes. */
static DVM_GCWorkers *collectors; /* Real collector service, alive through probe. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers);
void __real_dvm_runtime_list_bytes_copy(int opcode);
const char *__real_dvm_runtime_bytes_copy(const void *, void *, size_t);
const char *__real_dvm_runtime_list_hold(int, const void *, DVM_ListHold *);
void __real_dvm_runtime_byte_transfer(void *, const void *, size_t);
DVM_CellResult __real_dvm_managed_spans_begin(const void *, size_t, void *, size_t, DVM_CellAccess *);
DVM_CellResult __real_dvm_managed_ranges_begin(const void *, size_t, void *, size_t, DVM_RangeAccess *);
int __real_dvm_runtime_root_batch_publish(DVM_RootBatch *, void *const *, size_t);
DVM_RefcountResult __real_dvm_refcount_release_u(unsigned long *count);
ulong __real_poolmsize(Pool *pool, void *value);
void __real_destroy(void *value);
void __real_error(char *message);
void *__real_malloc(size_t bytes);
void *__real_calloc(size_t count, size_t bytes);


/* Explicit safe semantic stop, before bad copy/GC/count continuation. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL scalar list: %s\n", message);
        exit(10);
    }
}


/* Count allocation attempts outside guards; refuse each source-root stage. */
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


/* Actual native metadata allocator, never fabricated managed storage. */
void *__wrap_malloc(size_t bytes)
{
    return allocation() ? __real_malloc(bytes) : NULL;
}


/* Actual reserved root-array allocation boundary. */
void *__wrap_calloc(size_t count, size_t bytes)
{
    return allocation() ? __real_calloc(count, bytes) : NULL;
}


/* Missing container ownership must stop before the actual payload helper. */
const char *__wrap_dvm_runtime_list_hold(int opcode, const void *cell, DVM_ListHold *hold)
{
    if (active && CONTROL == 1)
    {
        hold->value = node;
        return NULL;
    }
    return __real_dvm_runtime_list_hold(opcode, cell, hold);
}


/* Real invalid-mark rollback control; never consume the original owner. */
DVM_RefcountResult __wrap_dvm_refcount_release_u(unsigned long *count)
{
    if (active && CONTROL == 7 && count == &D2H(node)->ref)
    {
        return DVM_REFCOUNT_LIVE;
    }
    return __real_dvm_refcount_release_u(count);
}


/* Missing source root is detected before any payload read. */
int __wrap_dvm_runtime_root_batch_publish(DVM_RootBatch *batch, void *const *keys, size_t count)
{
    if (active && CONTROL == 5 && count == 1 && keys[0] == D2H(node))
    {
        return 1;
    }
    return __real_dvm_runtime_root_batch_publish(batch, keys, count);
}


/* Wrong native allocation-size response must be caught by independent fixture
 * extent before a copy can touch bytes outside the actual cons payload. */
ulong __wrap_poolmsize(Pool *pool, void *value)
{
    if (active && CONTROL == 6 && value == D2H(node))
    {
        return (ulong)-1;
    }
    return __real_poolmsize(pool, value);
}


/* Validate exact requested small-byte spans; no model performs the claim. */
DVM_CellResult __wrap_dvm_managed_spans_begin(const void *source, size_t source_size,
    void *destination, size_t destination_size, DVM_CellAccess *access)
{
    if (active)
    {
        check(source == node->data && destination == target && source_size == width &&
            destination_size == width, "exact small payload spans");
        if (CONTROL == 2)
        {
            return DVM_CELL_ACQUIRED;
        }
    }
    return __real_dvm_managed_spans_begin(source, source_size, destination, destination_size, access);
}


/* Wide requests must use the full actual byte length in the same guard domain. */
DVM_CellResult __wrap_dvm_managed_ranges_begin(const void *source, size_t source_size,
    void *destination, size_t destination_size, DVM_RangeAccess *access)
{
    if (active && !destination)
    {
        check(source == length_cell && source_size == sizeof(WORD) && !destination_size,
            "exact guarded HEADM length operand");
    }
    else if (active)
    {
        check(source == node->data && destination == target && source_size == width &&
            destination_size == width, "exact wide payload spans");
        if (CONTROL == 2)
        {
            return DVM_CELL_ACQUIRED;
        }
    }
    return __real_dvm_managed_ranges_begin(source, source_size, destination, destination_size, access);
}


/* Actual transfer is reached only inside real payload guards. */
void __wrap_dvm_runtime_byte_transfer(void *destination, const void *source, size_t size)
{
    if (active)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()) > 0, "payload transfer guarded");
        if (CONTROL == 3)
        {
            return;
        }
    }
    __real_dvm_runtime_byte_transfer(destination, source, size);
}


/* Diagnostic post-copy return hook is not a production callback API. It forces
 * genuine GC while the new projection's native frame still owns its source hold. */
const char *__wrap_dvm_runtime_bytes_copy(const void *source, void *destination, size_t size)
{
    const char *problem;
    if (active)
    {
        check(heapref(node) == 2 && dvm_runtime_root_count(D2H(node)) == 2,
            "source container retained and rooted");
        check(size <= PAYLOAD, "fixture extent precedes copy");
    }
    problem = __real_dvm_runtime_bytes_copy(source, destination, size);
    if (active && !problem)
    {
        check(!memcmp(destination, wanted, size), "actual bytes published");
        check(!dvm_native_context_object_count(dvm_native_context_current()), "copy hook unlocked");
        if (collect)
        {
            DVM_HeapOwner *owner = dvm_native_owner_current();
            void *empty = H;
            int index;
            collect = 0;
            memcpy(source_cell, &empty, sizeof(empty));
            ptrdel(D2H(node));
            destroy(node);
            check(heapref(node) == 1 && dvm_runtime_root_count(D2H(node)) == 1,
                "temporary container is sole rooted owner");
            check(dvm_native_owner_leave(owner), "leave for real scalar Cycles");
            for (index = 0; index < 32; index++)
            {
                check(dvm_gc_workers_request(collectors, DVM_GC_CYCLES) &&
                    dvm_gc_workers_wait_idle(collectors), "actual scalar-list Cycles");
            }
            check(dvm_native_owner_enter(owner), "scalar-list callback reentry");
            DVM_R.s = H;
            DVM_R.d = H;
            DVM_R.m = H;
            release_error = 1;
            error("scalar-head completion diagnostic");
        }
    }
    return problem;
}


/* Temporary consumption must remain outside all access guards. */
void __wrap_destroy(void *object)
{
    int releasing = active && object == node && release_error;
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "retirement unlocked");
        if (object == node && CONTROL == 4)
        {
            return;
        }
    }
    __real_destroy(object);
    if (releasing)
    {
        release_error = 0;
        error("scalar-list cleanup diagnostic");
    }
}


/* Native exceptions never span access guards. */
void __wrap_error(char *message)
{
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "error unlocked");
    }
    __real_error(message);
}


/* Invoke actual driver with captured native operands, no fake Module/Type. */
static int invoke(void *unused)
{
    (void)unused;
    DVM_R.s = source_cell;
    DVM_R.d = target;
    DVM_R.m = (void *)length_cell;
    __real_dvm_runtime_list_bytes_copy(opcode);
    return 1;
}


/* Only actual guest opcodes reach this wrapper. */
void __wrap_dvm_runtime_list_bytes_copy(int operation)
{
    static int seen;
    __real_dvm_runtime_list_bytes_copy(operation);
    if (!seen++)
    {
        puts("PASS actual scalar-list guest route");
    }
}


/* [GC:] Actual cons allocation and native unaligned byte views, with exact bits,
 * canaries, overlap, error precedence, real post-copy Cycles and Fast baseline. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    REG saved;
    List *slot = H;
    unsigned char source[sizeof(void *) + 1];
    unsigned char destination[PAYLOAD + 1002];
    unsigned char oracle[PAYLOAD];
    uint64_t signaling = UINT64_C(0xfff0000000000001);
    int operations[5] = {IHEADB, IHEADW, IHEADL, IHEADF, IHEADM};
    size_t widths[5] = {sizeof(BYTE), sizeof(WORD), sizeof(LONG), sizeof(REAL), PAYLOAD};
    int baseline;
    int index;
    int color;
    fenv_t environment;
    char previous[ERRMAX];
    __real_dvm_runtime_root_registry_probe(workers);
    memcpy(&saved, &DVM_R, sizeof(saved));
    memcpy(previous, getup()->env->errstr, sizeof(previous));
    check(!fegetenv(&environment), "capture FP environment");
    baseline = dvm_refcount_load_i(&Tlist.ref);
    collectors = workers;
    node = cons(PAYLOAD, &slot);
    ptradd(D2H(node));
    for (index = 0; index < PAYLOAD; index++)
    {
        oracle[index] = (unsigned char)(index * 37 + (index >> 3) + 11);
    }
    memcpy(oracle, &signaling, sizeof(signaling));
    memcpy(node->data, oracle, sizeof(oracle));
    source_cell = source + 1;
    memcpy(source_cell, &node, sizeof(node));
    target = destination + 1;
    length_cell = &length;
    opcode = IHEADB;
    width = 1;
    active = 1;
    check(dvm_gc_field_load(&D2H(node)->color, &color) && dvm_gc_field_store(&D2H(node)->color, -1),
        "inject real source mark refusal");
    check(!kprotect(invoke, NULL), "actual source mark error caught");
    check(dvm_gc_field_store(&D2H(node)->color, color), "restore valid source mark");
    check(heapref(node) == 1 && dvm_runtime_root_count(D2H(node)) == 1,
        "source hold rollback baseline");
    for (index = 0; index < 2; index++)
    {
        fail_after = index;
        check(!kprotect(invoke, NULL) && fail_after == -1 && heapref(node) == 1,
            "root preparation refusal baseline");
    }
    for (index = 0; index < 5; index++)
    {
        opcode = operations[index];
        width = widths[index];
        length = (WORD)width;
        memcpy(wanted, oracle, width);
        memset(destination, 0x5a, sizeof(destination));
        check(!feclearexcept(FE_ALL_EXCEPT), "clear FP flags before bitwise copy");
        check(kprotect(invoke, NULL) && !memcmp(target, wanted, width) && destination[0] == 0x5a &&
            destination[width + 1] == 0x5a, "scalar/raw exact bits and canaries");
        check(!fetestexcept(FE_ALL_EXCEPT), "signaling NaN copy changes no FP flags");
        check(heapref(node) == 1 && dvm_runtime_root_count(D2H(node)) == 1,
            "temporary owner baseline");
    }
    opcode = IHEADM;
    length = PAYLOAD + 1000;
    check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exBounds) && heapref(node) == 1,
        "actual source extent refuses oversized raw copy");
    length = -1;
    target = H;
    check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exBounds),
        "negative length precedes raw destination");
    length = 0;
    check(kprotect(invoke, NULL) && heapref(node) == 1, "empty raw head ignores destination");
    release_error = 1;
    check(!kprotect(invoke, NULL) && !release_error &&
        !strcmp(getup()->env->errstr, "scalar-list cleanup diagnostic"), "successful empty release error");
    {
        void *empty = H;
        memcpy(source_cell, &empty, sizeof(empty));
        length = -1;
        check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exNilref),
            "raw nil source precedes length error");
        opcode = IHEADB;
        check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exBounds),
            "scalar destination precedes nil source");
        memcpy(source_cell, &node, sizeof(node));
    }
    opcode = IHEADM;
    target = destination + 1;
    length_cell = H;
    check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exBounds) && heapref(node) == 1,
        "invalid count cell cleans source hold");
    length_cell = &length;
    width = 97;
    length = (WORD)width;
    target = (uchar *)node->data + 1;
    memcpy(wanted, oracle, width);
    check(kprotect(invoke, NULL), "overlapping raw head copy");
    memmove(oracle + 1, oracle, width);
    check(!memcmp(node->data, oracle, sizeof(oracle)), "complete independent overlap oracle");
    target = destination + 1;
    width = 257;
    length = (WORD)width;
    memcpy(wanted, oracle, width);
    collect = 1;
    check(!kprotect(invoke, NULL) && !collect && !release_error &&
        strstr(getup()->env->errstr, "scalar-head completion diagnostic") &&
        !memcmp(target, wanted, width), "committed bytes and first error survive source cleanup");
    active = 0;
    check(dvm_native_owner_leave(dvm_native_owner_current()), "leave for scalar-list Fast");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) && dvm_gc_workers_wait_idle(workers),
        "actual scalar-list Fast");
    check(dvm_native_owner_enter(dvm_native_owner_current()) && dvm_refcount_load_i(&Tlist.ref) == baseline,
        "final real List Type baseline");
    check(!fesetenv(&environment), "restore FP environment");
    memcpy(&DVM_R, &saved, sizeof(saved));
    memcpy(getup()->env->errstr, previous, sizeof(previous));
    puts("PASS actual scalar-list projection: five widths,NaN,overlap,extent,errors,32 Cycles,Fast baseline");
}
'''


def main():
    """Execute actual C/JIT and both polls; semantic controls stop before harm."""
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    markers = {1: 'source container retained and rooted', 2: 'payload transfer guarded',
               3: 'actual bytes published', 4: 'temporary owner baseline',
               5: 'source container retained and rooted', 6: 'fixture extent precedes copy',
               7: 'source hold rollback baseline'}
    with tempfile.TemporaryDirectory(prefix='actual scalar list ') as directory:
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
                            ['dvm_runtime_root_registry_probe', 'dvm_runtime_list_bytes_copy',
                             'dvm_runtime_bytes_copy', 'dvm_runtime_list_hold', 'dvm_runtime_byte_transfer',
                             'dvm_managed_spans_begin', 'dvm_managed_ranges_begin',
                             'dvm_runtime_root_batch_publish', 'dvm_refcount_release_u',
                             'poolmsize', 'destroy', 'error', 'malloc', 'calloc'])
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
                        if run.returncode != 10 or 'FAIL scalar list: '+markers[variant] not in output:
                            raise RuntimeError('scalar-list control missed '+str(variant)+': '+output)
                    elif run.returncode or 'PASS actual scalar-list projection:' not in output or \
                            'PASS actual scalar-list guest route' not in output:
                        raise RuntimeError('scalar-list positive failed '+str(run.returncode)+': '+output)
    print('PASS actual scalar-list C/JIT,both polls'+
          (' positives only' if '--positive-only' in sys.argv else '; seven semantic controls'))


if __name__ == '__main__':
    main()
