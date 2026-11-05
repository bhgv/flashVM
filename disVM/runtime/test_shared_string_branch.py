"""Actual six string branches with real ASCII/Rune owners and exact guard controls."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = '''implement Strings;
include "sys.m";
Strings: module {init: fn();};
compare(left, right: string, done: chan of int)
{
    if (left < right)
        done <-= 1;
    else
        done <-= 0;
}
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN string branches\\n");
    values := array[] of {"", "a", "Ж", "ab"};
    done := chan of int;
    spawn compare(values[1], values[3], done);
    spawn compare(values[1], values[2], done);
    first := <-done;
    second := <-done;
    if (first != 1 || second != 1)
        raise "concurrent string comparison";
    score := 0;
    for (i := 0; i < 4; i++) {
        for (j := 0; j < 4; j++) {
            if (values[i] == values[j])
            {
                score++;
            }
            if (values[i] != values[j])
            {
                score++;
            }
            if (values[i] < values[j])
            {
                score++;
            }
            if (values[i] <= values[j])
            {
                score++;
            }
            if (values[i] > values[j])
            {
                score++;
            }
            if (values[i] >= values[j])
            {
                score++;
            }
        }
    }
    if (score != 48)
    {
        raise "string branch score";
    }
    prefix := "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    longleft := prefix + values[1];
    longright := prefix + values[3];
    if (longleft >= longright)
        raise "long string branch fallback";
    rune_prefix := "ЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖЖ";
    rune_left := rune_prefix + values[1];
    rune_right := rune_prefix + values[3];
    if (rune_left >= rune_right)
        raise "long Rune branch fallback";
    sys->print("PASS string branches6\\n");
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
#include "managed_owner.h"
#include "root_registry.h"
#include "shared_string_branch.h"
#include "string_branch_value.h"
#include "branch_value.h"
#include "disvm/native_context.h"
#include "disvm/native_owner.h"
#include <pthread.h>
#include "disvm/refcount.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern void beqc(void); /* Actual native EQ string entry. */
extern void bnec(void); /* Actual native NE string entry. */
extern void bltc(void); /* Actual native LT string entry. */
extern void blec(void); /* Actual native LE string entry. */
extern void bgtc(void); /* Actual native GT string entry. */
extern void bgec(void); /* Actual native GE string entry. */
static void (*const functions[])(void) = {beqc, bnec, bltc, blec, bgtc, bgec}; /* Retained six operations. */
static __thread int active; /* Owner-local fixture interval. */
static __thread unsigned int selected; /* Current condition. */
static __thread unsigned char keys[256]; /* Independent coverage of successfully requested spans. */
static __thread unsigned int depth; /* Cardinality of that coverage. */
static __thread unsigned long reads; /* Payload-key visits. */
static __thread unsigned long allocations; /* Allocation/free inside an opcode. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
DVM_CellResult __real_dvm_managed_three_begin(const void *,size_t,const void *,size_t,void *,size_t,DVM_RangeAccess *);
DVM_CellResult __real_dvm_range_access_extend(DVM_RangeAccess *,const void *,size_t);
void *__real_dvm_runtime_string_branch_source(const void *);
void __real_dvm_runtime_string_branch_header(const void *,int *,int *);
uint32_t __real_dvm_runtime_string_branch_key(const void *,size_t,int);
int __real_dvm_runtime_branch_target(const void *,void *);
void __real_error(char *);
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void __real_free(void *);


/* valid/message describe an independent semantic obligation. Exit10 only for
 * that failure; compiler, sanitizer, resource or crash failures are not controls. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL string branch: %s\n", message);
        exit(10);
    }
}


/* Track a mapped span in the independent byte/stripe oracle; add selects update
 * versus verification. No managed memory or production selection code is used. */
static void coverage(const void *span, size_t size, int add)
{
    size_t index;
    for (index = 0; index < size; index++)
    {
        unsigned int key = (unsigned int)((((uintptr_t)span+index)/sizeof(void *))%256);
        if (add && !keys[key])
        {
            keys[key] = 1;
            depth++;
        }
        check(keys[key], "read span guarded");
    }
    if (!add)
    {
        check(depth && dvm_native_context_object_count(dvm_native_context_current()) == depth,
            "read depth guarded");
    }
}


/* Observe actual initial admission; control1 forges success before any read. */
DVM_CellResult __wrap_dvm_managed_three_begin(const void *a,size_t an,const void *b,size_t bn,
    void *c,size_t cn,DVM_RangeAccess *access)
{
    DVM_CellResult result;
    static Inst *retry_pc; /* Post-decode PC of the selected guest branch. */
    static int replayed; /* The selected branch reached admission again. */
    int opcode = dvm_native_execution()->arithmetic_retry_opcode;

    if (CONTROL == 0 && !active && !replayed &&
        (opcode == IBEQC || opcode == IBNEC || opcode == IBLTC || opcode == IBLEC || opcode == IBGTC || opcode == IBGEC))
    {
        if (!retry_pc)
        {
            retry_pc = DVM_R.PC;
            return DVM_CELL_BUSY;
        }
        check(DVM_R.PC == retry_pc, "same branch replays after input contention");
        replayed = 1;
        print("PASS scheduled input retry\n");
    }

    if (active && CONTROL == 1)
    {
        return DVM_CELL_ACQUIRED;
    }
    result = __real_dvm_managed_three_begin(a,an,b,bn,c,cn,access);
    if (active && result == DVM_CELL_ACQUIRED)
    {
        coverage(a,an,1);
        coverage(b,bn,1);
        coverage(c,cn,1);
    }
    return result;
}


/* Preserve the real continuously held prefix; control2 omits new claims and
 * control4 returns BUSY, both before any unexcluded header/payload load. */
DVM_CellResult __wrap_dvm_range_access_extend(DVM_RangeAccess *access,const void *span,size_t size)
{
    DVM_CellResult result;
    if (active)
    {
        check(access->held == depth && access->count == depth &&
            dvm_native_context_object_count(dvm_native_context_current()) == depth, "continuous prefix");
        if (CONTROL == 2)
        {
            return DVM_CELL_ACQUIRED;
        }
        if (CONTROL == 4)
        {
            return DVM_CELL_BUSY;
        }
    }
    result = __real_dvm_range_access_extend(access,span,size);
    if (active && result == DVM_CELL_ACQUIRED)
    {
        coverage(span,size,1);
    }
    return result;
}


/* Snapshot actual reference bits only after independently checked coverage. */
void *__wrap_dvm_runtime_string_branch_source(const void *source)
{
    if (active)
    {
        coverage(source,sizeof(void *),0);
    }
    return __real_dvm_runtime_string_branch_source(source);
}


/* Both actual int fields require the retained owner/header guards. */
void __wrap_dvm_runtime_string_branch_header(const void *object,int *length,int *capacity)
{
    if (active)
    {
        coverage((const unsigned char *)object+offsetof(String,len),sizeof(int),0);
        coverage((const unsigned char *)object+offsetof(String,max),sizeof(int),0);
    }
    __real_dvm_runtime_string_branch_header(object,length,capacity);
}


static pthread_mutex_t overlap_lock = PTHREAD_MUTEX_INITIALIZER; /* Protect evaluator rendezvous. */
static pthread_cond_t overlap_ready = PTHREAD_COND_INITIALIZER; /* Wake the first evaluator. */
static DVM_NativeContext *first_evaluator; /* First context, borrowed until the rendezvous. */
static int evaluators; /* Number of distinct simultaneous evaluators, capped at two. */


/* Park the first private comparison until another OS owner also reaches it. */
static void observe_overlap(void)
{
    DVM_NativeContext *context = dvm_native_context_current();

    pthread_mutex_lock(&overlap_lock);
    if (!evaluators)
    {
        first_evaluator = context;
        evaluators = 1;
        while (evaluators < 2)
        {
            pthread_cond_wait(&overlap_ready, &overlap_lock);
        }
    }
    else if (evaluators == 1)
    {
        check(context != first_evaluator, "distinct simultaneous string evaluators");
        evaluators = 2;
        print("PASS overlapping string leases\n");
        pthread_cond_broadcast(&overlap_ready);
    }
    pthread_mutex_unlock(&overlap_lock);
}


DVM_CellResult __real_dvm_managed_ranges_begin(const void *source, size_t bytes,
    void *destination, size_t output, DVM_RangeAccess *access);


/* Inject contention at the taken branch's second acquisition. Keep the
 * process identity under NativeWorld so another guest can run meanwhile. */
DVM_CellResult __wrap_dvm_managed_ranges_begin(const void *source, size_t bytes,
    void *destination, size_t output, DVM_RangeAccess *access)
{
    static Prog *contended_process; /* Borrowed live fixture guest until replay. */
    static Inst *contended_pc; /* Post-decode PC of the first taken branch. */
    static int replayed; /* Only one contention injection in this invocation. */
    int opcode = dvm_native_execution()->arithmetic_retry_opcode;
    uintptr_t next = (uintptr_t)DVM_R.PC;

    if (CONTROL == 0 && !active && !replayed && bytes == sizeof(Inst *) &&
        !destination && !output && next >= sizeof(Inst) &&
        (uintptr_t)source == next - sizeof(Inst) + offsetof(Inst, d) &&
        (opcode == IBEQC || opcode == IBNEC || opcode == IBLTC ||
         opcode == IBLEC || opcode == IBGTC || opcode == IBGEC))
    {
        Prog *process = getup()->prog;

        if (!contended_process)
        {
            contended_process = process;
            contended_pc = DVM_R.PC;
            return DVM_CELL_BUSY;
        }
        if (process == contended_process)
        {
            check(contended_pc == DVM_R.PC, "target contention replays the same string branch");
            replayed = 1;
            print("PASS string target contention replay\n");
        }
    }
    return __real_dvm_managed_ranges_begin(source, bytes, destination, output, access);
}


/* Every key read has common-prefix coverage before touching real string data. */
uint32_t __wrap_dvm_runtime_string_branch_key(const void *data,size_t index,int wide)
{
    DVM_HeapOwnerStats owner;

    if (CONTROL == 0 && !active &&
        dvm_native_owner_statistics(dvm_native_owner_current(), &owner))
    {
        if (!index && owner.phase == DVM_HEAP_NATIVE)
        {
            check(!dvm_native_context_object_count(dvm_native_context_current()), "private string comparison has no guard");
            observe_overlap();
            fprint(2, "LEASED string branch %d\n", dvm_native_execution()->arithmetic_retry_opcode);
        }
        if (index == 128 && owner.phase == DVM_HEAP_ACTIVE)
        {
            print(wide ? "PASS long Rune guarded fallback\n" : "PASS long ASCII guarded fallback\n");
        }
    }
    if (active)
    {
        size_t unit = wide ? sizeof(Rune) : 1;
        coverage((const unsigned char *)data+index*unit,unit,0);
        reads++;
    }
    return __real_dvm_runtime_string_branch_key(data,index,wide);
}


/* Taken targets alone publish PC; control3 skips actual publication. */
int __wrap_dvm_runtime_branch_target(const void *target,void *pc)
{
    if (active)
    {
        coverage(target,sizeof(void *),0);
        if (CONTROL == 3)
        {
            return 1;
        }
    }
    return __real_dvm_runtime_branch_target(target,pc);
}


/* Real VM errors must follow complete guard release. */
void __wrap_error(char *message)
{
    check(!dvm_native_context_object_count(dvm_native_context_current()), "error after unlock");
    __real_error(message);
    abort();
}


/* Count libc operations only inside the fixture's guarded opcode interval. */
void *__wrap_malloc(size_t size)
{
    if (active)
    {
        allocations++;
    }
    return __real_malloc(size);
}


/* Count native zero allocation without changing storage or allocator semantics. */
void *__wrap_calloc(size_t count,size_t size)
{
    if (active)
    {
        allocations++;
    }
    return __real_calloc(count,size);
}


/* Count frees; GC/destructors are forbidden inside a guarded comparison. */
void __wrap_free(void *value)
{
    if (active)
    {
        allocations++;
    }
    __real_free(value);
}


/* Invoke the actual selected opcode through the genuine owner error frame. */
static int invoke(void *unused)
{
    (void)unused;
    functions[selected]();
    return 1;
}


/* Run with REG already set; reset the independent oracle for exactly one call.
 * Return kprotect status, requiring zero allocation and complete guard release. */
static int operation(void)
{
    int completed;
    memset(keys,0,sizeof(keys));
    depth = 0;
    allocations = 0;
    active = 1;
    completed = kprotect(invoke,NULL);
    active = 0;
    check(!allocations && !dvm_native_context_object_count(dvm_native_context_current()), "allocation-free unlocked");
    return completed;
}


/* Independent fixture ordering: map uint32 bits to exact signed64 mathematical
 * values. Only valid fixture owners are read here, outside production guards. */
static int order_of(String *left,String *right)
{
    int a = left == H ? 0 : left->len;
    int b = right == H ? 0 : right->len;
    int an = a < 0 ? -a : a;
    int bn = b < 0 ? -b : b;
    int index;
    for (index = 0; index < an && index < bn; index++)
    {
        uint32_t x = a < 0 ? (uint32_t)left->Srune[index] : (unsigned char)left->Sascii[index];
        uint32_t y = b < 0 ? (uint32_t)right->Srune[index] : (unsigned char)right->Sascii[index];
        int64_t sx = (int64_t)x-(x >= UINT32_C(0x80000000) ? INT64_C(4294967296) : 0);
        int64_t sy = (int64_t)y-(y >= UINT32_C(0x80000000) ? INT64_C(4294967296) : 0);
        if (sx != sy)
        {
            return sx < sy ? -1 : 1;
        }
    }
    return (an > bn)-(an < bn);
}


/* value is fixture ordering and condition a validated operation0..5. Return
 * boolean without invoking production string predicate/snapshot helpers. */
static int truth_of(int value,unsigned int condition)
{
    switch (condition)
    {
    case 0: return value == 0;
    case 1: return value != 0;
    case 2: return value < 0;
    case 3: return value <= 0;
    case 4: return value > 0;
    default: return value >= 0;
    }
}


/* workers is the actual collector. Own real strings until all cases finish;
 * root them independently only for genuine stopped-world Cycles. Restore REG
 * and malformed fields before collection/destruction; no fake Heap enters GC. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    String *values[9];
    REG saved;
    char message[ERRMAX];
    Inst code[2];
    unsigned int a;
    unsigned int b;
    unsigned int skew;
    unsigned long cases = 0;
    unsigned char cells[64];
    unsigned char before[64];
    __real_dvm_runtime_root_registry_probe(workers);
    check(sizeof(Rune) == 4, "Rune32 fixture");
    memcpy(&saved,&DVM_R,sizeof(saved));
    memcpy(message,getup()->env->errstr,sizeof(message));
    values[0] = H;
    values[1] = newstring(0);
    values[2] = newstring(1);
    values[2]->Sascii[0] = (char)255;
    values[3] = c2string("ab",2);
    values[4] = newrunes(1);
    values[4]->Srune[0] = UINT32_C(0x80000000);
    values[5] = newrunes(1);
    values[5]->Srune[0] = UINT32_C(0xffffffff);
    values[6] = newrunes(2);
    values[6]->Srune[0] = 'a';
    values[6]->Srune[1] = 'b';
    values[7] = newrunes(2);
    values[7]->Srune[0] = UINT32_C(0x10ffff);
    values[7]->Srune[1] = 0;
    values[8] = c2string("a",1);
    for (selected = 0; selected < 6; selected++)
    {
        for (a = 0; a < 9; a++)
        {
            for (b = 0; b < 9; b++)
            {
                for (skew = 0; skew < 8; skew++)
                {
                    Inst *destination = &code[1];
                    int truth = truth_of(order_of(values[a],values[b]),selected);
                    memset(cells,0xa5,sizeof(cells));
                    memcpy(cells+skew,&values[a],sizeof(void *));
                    memcpy(cells+16+skew,&values[b],sizeof(void *));
                    memcpy(cells+32+skew,&destination,sizeof(destination));
                    memcpy(before,cells,sizeof(cells));
                    DVM_R.s = cells+skew;
                    DVM_R.m = cells+16+skew;
                    DVM_R.d = cells+32+skew;
                    DVM_R.PC = &code[0];
                    if (CONTROL == 4 && (a || b))
                    {
                        check(!operation() && !strcmp(getup()->env->errstr,exHeap) &&
                            DVM_R.PC == &code[0] && !memcmp(cells,before,sizeof(cells)), "extension refusal unchanged");
                        goto finish;
                    }
                    check(operation() && DVM_R.PC == &code[truth ? 1 : 0] &&
                        !memcmp(cells,before,sizeof(cells)), "PC and unchanged cells");
                    cases++;
                }
            }
        }
    }
    check(cases == 3888 && reads, "six conditions representations and alignment");
    for (selected = 0; selected < 6; selected++)
    {
        String *empty = H;
        String *bad = NULL;
        Inst *destination = &code[1];
        int truth = truth_of(0,selected);
        DVM_R.s = &empty;
        DVM_R.m = &empty;
        DVM_R.d = H;
        DVM_R.PC = &code[0];
        check((truth ? !operation() && !strcmp(getup()->env->errstr,exHeap) : operation()) &&
            DVM_R.PC == &code[0], "lazy target geometry");
        DVM_R.s = &bad;
        DVM_R.d = &destination;
        check(!operation() && !strcmp(getup()->env->errstr,exNilref) &&
            DVM_R.PC == &code[0], "NULL before header");
        DVM_R.s = H;
        check(!operation() && !strcmp(getup()->env->errstr,exHeap), "bad source geometry");
    }
    for (selected = 0; selected < 6; selected++)
    {
        String *empty = H;
        void *invalid[3] = {NULL,H,(void *)((uintptr_t)-1-2)};
        Inst *invalid_value[2] = {NULL,H};
        unsigned int bad;
        int truth = truth_of(0,selected);
        DVM_R.s = &empty;
        DVM_R.m = &empty;
        for (bad = 0; bad < 3; bad++)
        {
            int completed;
            DVM_R.d = invalid[bad];
            DVM_R.PC = &code[0];
            completed = operation();
            check((truth ? !completed && !strcmp(getup()->env->errstr,exHeap) : completed) &&
                DVM_R.PC == &code[0], "all lazy target geometries");
        }
        for (bad = 0; bad < 2; bad++)
        {
            int completed;
            DVM_R.d = &invalid_value[bad];
            DVM_R.PC = &code[0];
            completed = operation();
            check((truth ? !completed && !strcmp(getup()->env->errstr,exHeap) : completed) &&
                DVM_R.PC == &code[0], "lazy target sentinels");
        }
    }
    {
        String *a_string = newstring(4097);
        String *r_string = newrunes(4097);
        Inst *destination = &code[1];
        unsigned int index;
        memset(a_string->Sascii,'a',4097);
        for (index = 0; index < 4097; index++)
        {
            r_string->Srune[index] = 'a';
        }
        r_string->Srune[4096] = 'b';
        selected = 2;
        DVM_R.s = &a_string;
        DVM_R.m = &r_string;
        DVM_R.d = &destination;
        DVM_R.PC = &code[0];
        check(operation() && DVM_R.PC == &code[1] && depth == 256, "full-domain common prefix");
        destroy(a_string);
        destroy(r_string);
    }
    {
        Inst *foreign = &code[0];
        check(dvm_runtime_string_branch(6,H,H,H,&DVM_R.PC) == exHeap, "invalid condition before operands");
        check(dvm_runtime_string_branch(0,H,H,H,&foreign) == exHeap && foreign == &code[0], "foreign PC refused");
    }
    for (a = 0; a < 2; a++)
    {
        int old_length = values[8]->len;
        int old_max = values[8]->max;
        Inst *destination = &code[1];
        selected = 0;
        values[8]->len = a ? old_length : (-0x7fffffff-1);
        values[8]->max = a ? -1 : old_max;
        DVM_R.s = &values[8];
        DVM_R.m = &values[8];
        DVM_R.d = &destination;
        DVM_R.PC = &code[0];
        check(!operation() && !strcmp(getup()->env->errstr,exHeap) &&
            DVM_R.PC == &code[0], "identity still validates header");
        values[8]->len = old_length;
        values[8]->max = old_max;
    }
    /* Baseline conservation only: ref1 is not an exclusivity/provenance test. */
    for (a = 1; a < 9; a++)
    {
        check(dvm_refcount_load_u(&D2H(values[a])->ref) == 1 &&
            dvm_runtime_root_count(D2H(values[a])) == 0, "no temporary owner or root remains");
    }
    memcpy(&DVM_R,&saved,sizeof(saved));
    for (a = 1; a < 9; a++)
    {
        ptradd(D2H(values[a]));
    }
    {
        DVM_HeapOwner *owner = dvm_native_owner_current();
        check(dvm_native_owner_leave(owner), "leave for Cycles");
        for (a = 0; a < 32; a++)
        {
            check(dvm_gc_workers_request(workers,DVM_GC_CYCLES) && dvm_gc_workers_wait_idle(workers), "real Cycles");
        }
        check(dvm_native_owner_enter(owner) && order_of(values[3],values[6]) == 0, "rooted representations survive");
    }
    for (a = 1; a < 9; a++)
    {
        ptrdel(D2H(values[a]));
    }
finish:
    for (a = 1; a < 9; a++)
    {
        destroy(values[a]);
    }
    memcpy(&DVM_R,&saved,sizeof(saved));
    memcpy(getup()->env->errstr,message,sizeof(message));
    {
        DVM_HeapOwner *owner = dvm_native_owner_current();
        check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers,DVM_GC_FAST) &&
            dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner), "final Fast");
    }
    print("PASS string branch probe:3888 cases or extension refusal\n");
}
'''


def main():
    """Relink matching actual objects; require positives and exact-stage controls under both polls."""
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='guarded string branch ') as directory:
        work = Path(directory)
        source, image = work/'main.b', work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)], check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text(WRAPPER)
        for variant in ((0, 4) if '--positive-only' in sys.argv else (0, 1, 2, 3, 4)):
            binary = relink(runtime, work, wrapper, variant,
                ['dvm_runtime_root_registry_probe','dvm_managed_three_begin','dvm_managed_ranges_begin','dvm_range_access_extend',
                 'dvm_runtime_string_branch_source','dvm_runtime_string_branch_header',
                 'dvm_runtime_string_branch_key','dvm_runtime_branch_target','error','malloc','calloc','free'])
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    args = [str(binary),mode,'--gc-worker','--guarded-copies','--managed-owner-probe']
                    if variant == 0:
                        args += ['--managed-workers', '2', '--parallel-string']
                    if locked:
                        args += ['--gc-locked-poll']
                    run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=90,
                        env=dict(os.environ,TMPDIR=str(work)))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant in (1,2,3):
                        marker = 'read span guarded' if variant in (1,2) else 'PC and unchanged cells'
                        if run.returncode != 10 or 'FAIL string branch: '+marker not in output:
                            raise RuntimeError('control missed: '+output)
                    elif run.returncode or 'PASS string branch probe:' not in output or 'PASS string branches6' not in output:
                        raise RuntimeError('positive failed: '+output)
                    if variant == 0:
                        leased = {line for line in output.splitlines() if line.startswith('LEASED string branch ')}
                        if (len(leased) != 6 or 'PASS long ASCII guarded fallback' not in output or
                                'PASS long Rune guarded fallback' not in output or
                                'PASS overlapping string leases' not in output or
                                'PASS string target contention replay' not in output):
                            raise RuntimeError('leased string coverage or bounded fallback missed: '+output)
                    if variant == 0 and 'PASS scheduled input retry' not in output:
                        raise RuntimeError('input retry missed: '+output)
    print('PASS six actual string branches:3888 real-owner cases, lazy targets, Cycles/Fast, both polls')


if __name__ == '__main__':
    main()
