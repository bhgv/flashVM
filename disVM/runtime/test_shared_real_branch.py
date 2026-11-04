"""Actual REAL branches: coherent byte snapshots, unlocked FP, lazy target and flags."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = '''implement Reals;
include "sys.m";
Reals: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN real branches\\n");
    values := array[] of {0.0, -1.0, 1.0, 2.5};
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
        raise "real branch score";
    }
    sys->print("PASS real branches6\\n");
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
#include "managed_owner.h"
#include "shared_real_branch.h"
#include "real_branch_value.h"
#include "branch_value.h"
#include "disvm/native_context.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>

extern void beqf(void); /* Actual EQ entry. */
extern void bnef(void); /* Actual NE entry. */
extern void bltf(void); /* Actual LT entry. */
extern void blef(void); /* Actual LE entry. */
extern void bgtf(void); /* Actual GT entry. */
extern void bgef(void); /* Actual GE entry. */
static void (*const functions[])(void) = {beqf,bnef,bltf,blef,bgtf,bgef}; /* Existing operations. */
static __thread int active; /* Current fixture interval, never another worker's allocation. */
static __thread int phase; /* 1 input union, 2 later target. */
static __thread unsigned int selected; /* Condition0..5. */
static __thread unsigned char keys[256]; /* Independent exact stripe coverage. */
static __thread unsigned int depth; /* Coverage cardinality. */
static __thread unsigned long allocations; /* Native allocator calls during fixture operation. */
static __thread void *input_a; /* Independently live source cell. */
static __thread void *input_b; /* Independently live second source cell. */
static __thread void *target_cell; /* Independently live lazy target cell. */
static __thread Inst *replacement; /* Later target for deliberate phase-boundary update. */
static volatile int reference_sink; /* Consume reference predicates so FP evaluation is observable. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
DVM_CellResult __real_dvm_managed_three_begin(const void *,size_t,const void *,size_t,void *,size_t,DVM_RangeAccess *);
DVM_CellResult __real_dvm_managed_ranges_begin(const void *,size_t,void *,size_t,DVM_RangeAccess *);
int __real_dvm_range_access_end(DVM_RangeAccess *);
void __real_dvm_runtime_real_branch_capture(const void *,const void *,void *);
int __real_dvm_runtime_real_branch_predicate(int,const void *);
int __real_dvm_runtime_branch_target(const void *,void *);
void __real_error(char *);
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void __real_free(void *);


/* Require independent semantic valid/message; exit10 is the only accepted
 * omission result. Signals, compiler/sanitizer/resource failures never count. */
static void check(int valid,const char *message)
{
    if (!valid)
    {
        fprint(2,"FAIL real branch: %s\n",message);
        exit(10);
    }
}


/* Track each byte's canonical stripe without using production span selection.
 * add records successful actual admission, otherwise require exact coverage. */
static void coverage(const void *span,size_t size,int add)
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
        check(keys[key],"read span guarded");
    }
    if (!add)
    {
        check(depth && depth == dvm_native_context_object_count(dvm_native_context_current()),"read depth guarded");
    }
}


/* Initial admission contains both REAL cells and NO target. Controls omit or
 * refuse only this stage, before any input bytes are read. */
DVM_CellResult __wrap_dvm_managed_three_begin(const void *a,size_t an,const void *b,size_t bn,
    void *out,size_t size,DVM_RangeAccess *access)
{
    DVM_CellResult result;
    if (active)
    {
        check(a == input_a && b == input_b && an == sizeof(REAL) && bn == sizeof(REAL) && !out && !size,
            "input pair only");
        phase = 1;
        if (CONTROL == 1)
        {
            return DVM_CELL_ACQUIRED;
        }
        if (CONTROL == 7)
        {
            return DVM_CELL_BUSY;
        }
    }
    result = __real_dvm_managed_three_begin(a,an,b,bn,out,size,access);
    if (active && result == DVM_CELL_ACQUIRED)
    {
        coverage(a,an,1);
        coverage(b,bn,1);
    }
    return result;
}


/* Taken-target admission happens only after the input union has ended. */
DVM_CellResult __wrap_dvm_managed_ranges_begin(const void *target,size_t size,void *out,
    size_t out_size,DVM_RangeAccess *access)
{
    DVM_CellResult result;
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()) && target == target_cell &&
            size == sizeof(Inst *) && !out && !out_size,"separate lazy target");
        memset(keys,0,sizeof(keys));
        depth = 0;
        phase = 2;
        if (CONTROL == 3)
        {
            return DVM_CELL_ACQUIRED;
        }
        if (CONTROL == 6)
        {
            return DVM_CELL_BUSY;
        }
    }
    result = __real_dvm_managed_ranges_begin(target,size,out,out_size,access);
    if (active && result == DVM_CELL_ACQUIRED)
    {
        coverage(target,size,1);
    }
    return result;
}


/* Control2 omits initial end; predicate must detect that before FP evaluation. */
int __wrap_dvm_range_access_end(DVM_RangeAccess *access)
{
    if (active && phase == 1 && CONTROL == 2)
    {
        return 1;
    }
    return __real_dvm_range_access_end(access);
}


/* Actual snapshots require the entire pair, and must preserve their raw bits. */
void __wrap_dvm_runtime_real_branch_capture(const void *a,const void *b,void *pair)
{
    if (active)
    {
        coverage(a,sizeof(REAL),0);
        coverage(b,sizeof(REAL),0);
    }
    __real_dvm_runtime_real_branch_capture(a,b,pair);
    if (active)
    {
        check(!memcmp(pair,a,sizeof(REAL)) &&
            !memcmp((unsigned char *)pair+sizeof(REAL),b,sizeof(REAL)),"exact pair bits");
    }
}


/* Native FP always runs outside guards. Control5 injects a real VM error here;
 * control8 changes both original inputs and target after capture, proving the
 * saved pair and later target have distinct observation points. */
int __wrap_dvm_runtime_real_branch_predicate(int condition,const void *pair)
{
    if (active)
    {
        uint64_t a = UINT64_C(0x3ff0000000000000);
        uint64_t b = UINT64_C(0xbff0000000000000);
        check(!dvm_native_context_object_count(dvm_native_context_current()),"predicate without guards");
        if (CONTROL == 5)
        {
            error(exZdiv);
        }
        if (CONTROL == 8)
        {
            memcpy(input_a,&a,sizeof(a));
            memcpy(input_b,&b,sizeof(b));
            memcpy(target_cell,&replacement,sizeof(replacement));
        }
    }
    return __real_dvm_runtime_real_branch_predicate(condition,pair);
}


/* Read/publish target only under its new interval; control4 omits publication. */
int __wrap_dvm_runtime_branch_target(const void *target,void *pc)
{
    if (active)
    {
        coverage(target,sizeof(Inst *),0);
        if (CONTROL == 4)
        {
            return 1;
        }
    }
    return __real_dvm_runtime_branch_target(target,pc);
}


/* Actual VM errors must find no remaining object claims in either phase. */
void __wrap_error(char *message)
{
    check(!dvm_native_context_object_count(dvm_native_context_current()),"error after unlock");
    __real_error(message);
    abort();
}


/* Count actual native allocations only on the owning fixture thread. */
void *__wrap_malloc(size_t size)
{
    if (active)
    {
        allocations++;
    }
    return __real_malloc(size);
}


/* Count calloc requests without changing actual allocator behavior. */
void *__wrap_calloc(size_t count,size_t size)
{
    if (active)
    {
        allocations++;
    }
    return __real_calloc(count,size);
}


/* Count actual frees; no owner/destructor is needed for local REAL bytes. */
void __wrap_free(void *value)
{
    if (active)
    {
        allocations++;
    }
    __real_free(value);
}


/* Invoke the selected actual opcode through the current Proc's error frame. */
static int invoke(void *unused)
{
    (void)unused;
    functions[selected]();
    return 1;
}


/* REG/cells are caller-owned. Reset diagnostics; return real kprotect result,
 * requiring zero allocations and complete unlock on normal/error return. */
static int operation(void)
{
    int result;
    memset(keys,0,sizeof(keys));
    depth = 0;
    phase = 0;
    allocations = 0;
    active = 1;
    result = kprotect(invoke,NULL);
    active = 0;
    check(!allocations && !dvm_native_context_object_count(dvm_native_context_current()),"unlocked allocation-free");
    return result;
}


/* Compare IEEE binary64 representations mathematically, without floating-point
 * instructions. Handle both zeros and unordered NaNs before monotonic bit keys. */
static int truth(uint64_t a,uint64_t b,unsigned int condition)
{
    uint64_t fraction = UINT64_C(0x000fffffffffffff);
    uint64_t exponent = UINT64_C(0x7ff0000000000000);
    uint64_t sign = UINT64_C(0x8000000000000000);
    uint64_t ak;
    uint64_t bk;
    int order;
    if (((a&exponent) == exponent && (a&fraction)) || ((b&exponent) == exponent && (b&fraction)))
    {
        return condition == 1;
    }
    ak = a&sign ? ~a : a^sign;
    bk = b&sign ? ~b : b^sign;
    order = ((a&~sign) == 0 && (b&~sign) == 0) ? 0 : (ak > bk)-(ak < bk);
    switch (condition)
    {
    case 0: return order == 0;
    case 1: return order != 0;
    case 2: return order < 0;
    case 3: return order <= 0;
    case 4: return order > 0;
    default: return order >= 0;
    }
}


/* Reference current native C comparison flags, independently of production
 * helpers; bits are fixture snapshots. Consume the result to retain evaluation. */
static int reference_flags(uint64_t a,uint64_t b,unsigned int condition)
{
    volatile REAL left;
    volatile REAL right;
    memcpy((void *)&left,&a,sizeof(a));
    memcpy((void *)&right,&b,sizeof(b));
    feclearexcept(FE_ALL_EXCEPT);
    switch (condition)
    {
    case 0:
        reference_sink = left == right;
        break;
    case 1:
        reference_sink = left != right;
        break;
    case 2:
        reference_sink = left < right;
        break;
    case 3:
        reference_sink = left <= right;
        break;
    case 4:
        reference_sink = left > right;
        break;
    default:
        reference_sink = left >= right;
        break;
    }
    return fetestexcept(FE_ALL_EXCEPT);
}


/* workers is borrowed but no managed fixture object/root is fabricated. Preserve
 * full REG, VM error text and FP environment. Exceptions are masked for IEEE
 * special-value tests; a separate injected VM error checks unlocked unwinding. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    static const uint64_t values[] = {0,UINT64_C(0x8000000000000000),UINT64_C(0x3ff0000000000000),
        UINT64_C(0xbff0000000000000),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),
        UINT64_C(0x7ff8000000000001),UINT64_C(0x7ff0000000000001),1,UINT64_C(0x7fefffffffffffff)}; /* IEEE boundaries. */
    REG saved;
    fenv_t fp;
    char message[ERRMAX];
    Inst code[3];
    unsigned char bytes[80];
    unsigned char before[80];
    unsigned long cases = 0;
    unsigned int a;
    unsigned int b;
    int other_offset;
    int target_offset;
    __real_dvm_runtime_root_registry_probe(workers);
    check(sizeof(REAL) == 8 && sizeof(Inst *) == 8,"native binary64 fixture");
    memcpy(&saved,&DVM_R,sizeof(saved));
    memcpy(message,getup()->env->errstr,sizeof(message));
    check(!feholdexcept(&fp),"save masked FP environment");
    if (CONTROL == 8)
    {
        uint64_t left = 0;
        uint64_t right = 0;
        Inst *destination = &code[1];
        input_a = &left;
        input_b = &right;
        target_cell = &destination;
        replacement = &code[2];
        selected = 0;
        DVM_R.s = input_a;
        DVM_R.m = input_b;
        DVM_R.d = target_cell;
        DVM_R.PC = &code[0];
        check(operation() && DVM_R.PC == &code[2] && left != right &&
            destination == &code[2],"saved pair and latest target");
        goto finish;
    }
    for (selected = 0; selected < 6; selected++)
    {
        for (a = 0; a < 10; a++)
        {
            for (b = 0; b < 10; b++)
            {
                for (other_offset = -7; other_offset <= 7; other_offset++)
                {
                    for (target_offset = -7; target_offset <= 7; target_offset++)
                    {
                        Inst *destination = &code[1];
                        uint64_t left;
                        uint64_t right;
                        int expected;
                        int flags;
                        memset(bytes,0xa5,sizeof(bytes));
                        input_a = bytes+24;
                        input_b = bytes+24+other_offset;
                        target_cell = bytes+24+target_offset;
                        memcpy(input_a,&values[a],8);
                        memcpy(input_b,&values[b],8);
                        memcpy(target_cell,&destination,8);
                        memcpy(before,bytes,sizeof(bytes));
                        memcpy(&left,input_a,8);
                        memcpy(&right,input_b,8);
                        expected = truth(left,right,selected);
                        flags = reference_flags(left,right,selected);
                        feclearexcept(FE_ALL_EXCEPT);
                        DVM_R.s = input_a;
                        DVM_R.m = input_b;
                        DVM_R.d = target_cell;
                        DVM_R.PC = &code[0];
                        replacement = &code[2];
                        if (CONTROL == 5 || CONTROL == 7 || (CONTROL == 6 && expected))
                        {
                            check(!operation() && !strcmp(getup()->env->errstr,CONTROL == 5 ? exZdiv : exHeap) &&
                                DVM_R.PC == &code[0] && !memcmp(bytes,before,sizeof(bytes)),"refusal or FP-boundary error unchanged");
                            goto finish;
                        }
                        if (CONTROL == 8)
                        {
                            check(operation() && DVM_R.PC == &code[expected ? 2 : 0],"saved pair and latest target");
                            goto finish;
                        }
                        check(operation() && DVM_R.PC == &code[expected ? 1 : 0] &&
                            !memcmp(bytes,before,sizeof(bytes)),"PC and unchanged cells");
                        check(fetestexcept(FE_ALL_EXCEPT) == flags,"native FP status preserved");
                        cases++;
                    }
                }
            }
        }
    }
    check(cases == 135000,"all conditions IEEE aliases");
    /* Keep every boundary pattern intact in a disjoint source/target grid too;
     * pointer overlap in the alias grid intentionally changes some bit patterns. */
    for (selected = 0; selected < 6; selected++)
    {
        for (a = 0; a < 10; a++)
        {
            for (b = 0; b < 10; b++)
            {
                uint64_t left = values[a];
                uint64_t right = values[b];
                Inst *destination = &code[1];
                int expected = truth(left,right,selected);
                int flags = reference_flags(left,right,selected);
                feclearexcept(FE_ALL_EXCEPT);
                input_a = &left;
                input_b = &right;
                target_cell = &destination;
                DVM_R.s = input_a;
                DVM_R.m = input_b;
                DVM_R.d = target_cell;
                DVM_R.PC = &code[0];
                check(operation() && DVM_R.PC == &code[expected ? 1 : 0] &&
                    left == values[a] && right == values[b] && destination == &code[1],
                    "intact IEEE boundary pair");
                check(fetestexcept(FE_ALL_EXCEPT) == flags,"intact native FP status");
                cases++;
            }
        }
    }
    check(cases == 135600,"alias and intact IEEE cases");
    for (selected = 0; selected < 6; selected++)
    {
        uint64_t zero = 0;
        uint64_t one = UINT64_C(0x3ff0000000000000);
        void *bad[3] = {NULL,H,(void *)((uintptr_t)-1-2)};
        Inst *bad_value[2] = {NULL,H};
        unsigned int index;
        int expected = truth(zero,one,selected);
        input_a = &zero;
        input_b = &one;
        DVM_R.s = input_a;
        DVM_R.m = input_b;
        for (index = 0; index < 3; index++)
        {
            int completed;
            target_cell = bad[index];
            DVM_R.d = target_cell;
            DVM_R.PC = &code[0];
            completed = operation();
            check((expected ? !completed && !strcmp(getup()->env->errstr,exBounds) : completed) &&
                DVM_R.PC == &code[0],"lazy target geometry");
        }
        for (index = 0; index < 2; index++)
        {
            int completed;
            target_cell = &bad_value[index];
            DVM_R.d = target_cell;
            DVM_R.PC = &code[0];
            completed = operation();
            check((expected ? !completed && !strcmp(getup()->env->errstr,exBounds) : completed) &&
                DVM_R.PC == &code[0],"lazy target sentinel");
        }
        DVM_R.s = H;
        check(!operation() && !strcmp(getup()->env->errstr,exBounds),"source geometry before FP");
    }
    {
        Inst *foreign = &code[0];
        check(dvm_runtime_real_branch(6,H,H,H,&DVM_R.PC) == exHeap,"invalid condition first");
        check(dvm_runtime_real_branch(0,H,H,H,&foreign) == exHeap && foreign == &code[0],"foreign PC refused");
    }
finish:
    memcpy(&DVM_R,&saved,sizeof(saved));
    memcpy(getup()->env->errstr,message,sizeof(message));
    check(!fesetenv(&fp),"restore full FP environment");
    print("PASS real branch probe:135600 IEEE cases or boundary control\n");
}
'''


def main():
    """Use actual matching objects, independent booleans/status, phase and error controls."""
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='guarded real branch ') as directory:
        work = Path(directory)
        source, image = work/'main.b', work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler),'-F','-I',str(include),'-o',str(image),str(source)],check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text(WRAPPER)
        variants = (0,5,6,7,8) if '--positive-only' in sys.argv else tuple(range(9))
        for variant in variants:
            binary = relink(runtime,work,wrapper,variant,['dvm_runtime_root_registry_probe',
                'dvm_managed_three_begin','dvm_managed_ranges_begin','dvm_range_access_end',
                'dvm_runtime_real_branch_capture','dvm_runtime_real_branch_predicate',
                'dvm_runtime_branch_target','error','malloc','calloc','free'])
            for mode in ('--no-jit','--jit'):
                for locked in (False,True):
                    args = [str(binary),mode,'--gc-worker','--guarded-copies','--managed-owner-probe']
                    if locked:
                        args += ['--gc-locked-poll']
                    run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=90,
                        env=dict(os.environ,TMPDIR=str(work)))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant in (1,2,3,4):
                        marker = {1:'read span guarded',2:'predicate without guards',3:'read span guarded',4:'PC and unchanged cells'}[variant]
                        if run.returncode != 10 or 'FAIL real branch: '+marker not in output:
                            raise RuntimeError('control missed: '+output)
                    elif run.returncode or 'PASS real branch probe:' not in output or 'PASS real branches6' not in output:
                        raise RuntimeError('positive failed: '+output)
    print('PASS six actual REAL branches:135600 IEEE/alias cases, native FP flags, unlocked predicate, both polls')


if __name__ == '__main__':
    main()
