"""Actual REAL arithmetic: overlap, unlocked FP, exact source/output guards and phases."""
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
    sys->print("BEGIN REAL arithmetic5\\n");
    values := array[] of {2.0, 4.0};
    a := values[0];
    b := values[1];
    if (a+b != 6.0 || a-b != -2.0 || a*b != 8.0 || a/b != 0.5 || -a != -2.0)
    {
        raise "REAL arithmetic";
    }
    ints := array[] of {2, -3, 0, -1, 3};
    exponent := ints[1];
    if (ints[0] ** exponent != 0 || ints[3] ** exponent != -1 ||
        ints[0] ** ints[4] != 8 || big ints[0] ** ints[4] != big 8 ||
        a ** exponent != 0.125)
        raise "guarded powers";
    {
        ints[0] = ints[2] ** exponent;
        raise "negative zero-base exponent accepted";
    } exception {
        "zero divide" => {}
    }
    sys->print("PASS REAL arithmetic5\\n");
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
#include "shared_real_arithmetic.h"
#include "real_arithmetic_value.h"
#include "disvm/native_context.h"
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <float.h>
#include <stdint.h>

static __thread int active; /* Exact private test interval only. */
static __thread int phase; /* Input snapshot1, unlocked evaluation2, output3. */
static __thread unsigned int depth; /* Actual canonical stripe coverage. */
static __thread unsigned char keys[256]; /* Exact expected held union. */
static __thread int refusal; /* Deliberate input1/output2 admission refusal. */
static __thread int injected; /* VM unwind at unlocked FP boundary. */
static __thread int modify; /* Deliberate source update after the snapshot. */
static __thread void *input; /* Borrowed live first cell for phase update. */
static __thread int prints; /* Guest output markers, exact current OS owner. */
static __thread unsigned int powers;
static __thread int seen[5]; /* Actual guest C entries, shared by its current OS owner. */
static const int operations[5] = {IADDF,ISUBF,IMULF,IDIVF,INEGF}; /* Existing file opcodes. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
DVM_CellResult __real_dvm_managed_three_begin(const void *,size_t,const void *,size_t,void *,size_t,DVM_RangeAccess *);
DVM_CellResult __real_dvm_managed_ranges_begin(const void *,size_t,void *,size_t,DVM_RangeAccess *);
int __real_dvm_range_access_end(DVM_RangeAccess *);
void __real_dvm_runtime_real_branch_capture(const void *,const void *,void *);
void __real_dvm_runtime_real_arithmetic_value(int,const void *,void *);
void __real_dvm_runtime_real_arithmetic_store(void *,const void *);
const char *__real_dvm_runtime_real_arithmetic(int,const void *,const void *,void *);
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void __real_free(void *);
extern void Sys_print(void *); /* Actual guest print boundary. */
int __real_dvm_bigtable_call_ready(void (*)(void *),void *);


/* Stop only at the exact semantic predicate; never turn a crash into success. */
static void check(int valid,const char *message)
{
    if (!valid)
    {
        fprint(2,"FAIL real arithmetic: %s\n",message);
        exit(10);
    }
}


/* Observe canonical span identities independently of the access object's layout. */
static void span(const void *address,size_t bytes)
{
    uintptr_t begin = (uintptr_t)address / sizeof(void *);
    uintptr_t end = ((uintptr_t)address+bytes-1) / sizeof(void *);
    uintptr_t value;
    for (value = begin; value <= end; value++)
    {
        keys[value & 255] = 1;
    }
}


/* Require the expected union cardinality on the real TLS owner.
 * Identity selection uses actual guard implementation, not inferred from this count. */
static void coverage(void)
{
    unsigned int index;
    unsigned int count = 0;
    for (index = 0; index < 256; index++)
    {
        count += keys[index];
    }
    check(dvm_native_context_object_count(dvm_native_context_current()) == count,"exact guarded depth");
    depth = count;
}


/* Intercept actual input union without manufacturing a replacement lock. */
DVM_CellResult __wrap_dvm_managed_three_begin(const void *a,size_t n,const void *b,size_t m,void *c,size_t z,DVM_RangeAccess *access)
{
    DVM_CellResult result;
    if (active && refusal == 1)
    {
        return DVM_CELL_BUSY;
    }
    result = __real_dvm_managed_three_begin(a,n,b,m,c,z,access);
    if (active)
    {
        check(!phase && n == sizeof(REAL) && (m == sizeof(REAL) || (!b && !m)) && !c && !z,"input phase geometry");
        check(result == DVM_CELL_ACQUIRED,"input acquired");
        memset(keys,0,sizeof(keys));
        span(a,n);
        if (m)
        {
            span(b,m);
        }
        coverage();
        phase = 1;
    }
    return result;
}


