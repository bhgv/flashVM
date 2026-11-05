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
Pair: adt { first, second: int; };
fresh(): array of Pair {
    return array[] of {Pair(19, 23)};
}
sparse(value: int): int {
    case value {
    -42 => return 11;
    100 to 105 => return 22;
    * => return 33;
    }
}
large(value: big): int {
    case value {
    big 9007199254740993 => return 19;
    big -9007199254740993 => return 23;
    * => return 0;
    }
}
strings(value: string): int {
    case value {
    "2.5" => return 29;
    "Ж" => return 31;
    * => return 0;
    }
}
dense(value: int): int {
    case value {
    0 => return 2;
    1 => return 5;
    2 => return 11;
    3 => return 17;
    4 => return 23;
    5 => return 31;
    6 => return 41;
    7 => return 47;
    * => return 97;
    }
}
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
            pair := fresh()[zero];
            if (pair.first != 19 || pair.second != 23) {
                raise "temporary array index lost its owner";
            }
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
    source := array[] of {"left", "middle", "right"};
    copy := source[-2:-1];
    copy[-1] = "copy";
    source[-2:] = array[] of {"new", "tail"};
    if (len copy != 2 || copy[-1] != "copy" || source[-2] != "new" || source[-1] != "tail") {
        raise "guarded mapped slice mismatch";
    }
    mapped := array[] of {"a", "b", "c", "d"};
    mapped[1:] = mapped[0:3];
    if (mapped[0] != "a" || mapped[1] != "a" || mapped[2] != "b" || mapped[3] != "c") {
        raise "right overlapping managed copy";
    }
    mapped[0:] = mapped[1:4];
    if (mapped[0] != "a" || mapped[1] != "b" || mapped[2] != "c" || mapped[3] != "c") {
        raise "left overlapping managed copy";
    }
    view := mapped[0:4];
    nested := view[1:3];
    if (len view != 4 || len nested != 2 || nested[0] != "b" || nested[1] != "c") {
        raise "nested view lost its backing owner";
    }
    scalar := array[] of {11, 22, 33, 44};
    scalar[1:] = scalar[0:3];
    scalar = scalar[1:3];
    if (len scalar != 2 || scalar[0] != 11 || scalar[1] != 22) {
        raise "scalar copy and in-place view narrowing";
    }
    relative_scalar := scalar[-2:-1];
    relative_scalar[0] = 99;
    if (len relative_scalar != 2 || relative_scalar[1] != 22 || scalar[0] != 11) {
        raise "relative scalar slice must publish an independent final copy";
    }
    samples := array[] of {"-42", "9007199254740993", "2.5", "Ж"};
    word := int samples[0];
    wide := big samples[1];
    realvalue := real samples[2];
    if (sparse(word) != 11 || sparse(0) != 33 || large(wide) != 19 ||
        large(-wide) != 23 || strings(samples[2]) != 29 || strings(samples[3]) != 31 ||
        dense(len samples) != 23 || dense(word) != 97) {
        raise "guarded case and goto selectors";
    }
    capacity := len samples - 2;
    messages := chan[capacity] of int;
    pairs := chan[capacity] of (int, int);
    records := chan[capacity] of (int, string);
    byteschan := chan[capacity] of byte;
    bigchan := chan[capacity] of big;
    realchan := chan[capacity] of real;
    stringchan := chan[capacity] of string;
    records <-= (22, samples[3]);
    records <-= (23, samples[0]);
    (tag, text) := <-records;
    (nexttag, nexttext) := <-records;
    messages <-= 19;
    messages <-= 24;
    pairs <-= (20, 21);
    pairs <-= (25, 26);
    message := <-messages;
    nextmessage := <-messages;
    (left, right) := <-pairs;
    (nextleft, nextright) := <-pairs;
    byteschan <-= byte 7;
    bigchan <-= wide;
    realchan <-= realvalue;
    stringchan <-= samples[3];
    if (int <-byteschan != 7 || <-bigchan != wide ||
        <-realchan != realvalue || <-stringchan != "Ж") {
        raise "typed channel constructor replay";
    }
    if (message != 19 || left != 20 || right != 21 || tag != 22 || text != "Ж" ||
        nexttag != 23 || nexttext != "-42" || nextmessage != 24 ||
        nextleft != 25 || nextright != 26) {
        raise "guarded channel capture changed buffered transfer";
    }
    altvalue := 0;
    messages <-= 31;
    alt {
        altvalue = <-messages => {}
    }
    if (altvalue != 31) {
        raise "blocking ALT header replay";
    }
    alt {
        <-messages => raise "unexpected value after ALT";
        * => altvalue = 37;
    }
    if (altvalue != 37) {
        raise "nonblocking ALT header replay";
    }
    encoded := array of byte samples[3];
    decoded := string encoded;
    if (word != -42 || wide != big 9007199254740993 || realvalue != 2.5 ||
        string word != "-42" || string wide != "9007199254740993" ||
        real string realvalue != 2.5 || decoded != "Ж" || len encoded != 2) {
        raise "guarded string conversion mismatch";
    }
    checkwide := big 0;
    {
        checkwide = big samples[3];
    } exception failure {
        "test UTF-8 view refusal" => checkwide = big 19;
        * => raise;
    }
    if (checkwide != big 19 && checkwide != big 0) {
        raise "string conversion refusal handling";
    }
    checkwide = big samples[1];
    if (checkwide != wide) {
        raise "string conversion failed after refusal";
    }
    narrowtext, widetext: string;
    for (step := 0; step < 3; step++) {
        narrowtext = string (word + step);
        widetext = string (wide + big step);
        if (int narrowtext != word + step || big widetext != wide + big step) {
            raise "integer string publication mismatch";
        }
    }
    realtext, asciitext: string;
    asciibytes: array of byte;
    for (pass := 0; pass < 3; pass++) {
        realtext = string (realvalue + real pass);
        asciibytes = array of byte samples[0];
        asciitext = string asciibytes;
        if (real realtext != realvalue + real pass || asciitext != samples[0]) {
            raise "real and byte string publication mismatch";
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
#include "runtime_gc.h"
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
static DVM_GCWorkers *array_collectors; /* Borrowed collector service until the fixture exits. */
static int address_publication; /* Observe only the private-to-shared publication call. */
const char *__real_dvm_runtime_bytes_publish_opcode(int, const void *, void *, size_t);
DVM_CellResult __real_dvm_managed_ranges_begin(const void *, size_t, void *, size_t, DVM_RangeAccess *);


char *__real_dvm_runtime_string_view(String *, const char **);

/* Make the second CVTCL view fail after it has reserved its destination.
 * The guest catches the exact error and converts again on the same frame. */
char *__wrap_dvm_runtime_string_view(String *string, const char **problem)
{
    static unsigned int calls; /* CVTCL calls that reached view preparation. */
    if (CONTROL == 0 && dvm_native_execution()->arithmetic_retry_opcode == ICVTCL && ++calls == 2)
    {
        *problem = "test UTF-8 view refusal";
        fprint(1, "PASS injected string view refusal\n");
        return NULL;
    }
    return __real_dvm_runtime_string_view(string, problem);
}


/* Stage selects an independent guest capture. Refuse it once, then verify the replay PC. */
static int contend_capture(int stage)
{
    static Inst *pcs[50]; /* Post-decode PC for each refused capture. */
    static int replayed[50]; /* One successful replay for each stage. */

    if (replayed[stage])
    {
        return 0;
    }
    if (!pcs[stage])
    {
        pcs[stage] = DVM_R.PC;
        return 1;
    }
    if (pcs[stage] != DVM_R.PC)
    {
        fprint(2, "FAIL guarded capture replay changed PC\n");
        exit(10);
    }
    replayed[stage] = 1;
    fprint(1, "PASS guarded capture replay %d\n", stage);
    return 0;
}


static __thread int ring_kind; /* 1 mapped, 2 native scalar, 3 raw record. */
int __real_dvm_runtime_mapped_buffer_transfer(const void *, void *, Type *, int *, int);
DVM_CellResult __real_dvm_managed_spans_begin(const void *, size_t, void *, size_t, DVM_CellAccess *);

/* Identify the exact transfer layout before injecting at its range claim. */
int __wrap_dvm_runtime_mapped_buffer_transfer(const void *source, void *destination,
    Type *type, int *consumed, int opcode)
{
    int result;
    ring_kind = type->np ? 1 : (type->size <= sizeof(void *) ? 2 : 3);
    result = __real_dvm_runtime_mapped_buffer_transfer(source, destination, type, consumed, opcode);
    ring_kind = 0;
    return result;
}


/* Inject BUSY into the first guarded WORD send and receive. */
DVM_CellResult __wrap_dvm_managed_spans_begin(const void *source, size_t source_size,
    void *destination, size_t destination_size, DVM_CellAccess *access)
{
    int opcode = dvm_native_execution()->arithmetic_retry_opcode;
    if (CONTROL == 0 && ring_kind == 2 && source_size == sizeof(WORD) &&
        ((opcode == ISEND && contend_capture(44)) || (opcode == IRECV && contend_capture(45))))
    {
        return DVM_CELL_BUSY;
    }
    return __real_dvm_managed_spans_begin(source, source_size, destination, destination_size, access);
}


DVM_CellResult __real_dvm_managed_three_begin(const void *, size_t, const void *, size_t,
    void *, size_t, DVM_RangeAccess *);

/* Refuse once at the existing-owner payload/header/slot claim of each codec. */
DVM_CellResult __wrap_dvm_managed_three_begin(const void *payload, size_t payload_size,
    const void *header, size_t header_size, void *slot, size_t slot_size, DVM_RangeAccess *access)
{
    int opcode = dvm_native_execution()->arithmetic_retry_opcode;
    if (CONTROL == 0 && !active &&
        ((opcode == ICVTCA && contend_capture(19)) || (opcode == ICVTAC && contend_capture(20))))
    {
        return DVM_CELL_BUSY;
    }
    return __real_dvm_managed_three_begin(payload, payload_size, header, header_size, slot, slot_size, access);
}


/* Observe source/destination geometry inside the actual guard acquisition. */
DVM_CellResult __wrap_dvm_managed_ranges_begin(const void *source, size_t source_size,
    void *destination, size_t destination_size, DVM_RangeAccess *access)
{
    static Inst *busy_pc; /* First decoded index that reaches publication. */
    static int replayed; /* One matching address publication retry completed. */

    if (CONTROL == 0 && !active && !destination && !destination_size &&
        dvm_native_execution()->arithmetic_retry_opcode == ISLICELA && contend_capture(1))
    {
        return DVM_CELL_BUSY;
    }

    if (CONTROL == 0 && !active)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if (opcode == ICVTCW || opcode == ICVTCL || opcode == ICVTCF)
        {
            size_t width = opcode == ICVTCW ? sizeof(WORD) :
                (opcode == ICVTCL ? sizeof(LONG) : sizeof(REAL));
            if (source != DVM_R.s || source_size != sizeof(String *) ||
                destination != DVM_R.d || destination_size != width)
            {
                fprint(2, "FAIL conversion must claim both decoded spans together\n");
                exit(10);
            }
            if ((opcode == ICVTCW && contend_capture(3)) ||
                (opcode == ICVTCL && contend_capture(4)) ||
                (opcode == ICVTCF && contend_capture(5)))
            {
                return DVM_CELL_BUSY;
            }
        }
    }

    if (CONTROL == 0 && !active && source && destination)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if (opcode == ICVTWC || opcode == ICVTLC)
        {
            String *previous;
            int stage;
            memmove(&previous, destination, sizeof(previous));
            stage = (opcode == ICVTWC ? 11 : 12) + (source == previous ? 2 : 0);
            if (contend_capture(stage))
            {
                return DVM_CELL_BUSY;
            }
        }
    }

    if (CONTROL == 0 && !active && destination)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if (opcode == ICVTFC && source)
        {
            String *previous;
            memmove(&previous, destination, sizeof(previous));
            if (contend_capture(source == previous ? 16 : 15))
            {
                return DVM_CELL_BUSY;
            }
        }
        if (!source && ((opcode == ICVTCA && contend_capture(17)) ||
                        (opcode == ICVTAC && contend_capture(18))))
        {
            return DVM_CELL_BUSY;
        }
    }

    if (CONTROL == 0 && !active && destination)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if (!source && opcode == ISLICEA)
        {
            WORD start, end;
            Array *previous;
            memmove(&start, DVM_R.s, sizeof(start));
            memmove(&end, DVM_R.m, sizeof(end));
            memmove(&previous, destination, sizeof(previous));
            int stage;
            if (start < 0 || end < 0)
            {
                stage = previous->t->np ? 21 : 26;
            }
            else
            {
                stage = previous->root == H ? 24 : 25;
            }
            if (contend_capture(stage))
            {
                return DVM_CELL_BUSY;
            }
        }
        if (source && opcode == ISLICELA &&
            ((source_size == 3 * sizeof(WORD) && contend_capture(22)) ||
             (source_size != 3 * sizeof(WORD) && contend_capture(23))))
        {
            return DVM_CELL_BUSY;
        }
    }

    if (CONTROL == 0 && !active && source && !destination)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if ((opcode == ICASE && contend_capture(27)) ||
            (opcode == ICASEL && contend_capture(28)) ||
            (opcode == ICASEC && contend_capture(29)) ||
            (opcode == IGOTO && contend_capture(30)))
        {
            return DVM_CELL_BUSY;
        }
    }

    if (CONTROL == 0 && !active && source && !destination)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if ((opcode == ISEND && contend_capture(31)) ||
            (opcode == IRECV && contend_capture(32)) ||
            (opcode == INEWCW && contend_capture(33)) ||
            (opcode == INEWCM && contend_capture(source == DVM_R.m ? 40 : 34)) ||
            (opcode == INEWCMP && contend_capture(source == DVM_R.m ? 41 : 35)) ||
            (opcode == INEWCB && contend_capture(36)) ||
            (opcode == INEWCL && contend_capture(37)) ||
            (opcode == INEWCF && contend_capture(38)) ||
            (opcode == INEWCP && contend_capture(39)))
        {
            return DVM_CELL_BUSY;
        }
    }

    if (CONTROL == 0 && !active && source && destination)
    {
        int opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if ((ring_kind == 1 && ((opcode == ISEND && contend_capture(42)) ||
                                (opcode == IRECV && contend_capture(43)))) ||
            (ring_kind == 3 && ((opcode == ISEND && contend_capture(46)) ||
                                (opcode == IRECV && contend_capture(47)))))
        {
            return DVM_CELL_BUSY;
        }
    }

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


