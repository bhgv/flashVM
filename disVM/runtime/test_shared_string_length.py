"""Actual LENC: continuous source guard, discovered field, real strings and GC."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = '''implement StringLength;
include "sys.m";
StringLength: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN string length\\n");
    ascii := "";
    unicode := "";
    for (i := 0; i < 32; i++) {
        if (len ascii != i || len unicode != i) {
            raise "string length mismatch";
        }
        ascii += "a";
        unicode += "Ж";
    }
    n: string;
    if (len n != 0) {
        raise "empty string length mismatch";
    }
    sys->print("PASS string length32\\n");
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
#include "shared_string_length.h"
#include "string_length_value.h"
#include "managed_owner.h"
#include "root_registry.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern void lenc(void); /* Actual native opcode, not a modeled operation. */
static __thread int active; /* Only the serialized fixture controller observes helpers. */
static __thread unsigned long allocations; /* Native allocation/free calls inside LENC. */
static __thread unsigned int expected_depth; /* Independent six-key union size. */
static String *string; /* Actual independently rooted fixture owner, or NULL/H. */
static const void *source_cell; /* Complete borrowed reference slot. */
static void *output_cell; /* Complete borrowed writable WORD. */
static const void *field_cell; /* Expected live field or NULL for nil/H. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
DVM_CellResult __real_dvm_managed_small_three_begin(const void *, size_t, const void *, size_t, void *, size_t, DVM_TripleAccess *);
DVM_CellResult __real_dvm_small_range_access_extend(DVM_TripleAccess *, const void *, size_t);
void *__real_dvm_runtime_string_length_source(const void *);
int __real_dvm_runtime_string_length_value(const void *, void *);
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
        fprint(2, "FAIL string length: %s\n", message);
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
            size_t width = index == 2 ? sizeof(int) : sizeof(void *);
            uintptr_t address = (uintptr_t)slots[index]+(edge ? width-1 : 0);
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
    if (active)
    {
        check(a == source_cell && b == source_cell && d == output_cell &&
            an == sizeof(String *) && bn == sizeof(String *) && dn == sizeof(WORD), "initial spans");
        expected_depth = union_size(source_cell, output_cell, NULL);
        if (CONTROL == 1)
        {
            return DVM_CELL_ACQUIRED;
        }
    }
    return __real_dvm_managed_small_three_begin(a, an, b, bn, d, dn, access);
}


/* Actual capture happens under the initial guards, without changing real owners. */
void *__wrap_dvm_runtime_string_length_source(const void *source)
{
    if (active)
    {
        check(source == source_cell && dvm_native_context_object_count(dvm_native_context_current()) ==
            expected_depth && expected_depth, "source read guarded");
    }
    return __real_dvm_runtime_string_length_source(source);
}


/* Discover/claim the field while every original claim remains live. This hook
 * only inspects owned native metadata, never forges guard or Heap headers. */
DVM_CellResult __wrap_dvm_small_range_access_extend(DVM_TripleAccess *access, const void *field, size_t size)
{
    static Inst *retry_pc; /* Post-decode PC while extension is contended. */
    static int replayed; /* The guest retried the original length instruction. */

    if (CONTROL == 0 && !active && !replayed &&
        dvm_native_execution()->arithmetic_retry_opcode == ILENC)
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
        check(field == field_cell && size == sizeof(int) && access->count == expected_depth &&
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
int __wrap_dvm_runtime_string_length_value(const void *field, void *destination)
{
    if (active)
    {
        check(field == field_cell && destination == output_cell && expected_depth &&
            dvm_native_context_object_count(dvm_native_context_current()) == expected_depth, "field read guarded");
        if (string && string != H)
        {
            check(dvm_refcount_load_u(&D2H(string)->ref) == 1 &&
                dvm_runtime_root_count(D2H(string)) == 1, "no temporary owners or roots");
        }
        check(!dvm_native_owner_leave(dvm_native_owner_current()), "guards exclude world release");
        if (CONTROL == 3)
        {
            return 1;
        }
    }
    return __real_dvm_runtime_string_length_value(field, destination);
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
    lenc();
    return 1;
}


/* string/source/output globals borrow independently kept-alive fixture storage.
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


/* workers is a real service; all strings use actual allocation and GC.
 * Native alias cells borrow a separately rooted ASCII/Rune owner. Restore REG before actual
 * collector intervals; no fabricated Heap or native-key root enters the GC. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
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
    check(sizeof(WORD) == sizeof(void *) && sizeof(int) == 4, "native fixture widths");
    memcpy(&saved, &DVM_R, sizeof(saved));
    memcpy(message, getup()->env->errstr, sizeof(message));
    for (kind = 0; kind < 2; kind++)
    {
        for (length = 0; length < 32; length++)
        {
            Heap *heap;
            WORD expected = length;
            string = kind ? newrunes((int)length) : newstring((int)length);
            heap = D2H(string);
            ptradd(heap);
            field_cell = (unsigned char *)string+offsetof(String, len);
            for (skew = 0; skew < 8; skew++)
            {
                for (delta = -7; delta <= 7; delta++)
                {
                    memset(bytes, 0xa5, sizeof(bytes));
                    source_cell = bytes+24+skew;
                    output_cell = bytes+24+skew+delta;
                    memcpy((void *)source_cell, &string, sizeof(string));
                    memcpy(wanted, bytes, sizeof(bytes));
                    if (CONTROL == 4)
                    {
                        check(!operation() && !strcmp(getup()->env->errstr, exHeap) &&
                            !memcmp(bytes, wanted, sizeof(bytes)) && dvm_refcount_load_u(&heap->ref) == 1 &&
                            dvm_runtime_root_count(heap) == 1, "extension refusal unchanged");
                        ptrdel(heap);
                        destroy(string);
                        goto finish;
                    }
                    memcpy(wanted+24+skew+delta, &expected, sizeof(expected));
                    check(operation() && !memcmp(bytes, wanted, sizeof(bytes)), "exact length and canaries");
                    cases++;
                }
            }
            /* Result aliases the int field and adjacent max: snapshot first and
             * restore both real fields before destructor/collector observation. */
            {
                int old_length = string->len;
                int old_max = string->max;
                WORD output;
                source_cell = &string;
                output_cell = &string->len;
                check(operation(), "field already in initial union");
                memcpy(&output, &string->len, sizeof(output));
                check(output == expected, "field alias normalized WORD");
                string->len = old_length;
                string->max = old_max;
            }
            ptrdel(heap);
            destroy(string);
        }
    }
    check(cases == 7680, "all real ASCII and Rune alias cases");
    for (kind = 0; kind < 2; kind++)
    {
        WORD zero = 0;
        string = kind ? H : NULL;
        field_cell = NULL;
        for (delta = -7; delta <= 7; delta++)
        {
            memset(bytes, 0xa5, sizeof(bytes));
            source_cell = bytes+24;
            output_cell = bytes+24+delta;
            memcpy((void *)source_cell, &string, sizeof(string));
            memcpy(wanted, bytes, sizeof(bytes));
            if (kind)
            {
                memcpy(wanted+24+delta, &zero, sizeof(zero));
                check(operation() && !memcmp(bytes, wanted, sizeof(bytes)), "H gives zero");
            }
            else
            {
                check(!operation() && !strcmp(getup()->env->errstr, exNilref) &&
                    !memcmp(bytes, wanted, sizeof(bytes)), "NULL unchanged Nilref");
            }
        }
    }
    {
        void *invalid[3] = {NULL, H, (void *)((uintptr)-1-2)};
        String *valid = H;
        WORD output = 0x1234;
        for (kind = 0; kind < 3; kind++)
        {
            DVM_R.s = invalid[kind];
            DVM_R.d = &output;
            check(!kprotect(invoke, NULL) && output == 0x1234 &&
                !strcmp(getup()->env->errstr, exHeap), "bad source geometry");
            DVM_R.s = &valid;
            DVM_R.d = invalid[kind];
            check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exHeap), "bad output geometry");
        }
        valid = (String *)((uintptr)-1-1);
        DVM_R.s = &valid;
        DVM_R.d = &output;
        check(!kprotect(invoke, NULL) && output == 0x1234 &&
            !strcmp(getup()->env->errstr, exHeap), "wrapped field refused");
    }
    {
        Heap *heap;
        int original;
        WORD output = 0x1234;
        string = newrunes(4);
        heap = D2H(string);
        ptradd(heap);
        original = string->len;
        field_cell = &string->len;
        source_cell = &string;
        output_cell = &output;
        string->len = (-0x7fffffff-1);
        check(!operation() && output == 0x1234 &&
            !strcmp(getup()->env->errstr, exHeap), "INT_MIN refused before publication");
        string->len = original;
        ptrdel(heap);
        destroy(string);
    }
    {
        Heap *heap;
        WORD output = -1;
        DVM_HeapOwner *owner = dvm_native_owner_current();
        string = c2string("kept", 4);
        heap = D2H(string);
        ptradd(heap);
        source_cell = &string;
        output_cell = &output;
        field_cell = &string->len;
        check(operation() && output == 4, "rooted string field snapshot");
        memcpy(&DVM_R, &saved, sizeof(saved));
        check(dvm_native_owner_leave(owner), "leave after guard release");
        for (kind = 0; kind < 32; kind++)
        {
            check(dvm_gc_workers_request(workers, DVM_GC_CYCLES) && dvm_gc_workers_wait_idle(workers), "actual string Cycles");
        }
        check(dvm_native_owner_enter(owner) && string->len == 4 &&
            !memcmp(string->Sascii, "kept", 4) && dvm_refcount_load_u(&heap->ref) == 1 &&
            dvm_runtime_root_count(heap) == 1, "caller string survives");
        check(operation() && output == 4, "post-Cycles guarded length");
        ptrdel(heap);
        destroy(string);
    }
finish:
    string = NULL;
    memcpy(&DVM_R, &saved, sizeof(saved));
    memcpy(getup()->env->errstr, message, sizeof(message));
    {
        DVM_HeapOwner *owner = dvm_native_owner_current();
        check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers, DVM_GC_FAST) &&
            dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner), "actual final Fast");
    }
    print("PASS string length probe:7680 real alias cases or explicit extension refusal\n");
}
'''


def main():
    """Use matching native objects, exact omission diagnostics and both polls."""
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='guarded string length ') as directory:
        work = Path(directory)
        source, image = work/'main.b', work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)], check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text(WRAPPER)
        for variant in ((0, 4) if '--positive-only' in sys.argv else (0, 1, 2, 3, 4)):
            binary = relink(runtime, work, wrapper, variant,
                ['dvm_runtime_root_registry_probe', 'dvm_managed_small_three_begin',
                 'dvm_small_range_access_extend', 'dvm_runtime_string_length_source',
                 'dvm_runtime_string_length_value', 'error', 'malloc', 'calloc', 'free'])
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
                        if run.returncode != 10 or 'FAIL string length: '+marker not in output:
                            raise RuntimeError('string-length control missed: '+output)
                    elif run.returncode or 'PASS string length probe:' not in output or 'PASS string length32' not in output:
                        raise RuntimeError('string-length positive failed: '+output)
                    if variant == 0 and 'PASS scheduled length extension retry' not in output:
                        raise RuntimeError('length extension retry missed: '+output)
    print('PASS actual LENC:7680 aliases,nil/geometry,caller-rooted Cycles/Fast,no allocations,both polls')


if __name__ == '__main__':
    main()