/* Only the output is guarded in the later interval, never FP source storage. */
DVM_CellResult __wrap_dvm_managed_ranges_begin(const void *a,size_t n,void *b,size_t m,DVM_RangeAccess *access)
{
    DVM_CellResult result;
    if (active && refusal == 2)
    {
        return DVM_CELL_BUSY;
    }
    result = __real_dvm_managed_ranges_begin(a,n,b,m,access);
    if (active)
    {
        check(phase == 2 && !a && !n && m == sizeof(REAL),"output phase geometry");
        check(result == DVM_CELL_ACQUIRED,"output acquired");
        memset(keys,0,sizeof(keys));
        span(b,m);
        coverage();
        phase = 3;
    }
    return result;
}


/* Observe complete unlock before the next phase; preserve actual end result. */
int __wrap_dvm_range_access_end(DVM_RangeAccess *access)
{
    int result = __real_dvm_range_access_end(access);
    if (active)
    {
        check(result && depth && !dvm_native_context_object_count(dvm_native_context_current()),"complete phase release");
        depth = 0;
        phase++;
    }
    return result;
}


/* Real snapshot requires the complete coherent input union. */
void __wrap_dvm_runtime_real_branch_capture(const void *a,const void *b,void *pair)
{
    if (active)
    {
        check(phase == 1,"capture phase");
        coverage();
    }
    __real_dvm_runtime_real_branch_capture(a,b,pair);
}


/* FP evaluation must happen after unlock, even for aliased input and destination. */
void __wrap_dvm_runtime_real_arithmetic_value(int opcode,const void *pair,void *result)
{
    if (active)
    {
        check(phase == 2 && !depth && !dvm_native_context_object_count(dvm_native_context_current()),"unlocked FP");
        if (injected)
        {
            error(exHeap);
        }
        if (modify)
        {
            REAL replacement = 99.0;
            memmove(input,&replacement,sizeof(replacement));
        }
    }
    __real_dvm_runtime_real_arithmetic_value(opcode,pair,result);
}


/* Publication is bytes only and requires the later complete output union. */
void __wrap_dvm_runtime_real_arithmetic_store(void *output,const void *result)
{
    if (active)
    {
        check(phase == 3,"store phase");
        coverage();
    }
    __real_dvm_runtime_real_arithmetic_store(output,result);
}


/* Reject allocation/free while any test operation is in progress. */
void *__wrap_malloc(size_t bytes)
{
    check(!active,"no allocation");
    return __real_malloc(bytes);
}


/* Actual calloc remains available before/after the operation. */
void *__wrap_calloc(size_t n,size_t bytes)
{
    check(!active,"no allocation");
    return __real_calloc(n,bytes);
}


/* No metadata or managed retirement may be inserted inside this helper. */
void __wrap_free(void *value)
{
    check(!active,"no allocation");
    __real_free(value);
}


/* Count each genuine C opcode route in the surrounding real Limbo guest. */
const char *__wrap_dvm_runtime_real_arithmetic(int opcode,const void *a,const void *b,void *out)
{
    unsigned int index;
    if (!active)
    {
        for (index = 0; index < 5; index++)
        {
            if (opcode == operations[index])
            {
                seen[index]++;
            }
        }
    }
    return __real_dvm_runtime_real_arithmetic(opcode,a,b,out);
}


const char *__real_dvm_runtime_power(int,const void *,const void *,void *,
    void (*)(const void *,const void *,void *));


const char *__wrap_dvm_runtime_power(int opcode,const void *base,const void *exponent,void *out,
    void (*evaluate)(const void *,const void *,void *))
{
    if (opcode == IEXPW)
        powers |= 1;
    if (opcode == IEXPL)
        powers |= 2;
    if (opcode == IEXPF)
        powers |= 4;
    return __real_dvm_runtime_power(opcode,base,exponent,out,evaluate);
}