/* Force Cycles after successful publication and before the guest consumes
 * the interior address. Only a real decoded index may suspend here. */
static void collect_before_address_use(int opcode)
{
    static unsigned int checked; /* One full collection per actual index opcode. */
    unsigned int bit;
    int released;

    switch (opcode)
    {
    case IINDX: bit = 1; break;
    case IINDW: bit = 2; break;
    case IINDF: bit = 4; break;
    case IINDL: bit = 8; break;
    case IINDB: bit = 16; break;
    default: return;
    }
    if (checked & bit)
    {
        return;
    }
    released = dvm_runtime_gc_native_try_leave();
    if (released != 1 || !array_collectors ||
        !dvm_gc_workers_request(array_collectors, DVM_GC_CYCLES) ||
        !dvm_gc_workers_wait_idle(array_collectors))
    {
        fprint(2, "FAIL cannot collect between array address and use\n");
        exit(10);
    }
    dvm_runtime_gc_native_enter(released);
    checked |= bit;
    if ((checked & 29) == 29)
    {
        print("PASS four interior addresses survive Cycles before use\n");
    }
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
    if (CONTROL == 0 && !problem && dvm_native_execution()->arithmetic_retry_opcode == opcode)
    {
        collect_before_address_use(opcode);
    }
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

    if (CONTROL == 0 && !active &&
        ((opcode == IALT && contend_capture(48)) || (opcode == INBALT && contend_capture(49)) ||
         (opcode == ISLICEA && contend_capture(0)) || (opcode == ISLICELA && contend_capture(2)) ||
         (opcode == ICVTCA && contend_capture(6)) || (opcode == ICVTAC && contend_capture(7)) ||
         (opcode == ICVTWC && contend_capture(8)) || (opcode == ICVTLC && contend_capture(9)) ||
         (opcode == ICVTFC && contend_capture(10))))
    {
        return DVM_CELL_BUSY;
    }

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
    array_collectors = workers;
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
                 'dvm_runtime_bytes_publish_opcode', 'dvm_managed_ranges_begin',
                 'dvm_runtime_string_view', 'dvm_managed_three_begin', 'dvm_managed_spans_begin',
                 'dvm_runtime_mapped_buffer_transfer', 'error', 'malloc', 'calloc', 'free'])
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
                                         'PASS scheduled address publication retry' not in output or
                                         'PASS four interior addresses survive Cycles before use' not in output):
                        raise RuntimeError('length extension retry missed: '+output)
                    if variant == 0 and 'PASS injected string view refusal' not in output:
                        raise RuntimeError('string view refusal coverage missed: '+output)
                    if variant == 0 and any(f'PASS guarded capture replay {stage}' not in output
                                            for stage in range(50)):
                        raise RuntimeError('slice/conversion replay coverage missed: '+output)
    print('PASS actual LENA:15360 aliases,nil/geometry,caller-rooted Cycles/Fast,no allocations,both polls')


if __name__ == '__main__':
    main()
