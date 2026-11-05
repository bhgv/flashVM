"""Actual LENA: continuous source guard, discovered field, real arrays and GC."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = '''implement ArrayLength;
include "sys.m";
ArrayLength: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN array length\\n");
    for (i := 0; i < 32; i++) {
        b := array[i] of byte;
        w := array[i] of int;
        r := array[i] of string;
        if (len b != i || len w != i || len r != i) {
            raise "array length mismatch";
        }
        if (i != 0) {
            at := i - 1;
            b[at] = byte i;
            w[at] = i + 7;
            r[at] = "guarded";
            wide := array[1] of big;
            floats := array[1] of real;
            zero := at - at;
            wide[zero] = big i;
            floats[zero] = real i;
            if (wide[zero] != big i || floats[zero] != real i) {
                raise "wide array index mismatch";
            }
            if (int b[at] != i || w[at] != i + 7 || r[at] != "guarded") {
                raise "array index mismatch";
            }
        }
    }
    n: array of byte;
    if (len n != 0) {
        raise "nil length mismatch";
    }
    sys->print("PASS array length32\\n");
}
'''

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "isa.h"
#include "interp.h"
#include "disvm/native_process.h"
#include "raise.h"
#include "shared_array_length.h"
#include "array_length_value.h"
#include "managed_owner.h"
#include "root_registry.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern void lena(void); /* Actual native opcode, not a modeled operation. */
static __thread int active; /* Only the serialized fixture controller observes helpers. */
static __thread unsigned long allocations; /* Native allocation/free calls inside LENA. */
static __thread unsigned int expected_depth; /* Independent six-key union size. */
static int address_publication; /* Observe only the private-to-shared publication call. */
const char *__real_dvm_runtime_bytes_publish_opcode(int, const void *, void *, size_t);
DVM_CellResult __real_dvm_managed_ranges_begin(const void *, size_t, void *, size_t, DVM_RangeAccess *);


/* Observe source/destination geometry inside the actual guard acquisition. */
DVM_CellResult __wrap_dvm_managed_ranges_begin(const void *source, size_t source_size,
    void *destination, size_t destination_size, DVM_RangeAccess *access)
{
    static Inst *busy_pc; /* First decoded index that reaches publication. */
    static int replayed; /* One matching address publication retry completed. */

    if (address_publication)
    {
        if (source || source_size || !destination || !destination_size)
        {
            fprint(2, "FAIL address publication acquired a private source guard\n");
            exit(10);
        }
        if (CONTROL == 0 && !replayed &&
            dvm_native_execution()->arithmetic_retry_opcode > 0)
        {
            if (!busy_pc)
            {
                busy_pc = DVM_R.PC;
                address_publication = 2;
                return DVM_CELL_BUSY;
            }
            if (busy_pc != DVM_R.PC)
            {
                fprint(2, "FAIL address publication replay changed PC\n");
                exit(10);
            }
            replayed = 1;
            print("PASS scheduled address publication retry\n");
        }
        address_publication = 2;
    }
    return __real_dvm_managed_ranges_begin(source, source_size, destination, destination_size, access);
}


/* Check each real address publication, independently of operand capture. */
const char *__wrap_dvm_runtime_bytes_publish_opcode(int opcode, const void *source, void *destination, size_t size)
{
    const char *problem;
    address_publication = 1;
    problem = __real_dvm_runtime_bytes_publish_opcode(opcode, source, destination, size);
    if (!problem && address_publication != 2)
    {
        fprint(2, "FAIL address publication skipped the destination guard\n");
        exit(10);
    }
    address_publication = 0;
    return problem;
}


static int index_reported; /* Emit one actual route marker per fixture process. */
const char *__real_dvm_runtime_array_index_operands(const void *, const void *, void *, void *, int *);


/* Verify that real C and compiled guests both enter the guarded index helper. */
const char *__wrap_dvm_runtime_array_index_operands(const void *source, const void *index,
    void *array_snapshot, void *index_snapshot, int *replay)
{
    const char *problem = __real_dvm_runtime_array_index_operands(source, index,
        array_snapshot, index_snapshot, replay);

    if (*replay)
    {
        print("PASS scheduled index retry\n");
    }
    if (!problem && !*replay && !index_reported)
    {
        index_reported = 1;
        print("PASS guarded array index route\n");
    }
    return problem;
}


static Array *array; /* Actual independently rooted fixture owner, or nil/H. */
static const void *source_cell; /* Complete borrowed reference slot. */
static void *output_cell; /* Complete borrowed writable WORD. */
static const void *field_cell; /* Expected live field or NULL for nil/H. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
DVM_CellResult __real_dvm_managed_small_three_begin(const void *, size_t, const void *, size_t, void *, size_t, DVM_TripleAccess *);
DVM_CellResult __real_dvm_small_range_access_extend(DVM_TripleAccess *, const void *, size_t);
void *__real_dvm_runtime_array_length_source(const void *);
void __real_dvm_runtime_array_length_value(const void *, void *);
void __real_error(char *);
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void __real_free(void *);


/* Count actual native allocation only inside the borrowed guarded operation. */
void *__wrap_malloc(size_t size)
{
    if (active)
    {
        allocations++;
    }
    return __real_malloc(size);
}


/* Count allocation requests without replacing libc storage or ownership. */
void *__wrap_calloc(size_t count, size_t size)
{
    if (active)
    {
        allocations++;
    }
    return __real_calloc(count, size);
}


/* Count retirement inside the operation; actual free remains untouched. */
void __wrap_free(void *value)
{
    if (active)
    {
        allocations++;
    }
    __real_free(value);
}


/* valid/message are an independent fixture predicate and borrowed diagnostic.
 * Semantic10 terminates before controlled defects can access unguarded fields. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL array length: %s\n", message);
        exit(10);
    }
}


/* Independent canonical boundary-key cardinality for up to three native spans.
 * Geometry is mapped by the fixture, not granted by this numeric calculation. */
static unsigned int union_size(const void *a, const void *b, const void *c)
{
    const void *slots[3] = {a, b, c};
    unsigned int keys[6];
    unsigned int count = 0;
    unsigned int index;
    for (index = 0; index < 3; index++)
    {
        unsigned int edge;
        if (!slots[index])
        {
            continue;
        }
        for (edge = 0; edge < 2; edge++)
        {
            uintptr_t address = (uintptr_t)slots[index]+(edge ? sizeof(void *)-1 : 0);
            unsigned int key = (unsigned int)((address/sizeof(void *))%256);
            unsigned int prior;
            for (prior = 0; prior < count && keys[prior] != key; prior++) { }
            if (prior == count)
            {
                keys[count++] = key;
            }
        }
    }
    return count;
}


/* First claim must exclude reference/output before any source pointer read. */
DVM_CellResult __wrap_dvm_managed_small_three_begin(const void *a, size_t an,
    const void *b, size_t bn, void *d, size_t dn, DVM_TripleAccess *access)
{
    static Inst *index_retry_pc; /* Guest index PC at the pre-effect refusal. */
    static int index_replayed; /* One matching decoded index resumed. */
    int opcode = dvm_native_execution()->arithmetic_retry_opcode;

    if (CONTROL == 0 && !active && !index_replayed &&
        (opcode == IINDX || opcode == IINDW || opcode == IINDF || opcode == IINDL || opcode == IINDB))
    {
        if (!index_retry_pc)
        {
            index_retry_pc = DVM_R.PC;
            return DVM_CELL_BUSY;
        }
        check(index_retry_pc == DVM_R.PC, "index retries the same decoded instruction");
        index_replayed = 1;
    }

    if (active)
    {
        check(a == source_cell && b == source_cell && d == output_cell &&
            an == sizeof(Array *) && bn == sizeof(Array *) && dn == sizeof(WORD), "initial spans");
        expected_depth = union_size(source_cell, output_cell, NULL);
        if (CONTROL == 1)
        {
            return DVM_CELL_ACQUIRED;
        }
    }
    return __real_dvm_managed_small_three_begin(a, an, b, bn, d, dn, access);
}


/* Actual capture happens under the initial guards, without changing real owners. */
void *__wrap_dvm_runtime_array_length_source(const void *source)
{
    if (active)
    {
        check(source == source_cell && dvm_native_context_object_count(dvm_native_context_current()) ==
            expected_depth && expected_depth, "source read guarded");
    }
    return __real_dvm_runtime_array_length_source(source);
}


/* Discover/claim the field while every original claim remains live. This hook
 * only inspects owned native metadata, never forges guard or Heap headers. */
DVM_CellResult __wrap_dvm_small_range_access_extend(DVM_TripleAccess *access, const void *field, size_t size)
{
    static Inst *retry_pc; /* Post-decode PC while extension is contended. */
    static int replayed; /* The guest retried the original length instruction. */

    if (CONTROL == 0 && !active && !replayed &&
        dvm_native_execution()->arithmetic_retry_opcode == ILENA)
    {
        if (!retry_pc)
        {
            retry_pc = DVM_R.PC;
            return DVM_CELL_BUSY;
        }
        check(retry_pc == DVM_R.PC, "length extension BUSY replays the same PC");
        replayed = 1;
        print("PASS scheduled length extension retry\n");
    }
    if (active)
    {
        check(field == field_cell && size == sizeof(WORD) && access->count == expected_depth &&
            access->held == expected_depth && dvm_native_context_object_count(dvm_native_context_current()) ==
            expected_depth, "continuous source prefix");
        expected_depth = union_size(source_cell, output_cell, field_cell);
        if (CONTROL == 2)
        {
            return DVM_CELL_ACQUIRED;
        }
        if (CONTROL == 4)
        {
            return DVM_CELL_BUSY;
        }
    }
    return __real_dvm_small_range_access_extend(access, field, size);
}


/* Guard depth independently accounts for the discovered field before any load.
 * Omitted extension is caught before reading an unexcluded field. */
void __wrap_dvm_runtime_array_length_value(const void *field, void *destination)
{
    if (active)
    {
        check(field == field_cell && destination == output_cell && expected_depth &&
            dvm_native_context_object_count(dvm_native_context_current()) == expected_depth, "field read guarded");
        if (array && array != H)
        {
            check(dvm_refcount_load_u(&D2H(array)->ref) == 1 &&
                dvm_runtime_root_count(D2H(array)) == 1, "no temporary owners or roots");
        }
        check(!dvm_native_owner_leave(dvm_native_owner_current()), "guards exclude world release");
        if (CONTROL == 3)
        {
            return;
        }
    }
    __real_dvm_runtime_array_length_value(field, destination);
}


/* Real error propagation must follow release of original and extended guards. */
void __wrap_error(char *message)
{
    check(!dvm_native_context_object_count(dvm_native_context_current()), "error after complete unlock");
    __real_error(message);
    abort();
}


/* Invoke the actual opcode through hosted kprotect, with genuine Proc labels. */
static int invoke(void *unused)
{
    (void)unused;
    lena();
    return 1;
}


/* array/source/output globals borrow independently kept-alive fixture storage.
 * Call once, then require unlocked state and zero allocation before returning. */
static int operation(void)
{
    int result;
    allocations = 0;
    DVM_R.s = (void *)source_cell;
    DVM_R.d = output_cell;
    active = 1;
    result = kprotect(invoke, NULL);
    active = 0;
    check(!allocations && !dvm_native_context_object_count(dvm_native_context_current()), "allocation-free interval");
    return result;
}


/* workers is a real service; all arrays/children use actual allocation and GC.
 * Native alias cells borrow a separately rooted owner. Restore REG before actual
 * collector intervals; no fabricated Heap or native-key root enters the GC. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    Type *types[4] = {&Tbyte, &Tword, &Tlong, &Tptr};
    REG saved;
    char message[ERRMAX];
    unsigned char bytes[80];
    unsigned char wanted[80];
    unsigned int kind;
    unsigned int length;
    unsigned int skew;
    unsigned long cases = 0;
    int delta;
    __real_dvm_runtime_root_registry_probe(workers);
    check(sizeof(WORD) == sizeof(void *), "native fixture width");
    memcpy(&saved, &DVM_R, sizeof(saved));
    memcpy(message, getup()->env->errstr, sizeof(message));
    for (kind = 0; kind < 4; kind++)
    {
        for (length = 0; length < 32; length++)
        {
            Heap *heap = heaparray(types[kind], (int)length);
            WORD expected = length;
            array = H2D(Array *, heap);
            ptradd(heap);
            field_cell = (unsigned char *)array+offsetof(Array, len);
            for (skew = 0; skew < 8; skew++)
            {
                for (delta = -7; delta <= 7; delta++)
                {
                    memset(bytes, 0xa5, sizeof(bytes));
                    source_cell = bytes+24+skew;
                    output_cell = bytes+24+skew+delta;
                    memcpy((void *)source_cell, &array, sizeof(array));
                    memcpy(wanted, bytes, sizeof(bytes));
                    if (CONTROL == 4)
                    {
                        check(!operation() && !strcmp(getup()->env->errstr, exHeap) &&
                            !memcmp(bytes, wanted, sizeof(bytes)) && dvm_refcount_load_u(&heap->ref) == 1 &&
                            dvm_runtime_root_count(heap) == 1, "extension refusal unchanged");
                        ptrdel(heap);
                        destroy(array);
                        goto finish;
                    }
                    memcpy(wanted+24+skew+delta, &expected, sizeof(expected));
                    check(operation() && !memcmp(bytes, wanted, sizeof(bytes)), "exact length and canaries");
                    cases++;
                }
            }
            source_cell = &array;
            output_cell = &array->len;
            check(operation() && array->len == expected, "field already in initial union");
            ptrdel(heap);
            destroy(array);
        }
    }
    check(cases == 15360, "all real typed array cases");
    for (kind = 0; kind < 2; kind++)
    {
        WORD zero = 0;
        array = kind ? H : NULL;
        field_cell = NULL;
        for (delta = -7; delta <= 7; delta++)
        {
            memset(bytes, 0xa5, sizeof(bytes));
            source_cell = bytes+24;
            output_cell = bytes+24+delta;
            memcpy((void *)source_cell, &array, sizeof(array));
            memcpy(wanted, bytes, sizeof(bytes));
            memcpy(wanted+24+delta, &zero, sizeof(zero));
            check(operation() && !memcmp(bytes, wanted, sizeof(bytes)), "nil value gives zero");
        }
    }
    {
        void *invalid[3] = {NULL, H, (void *)((uintptr)-1-2)};
        Array *valid = H;
        WORD output = 0x1234;
        for (kind = 0; kind < 3; kind++)
        {
            DVM_R.s = invalid[kind];
            DVM_R.d = &output;
            check(!kprotect(invoke, NULL) && output == 0x1234 &&
                !strcmp(getup()->env->errstr, exBounds), "bad source geometry");
            DVM_R.s = &valid;
            DVM_R.d = invalid[kind];
            check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exBounds), "bad output geometry");
        }
    }
    {
        Heap *heap = heaparray(&Tptr, 2);
        String *child;
        WORD output = -1;
        DVM_HeapOwner *owner = dvm_native_owner_current();
        array = H2D(Array *, heap);
        ptradd(heap);
        child = c2string("kept", 4);
        memcpy(array->data, &child, sizeof(child)); /* Transfer child initial owner before GC. */
        source_cell = &array;
        output_cell = &output;
        field_cell = &array->len;
        check(operation() && output == 2, "mapped-array field snapshot");
        memcpy(&DVM_R, &saved, sizeof(saved));
        check(dvm_native_owner_leave(owner), "leave after guard release");
        for (kind = 0; kind < 32; kind++)
        {
            check(dvm_gc_workers_request(workers, DVM_GC_CYCLES) && dvm_gc_workers_wait_idle(workers), "actual array Cycles");
        }
        check(dvm_native_owner_enter(owner) && array->len == 2 && child->len == 4 &&
            !memcmp(child->Sascii, "kept", 4) && dvm_refcount_load_u(&heap->ref) == 1 &&
            dvm_runtime_root_count(heap) == 1, "caller array and child survive");
        ptrdel(heap);
        destroy(array);
    }
finish:
    array = NULL;
    memcpy(&DVM_R, &saved, sizeof(saved));
    memcpy(getup()->env->errstr, message, sizeof(message));
    {
        DVM_HeapOwner *owner = dvm_native_owner_current();
        check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers, DVM_GC_FAST) &&
            dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner), "actual final Fast");
    }
    print("PASS array length probe:15360 real alias cases or explicit extension refusal\n");
}
'''


def main():
    """Use matching native objects, exact omission diagnostics and both polls."""
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='guarded array length ') as directory:
        work = Path(directory)
        source, image = work/'main.b', work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)], check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text(WRAPPER)
        for variant in ((0, 4) if '--positive-only' in sys.argv else (0, 1, 2, 3, 4)):
            binary = relink(runtime, work, wrapper, variant,
                ['dvm_runtime_root_registry_probe', 'dvm_managed_small_three_begin',
                 'dvm_small_range_access_extend', 'dvm_runtime_array_length_source',
                 'dvm_runtime_array_length_value', 'dvm_runtime_array_index_operands',
                 'dvm_runtime_bytes_publish_opcode', 'dvm_managed_ranges_begin', 'error', 'malloc', 'calloc', 'free'])
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
                        marker = {1: 'source read guarded', 2: 'field read guarded', 3: 'exact length and canaries'}[variant]
                        if run.returncode != 10 or 'FAIL array length: '+marker not in output:
                            raise RuntimeError('array-length control missed: '+output)
                    elif (run.returncode or 'PASS array length probe:' not in output or 'PASS array length32' not in output
                          or 'PASS guarded array index route' not in output):
                        raise RuntimeError('array-length positive failed: '+output)
                    if variant == 0 and ('PASS scheduled length extension retry' not in output or
                                         'PASS scheduled index retry' not in output or
                                         'PASS scheduled address publication retry' not in output):
                        raise RuntimeError('length extension retry missed: '+output)
    print('PASS actual LENA:15360 aliases,nil/geometry,caller-rooted Cycles/Fast,no allocations,both polls')


if __name__ == '__main__':
    main()