/* Before the guest prints, require all five genuine opcode routes. */
int __wrap_dvm_bigtable_call_ready(void (*function)(void *),void *frame)
{
    if (function == Sys_print && ++prints == 2)
    {
        check(seen[0] && seen[1] && seen[2] && seen[3] && seen[4],"five guest routes");
        check(powers == 7,"all guarded power routes");
    }
    return __real_dvm_bigtable_call_ready(function,frame);
}


/* Independent native expression oracle, evaluated with the same masked FP state. */
static REAL reference(int operation,REAL a,REAL b)
{
    volatile REAL left = a;
    volatile REAL right = b;
    switch (operation)
    {
    case 0:
        return left+right;
    case 1:
        return left-right;
    case 2:
        return left*right;
    case 3:
        return left/right;
    default:
        return -left;
    }
}


/* Invoke real guards/helper on retained mapped bytes; reset private phase state. */
static void invoke(int opcode,const void *a,const void *b,void *out)
{
    const char *problem;
    phase = 0;
    active = 1;
    problem = __real_dvm_runtime_real_arithmetic(opcode,a,b,out);
    active = 0;
    check(!problem && phase == 4 && !depth,"complete two phases");
}


/* Throw through the actual VM protection boundary with independently live cells.
 * Argument is unused; the caller checks rollback after kprotect unwinds. */
static void unwind(void *unused)
{
    REAL left = 1.0;
    REAL right = 0.0;
    REAL out = 123.0;
    (void)unused;
    (void)__real_dvm_runtime_real_arithmetic(IDIVF,&left,&right,&out);
}


/* Actual ACTIVE owner supplies live native byte cells; no fake Heap enters GC.
 * Preserve the caller's FP environment and force masked exceptions only in this
 * fixture. Check overlapping/unaligned full buffers, native flags and phase update. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    unsigned char bytes[96];
    unsigned char expected[96];
    unsigned int op;
    unsigned int a;
    unsigned int b;
    unsigned int d;
    unsigned int count = 0;
    fenv_t environment;
    REAL specials[] = {0.0,-0.0,1.0,-1.0,DBL_MAX,DBL_MIN};
    __real_dvm_runtime_root_registry_probe(workers);
    check(!feholdexcept(&environment),"fixture FP setup");
    for (op = 0; op < 5; op++)
    {
        for (a = 0; a < 12; a++)
        {
            for (b = 0; b < 12; b++)
            {
                for (d = 0; d < 12; d++)
                {
                    REAL left;
                    REAL right;
                    REAL result;
                    int flags;
                    unsigned int i;
                    for (i = 0; i < sizeof(bytes); i++)
                    {
                        bytes[i] = (unsigned char)(17*i+op+a+3*b);
                    }
                    memmove(&left,bytes+a,sizeof(left));
                    memmove(&right,bytes+b,sizeof(right));
                    memmove(expected,bytes,sizeof(bytes));
                    feclearexcept(FE_ALL_EXCEPT);
                    result = reference(op,left,right);
                    flags = fetestexcept(FE_ALL_EXCEPT);
                    memmove(expected+d,&result,sizeof(result));
                    feclearexcept(FE_ALL_EXCEPT);
                    invoke(operations[op],bytes+a,bytes+b,bytes+d);
                    check(fetestexcept(FE_ALL_EXCEPT) == flags && !memcmp(bytes,expected,sizeof(bytes)),"overlap result and flags");
                    count++;
                }
            }
        }
        for (a = 0; a < sizeof(specials)/sizeof(specials[0]); a++)
        {
            for (b = 0; b < sizeof(specials)/sizeof(specials[0]); b++)
            {
                REAL value;
                REAL wanted;
                int flags;
                feclearexcept(FE_ALL_EXCEPT);
                wanted = reference(op,specials[a],specials[b]);
                flags = fetestexcept(FE_ALL_EXCEPT);
                feclearexcept(FE_ALL_EXCEPT);
                invoke(operations[op],&specials[a],&specials[b],&value);
                check(fetestexcept(FE_ALL_EXCEPT) == flags && !memcmp(&value,&wanted,sizeof(value)),"boundary result and flags");
                count++;
            }
        }
    }
    {
        REAL first = 6.0;
        REAL second = 3.0;
        REAL output = 0.0;
        input = &first;
        modify = 1;
        invoke(IDIVF,&first,&second,&output);
        modify = 0;
        check(first == 99.0 && output == 2.0,"saved pair after source update");
        check(__real_dvm_runtime_real_arithmetic(IADDF,&first,&second,NULL) == exBounds,"invalid output before FP");
        check(__real_dvm_runtime_real_arithmetic(-1,&first,&second,&output) == exHeap,"invalid operation");
        phase = 0;
        active = 1;
        refusal = 1;
        check(__real_dvm_runtime_real_arithmetic(IADDF,&first,&second,&output) == exHeap && !phase,
            "input BUSY unchanged");
        refusal = 2;
        check(__real_dvm_runtime_real_arithmetic(IADDF,&first,&second,&output) == exHeap && phase == 2 && output == 2.0,
            "output BUSY unchanged");
        refusal = 0;
        phase = 0;
        injected = 1;
        check(!kprotect(unwind,NULL) && phase == 2 && !dvm_native_context_object_count(dvm_native_context_current()),
            "unlocked VM unwind");
        injected = active = 0;
        check(!dvm_native_context_object_count(dvm_native_context_current()),"balanced final guards");
    }
    check(!fesetenv(&environment),"fixture FP restore");
    fprint(1,"PASS actual REAL arithmetic: %ud overlap/boundary cases,unlocked FP,phase update\n",count);
}
'''


def main():
    """Relink matching actual objects; exercise C/JIT guests and both collector polls."""
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    symbols = ['dvm_runtime_root_registry_probe','dvm_managed_three_begin','dvm_managed_ranges_begin',
               'dvm_range_access_end','dvm_runtime_real_branch_capture','dvm_runtime_real_arithmetic_value',
               'dvm_runtime_real_arithmetic_store','dvm_runtime_real_arithmetic','dvm_runtime_power','dvm_bigtable_call_ready','malloc','calloc','free']
    with tempfile.TemporaryDirectory(prefix='actual REAL arithmetic ') as directory:
        work = Path(directory)
        source, image = work/'main.b', work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(source)],check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text(WRAPPER)
        binary = relink(runtime,work,wrapper,0,symbols)
        for mode in ('--jit','--no-jit'):
            for locked in (False,True):
                args = [str(binary),mode,'--gc-worker','--guarded-copies','--managed-owner-probe']
                if locked:
                    args += ['--gc-locked-poll']
                run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=60,
                                     env=dict(os.environ,TMPDIR=str(work)))
                output = run.stdout+run.stderr
                if run.returncode or 'PASS actual REAL arithmetic: 8820 ' not in output or 'PASS REAL arithmetic5' not in output:
                    raise RuntimeError('REAL arithmetic failed: '+output)
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError(output)
        if '--positive-only' not in sys.argv:
            original = (Path(__file__).resolve().parent/'shared_real_arithmetic.c').read_text()
            controls = [
                ('dvm_managed_three_begin(first, sizeof(REAL), second_width ? second : NULL, second_width, NULL, 0,\n        &access)', 'DVM_CELL_ACQUIRED', 'capture phase'),
                ('dvm_range_access_end(&access)', '1', 'unlocked FP'),
                ('dvm_managed_ranges_begin(NULL, 0, output, sizeof(REAL), &access)', 'DVM_CELL_ACQUIRED', 'store phase'),
                ('    dvm_runtime_real_arithmetic_store(output, result);', '    (void)result;', 'overlap result and flags')]
            for index, (before,after,marker) in enumerate(controls):
                if before not in original:
                    raise RuntimeError('REAL control source drift')
                private = work/'shared_real_arithmetic.c'
                private.write_text(original.replace(before,after,1))
                bad = relink(runtime,work,wrapper,index+1,symbols,shared_real_arithmetic_source=private)
                run = subprocess.run([str(bad),'--no-jit','--gc-worker','--guarded-copies',
                    '--managed-owner-probe',str(image)],capture_output=True,text=True,timeout=60,
                    env=dict(os.environ,TMPDIR=str(work)))
                if run.returncode != 10 or 'FAIL real arithmetic: '+marker not in run.stdout+run.stderr:
                    raise RuntimeError('REAL control missed: '+run.stdout+run.stderr)
    print('PASS actual REAL arithmetic: C/JIT guest,both polls,8820 overlap/boundary cases; four semantic controls' if '--positive-only' not in sys.argv else 'PASS actual REAL arithmetic positives')


if __name__ == '__main__':
    main()
