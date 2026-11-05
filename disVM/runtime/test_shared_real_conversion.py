"""Actual guarded FP conversions versus retained-source differential and boundary oracles."""
from pathlib import Path
import os
import hashlib
from prepare_shared_real_conversion import EXPECTED
import re
import subprocess
import sys
import tempfile
from relink_private import relink

OPS = ('cvtrf','cvtfr','cvtwf','cvtlf','cvtfw','cvtfl')  # Existing six scalar conversions.
GUEST = '''implement Conversions;
include "sys.m";
Conversions: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN FP conversions4\\n");
    ints := array[] of {3,7};
    bigs := array[] of {big 3,big 7};
    reals := array[] of {2.5,3.5};
    w := ints[0];
    b := bigs[0];
    f := reals[0];
    a := real w;
    c := real b;
    d := int f;
    e := big f;
    if (a != 3.0 || c != 3.0 || d != 3 || e != big 3)
    {
        raise "FP conversion result";
    }
    sys->print("PASS FP conversions4\\n");
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
#include "shared_bytes.h"
#include "managed_owner.h"
#include "shared_real_conversion.h"
#include "real_conversion_value.h"
#include "shared_bytes.h"
#include "bytes_fast.h"
#include "disvm/native_context.h"
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <stdint.h>

static __thread int active; /* Exact private helper operation interval only. */
static __thread int phase; /* Source1, unlocked evaluation2, output3, finished4. */
static __thread int refusal; /* Input1/output2 deliberate admission refusal. */
static __thread int injected; /* Throw at the fully unlocked evaluation boundary. */
static __thread int change; /* Change a live source after capture in one test. */
static __thread void *source_cell; /* Independently live input bytes through invoke. */
static __thread void *output_cell; /* Independently live output bytes through invoke. */
static __thread size_t input_width; /* Independent expected native source width. */
static __thread size_t output_width; /* Independent expected native result width. */
static __thread unsigned int copies; /* Actual byte-capture/publication count. */
static __thread unsigned int executions; /* Actual conversion helper calls. */
static const int operations[6] = {ICVTRF,ICVTFR,ICVTWF,ICVTLF,ICVTFW,ICVTFL}; /* Existing opcodes. */
static const size_t inputs[6] = {sizeof(SREAL),sizeof(REAL),sizeof(WORD),sizeof(LONG),sizeof(REAL),sizeof(REAL)}; /* Independent widths. */
static const size_t outputs[6] = {sizeof(REAL),sizeof(SREAL),sizeof(REAL),sizeof(REAL),sizeof(WORD),sizeof(LONG)}; /* Independent widths. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
DVM_CellResult __real_dvm_managed_ranges_begin(const void *,size_t,void *,size_t,DVM_RangeAccess *);
int __real_dvm_range_access_end(DVM_RangeAccess *);

const char *__real_dvm_runtime_real_conversion_value(int,const void *,void *);
const char *__real_dvm_runtime_real_conversion(int,const void *,void *);
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void __real_free(void *);

REFERENCES


/* Fail only at explicit semantic10, before unsafe continuation or collection. */
static void check(int valid,const char *message)
{
    if (!valid)
    {
        fprint(2,"FAIL real conversion: %s\n",message);
        exit(10);
    }
}


/* Observe genuine helper execution for the direct native C dispatcher test. */
const char *__wrap_dvm_runtime_real_conversion(int opcode,const void *source,void *output)
{
    executions++;
    return __real_dvm_runtime_real_conversion(opcode,source,output);
}


/* Check actual canonical guard cardinality from independent per-byte identities.
 * Count is not proof of exact stripe ownership; the real implementation is used. */
static void coverage(const void *address,size_t bytes)
{
    unsigned char keys[256] = {0};
    size_t index;
    unsigned int count = 0;
    for (index = 0; index < bytes; index++)
    {
        unsigned int key = (unsigned int)((((uintptr_t)address+index)/sizeof(void *))%256);
        if (!keys[key])
        {
            keys[key] = 1;
            count++;
        }
    }
    check(count == dvm_native_context_object_count(dvm_native_context_current()),"guard cardinality");
}


/* Input and output must occupy different complete intervals; keep real exclusion. */
DVM_CellResult __wrap_dvm_managed_ranges_begin(const void *source,size_t n,void *output,size_t m,DVM_RangeAccess *access)
{
    DVM_CellResult result;
    if (active)
    {
        if (!phase)
        {
            check(source == source_cell && n == input_width && !output && !m,"source geometry");
            if (refusal == 1)
            {
                return DVM_CELL_BUSY;
            }
        }
        else
        {
            check(phase == 2 && !source && !n && output == output_cell && m == output_width,"output geometry");
            if (refusal == 2)
            {
                return DVM_CELL_BUSY;
            }
        }
    }
    result = __real_dvm_managed_ranges_begin(source,n,output,m,access);
    if (active)
    {
        check(result == DVM_CELL_ACQUIRED,"actual admission");
        phase++;
        if (source)
        {
            copies++;
        }
        coverage(source ? source : output,source ? n : m);
    }
    return result;
}


/* Verify actual full release before FP or VM errors; no fake successful end. */
int __wrap_dvm_range_access_end(DVM_RangeAccess *access)
{
    int result = __real_dvm_range_access_end(access);
    if (active)
    {
        check(result && !dvm_native_context_object_count(dvm_native_context_current()),"complete release");
        phase++;
    }
    return result;
}


void __real_dvm_runtime_byte_transfer(void *,const void *,size_t);


/* The common publication path must hold the complete destination guard. */
void __wrap_dvm_runtime_byte_transfer(void *output,const void *source,size_t bytes)
{
    if (active)
    {
        check(phase == 3 && output == output_cell && bytes == output_width,"copy phase");
        coverage(output,bytes);
        copies++;
    }
    __real_dvm_runtime_byte_transfer(output,source,bytes);
}


/* All native FP conversion and any VM unwind occur with zero shared guards. */
const char *__wrap_dvm_runtime_real_conversion_value(int opcode,const void *source,void *output)
{
    if (active)
    {
        check(phase == 2 && copies == 1 && !dvm_native_context_object_count(dvm_native_context_current()),"unlocked conversion");
        if (change)
        {
            memset(source_cell,0,input_width);
        }
        if (injected)
        {
            error(exHeap);
        }
    }
    return __real_dvm_runtime_real_conversion_value(opcode,source,output);
}


/* Native preparation is allowed around, never inside, conversion execution. */
void *__wrap_malloc(size_t bytes)
{
    check(!active,"no allocation");
    return __real_malloc(bytes);
}


/* Observe actual calloc rather than replacing allocation semantics. */
void *__wrap_calloc(size_t n,size_t bytes)
{
    check(!active,"no allocation");
    return __real_calloc(n,bytes);
}


/* No metadata or managed reference retirement belongs inside this scalar helper. */
void __wrap_free(void *value)
{
    check(!active,"no allocation");
    __real_free(value);
}


/* Invoke actual helper on live mapped bytes; no owner/root is fabricated. */
static const char *invoke(unsigned int index,void *source,void *output)
{
    const char *problem;
    source_cell = source;
    output_cell = output;
    input_width = inputs[index];
    output_width = outputs[index];
    phase = 0;
    copies = 0;
    active = 1;
    problem = __real_dvm_runtime_real_conversion(operations[index],source,output);
    active = 0;
    check(!dvm_native_context_object_count(dvm_native_context_current()),"final guards");
    if (!refusal)
    {
        check((!problem && phase == 4 && copies == 2) || (problem == exInval && phase == 2 && copies == 1),"complete conversion");
    }
    return problem;
}


/* Actual kprotect callback throws at unlocked evaluation; argument is unused. */
static int unwind(void *unused)
{
    REAL value = 1.5;
    WORD result = 0;
    (void)unused;
    (void)invoke(4,&value,&result);
    return 1;
}


/* Native cells remain live on this ACTIVE owner's C stack. Differential oracle
 * is a checked retained-source copy, not a second invocation of the new helper.
 * Mask FP traps only for the fixture; preserve the previous environment/REG. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    unsigned char actual[96];
    unsigned char expected[96];
    REG saved;
    fenv_t environment;
    unsigned int op;
    unsigned int a;
    unsigned int d;
    unsigned int cases = 0;
    __real_dvm_runtime_root_registry_probe(workers);
    memmove(&saved,&DVM_R,sizeof(saved));
    check(!feholdexcept(&environment),"fixture FP setup");
    for (op = 0; op < 6; op++)
    {
        for (a = 0; a < 24; a++)
        {
            for (d = 0; d < 24; d++)
            {
                unsigned int i;
                int flags;
                const char *wanted;
                const char *problem;
                for (i = 0; i < sizeof(actual); i++)
                {
                    actual[i] = (unsigned char)(19*i+7*a+op);
                }
                memmove(expected,actual,sizeof(expected));
                feclearexcept(FE_ALL_EXCEPT);
                wanted = references[op](expected+a,expected+d);
                flags = fetestexcept(FE_ALL_EXCEPT);
                feclearexcept(FE_ALL_EXCEPT);
                problem = invoke(op,actual+a,actual+d);
                check(problem == wanted && fetestexcept(FE_ALL_EXCEPT) == flags &&
                    !memcmp(actual,expected,sizeof(actual)),"oracle result and flags");
                cases++;
            }
        }
    }
    {
        REAL values[] = {0.0,-0.0,0.49,-0.49,0.5,-0.5,1.5,-1.5};
        LONG results[] = {0,0,0,0,1,-1,2,-2};
        unsigned int i;
        for (op = 4; op < 6; op++)
        {
            for (i = 0; i < sizeof(values)/sizeof(values[0]); i++)
            {
                LONG output = 123;
                check(!invoke(op,&values[i],&output) && output == results[i],"known rounded integer");
                cases++;
            }
        }
    }
    {
        uint64_t bits[] = {UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),
            UINT64_C(0x7ff8000000000001),UINT64_C(0x43e0000000000000)};
        unsigned int i;
        for (op = 4; op < 6; op++)
        {
            for (i = 0; i < sizeof(bits)/sizeof(bits[0]); i++)
            {
                REAL value;
                LONG output = 321;
                memmove(&value,&bits[i],sizeof(value));
                check(invoke(op,&value,&output) == exInval && output == 321,"invalid rounded integer unchanged");
                cases++;
            }
        }
    }
    {
        uint64_t real_bits[] = {UINT64_C(0),UINT64_C(0x8000000000000000),
            UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),
            UINT64_C(0x7ff8000000000001),UINT64_C(0x7ff0000000000001),
            UINT64_C(0x7fefffffffffffff),UINT64_C(1)};
        uint32_t small_bits[] = {0,UINT32_C(0x80000000),UINT32_C(0x7f800000),
            UINT32_C(0xff800000),UINT32_C(0x7fc00001),UINT32_C(0x7f800001),
            UINT32_C(0x7f7fffff),UINT32_C(1)};
        unsigned int i;
        for (op = 0; op < 2; op++)
        {
            for (i = 0; i < 8; i++)
            {
                unsigned char input[8] = {0};
                unsigned char output[8];
                unsigned char wanted[8];
                int flags;
                if (!op)
                {
                    memmove(input,&small_bits[i],sizeof(small_bits[i]));
                }
                else
                {
                    memmove(input,&real_bits[i],sizeof(real_bits[i]));
                }
                memset(output,0xa5,sizeof(output));
                memmove(wanted,output,sizeof(output));
                feclearexcept(FE_ALL_EXCEPT);
                check(!references[op](input,wanted),"boundary baseline");
                flags = fetestexcept(FE_ALL_EXCEPT);
                feclearexcept(FE_ALL_EXCEPT);
                check(!invoke(op,input,output) && fetestexcept(FE_ALL_EXCEPT) == flags &&
                    !memcmp(output,wanted,sizeof(output)),"IEEE conversion bits and flags");
                cases++;
            }
        }
    }
    for (op = 0; op < 6; op++)
    {
        unsigned char input[16] = {0};
        unsigned char output[16] = {0};
        unsigned int before = executions;
        REAL value = 2.0;
        SREAL small = 2.0;
        WORD word = 2;
        LONG big = 2;
        if (op == 0)
        {
            memmove(input,&small,sizeof(small));
        }
        else if (op == 2)
        {
            memmove(input,&word,sizeof(word));
        }
        else if (op == 3)
        {
            memmove(input,&big,sizeof(big));
        }
        else
        {
            memmove(input,&value,sizeof(value));
        }
        DVM_R.s = input;
        DVM_R.d = output;
        check(dvm_runtime_bytes_fast(&DVM_R,operations[op]) == 1 && executions == before+1,"six native dispatch completions");
    }
    {
        REAL value = 2.5;
        WORD result = 123;
        change = 1;
        check(!invoke(4,&value,&result) && value == 0.0 && result == 3,"saved value after phase update");
        change = 0;
        value = 2.5;
        refusal = 1;
        check(invoke(4,&value,&result) == exHeap && !phase && result == 3,"input BUSY unchanged");
        refusal = 2;
        check(invoke(4,&value,&result) == exHeap && phase == 2 && result == 3,"output BUSY unchanged");
        refusal = 0;
        check(__real_dvm_runtime_real_conversion(-1,&value,&result) == exHeap,"invalid opcode");
        check(__real_dvm_runtime_real_conversion(ICVTFW,&value,NULL) == exBounds,"invalid output");
        injected = 1;
        check(!kprotect(unwind,NULL) && phase == 2 && !dvm_native_context_object_count(dvm_native_context_current()),"unlocked VM unwind");
        injected = active = 0;
    }
    memmove(&DVM_R,&saved,sizeof(saved));
    check(!fesetenv(&environment),"fixture FP restore");
    fprint(1,"PASS actual FP conversion: %ud differential/boundary cases,six native dispatch entries\n",cases);
}
'''


def references(source):
    """Derive unadapted retained bodies with only argument/diagnostic plumbing changed.

    The checked source is never modified. No new helper or private admission table
    is semantic authority. Geometry remains original; known boundary tests are
    independent of this differential baseline and its rounding code.
    """
    if not source.exists():
        raise RuntimeError('explicit retained executor unavailable for differential oracle')
    text = source.read_text()
    bodies = []
    for name in OPS:
        matches = list(re.finditer(r'OP\('+name+r'\)\n\{.*?\n\}',text,re.S))
        if len(matches) != 1:
            raise RuntimeError('retained conversion drift: '+name)
        if hashlib.sha256(matches[0].group().replace('R.','DVM_R.').encode()).hexdigest() != EXPECTED[name]:
            raise RuntimeError('unreviewed retained conversion: '+name)
        body = matches[0].group().split('\n',1)[1]
        body = body.replace('R.s','cell_source').replace('R.d','cell_output')
        body = body.replace('error(exBounds);','return exBounds;').replace('error(exInval);','return exInval;')
        body = body[:-1]+'    return NULL;\n}'
        bodies.append('/* Retained-source '+name+' differential; borrowed mapped cells, static error/NULL. */\nstatic const char *reference_'+name+'(const void *cell_source,void *cell_output)\n'+body)
    bodies.append('static const char *(*const references[6])(const void *,void *) = {'+
                  ','.join('reference_'+name for name in OPS)+'}; /* Retained-source baseline entries. */')
    return '\n\n\n'.join(bodies)


def main():
    """Run actual matching runtime/retained-source controls in C/JIT and both polls."""
    runtime,compiler,include = (Path(v).resolve() for v in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    symbols = ['dvm_runtime_root_registry_probe','dvm_managed_ranges_begin','dvm_range_access_end',
               'dvm_runtime_real_conversion_value','dvm_runtime_byte_transfer',
               'dvm_runtime_real_conversion','malloc','calloc','free']
    with tempfile.TemporaryDirectory(prefix='actual FP conversion ') as directory:
        work = Path(directory)
        source,image = work/'main.b',work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(source)],check=True)
        wrapper = work/'wrapper.c'
        # Source location follows the explicitly supplied original include tree.
        executor = include.parent/'libinterp/xec.c'
        if not executor.exists():
            executor = root.parent/'conversion-oracle/xec.c'
        wrapper.write_text(WRAPPER.replace('REFERENCES',references(executor)))
        binary = relink(runtime,work,wrapper,0,symbols)
        for mode in ('--jit','--no-jit'):
            for locked in (False,True):
                args = [str(binary),mode,'--gc-worker','--guarded-copies','--managed-owner-probe']
                if locked:
                    args += ['--gc-locked-poll']
                run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=60,
                                     env=dict(os.environ,TMPDIR=str(work)))
                output = run.stdout+run.stderr
                if run.returncode or 'PASS actual FP conversion: 3496 ' not in output or 'PASS FP conversions4' not in output:
                    raise RuntimeError(output)
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError(output)
        if '--positive-only' not in sys.argv:
            original = (root/'shared_real_conversion.c').read_text()
            controls = [
                ('dvm_runtime_bytes_snapshot(source, captured, input)',
                 '(memmove(captured, source, input), NULL)', 'unlocked conversion'),
                ('dvm_runtime_bytes_snapshot(source, captured, input)',
                 'dvm_runtime_bytes_snapshot(source, captured, 0)', 'unlocked conversion'),
                ('return dvm_runtime_bytes_publish(result, destination, output);',
                 'dvm_runtime_byte_transfer(destination, result, output); return NULL;', 'copy phase'),
                ('return dvm_runtime_bytes_publish(result, destination, output);',
                 'return dvm_runtime_bytes_publish(result, destination, 0);', 'complete conversion')]
            for index,(before,after,marker) in enumerate(controls):
                if before not in original:
                    raise RuntimeError('conversion control anchor drift')
                private = work/'shared_real_conversion.c'
                private.write_text(original.replace(before,after,1))
                bad = relink(runtime,work,wrapper,index+1,symbols,shared_real_conversion_source=private)
                run = subprocess.run([str(bad),'--no-jit','--gc-worker','--guarded-copies',
                    '--managed-owner-probe',str(image)],capture_output=True,text=True,timeout=60,
                    env=dict(os.environ,TMPDIR=str(work)))
                if run.returncode != 10 or 'FAIL real conversion: '+marker not in run.stdout+run.stderr:
                    raise RuntimeError('conversion omission missed: '+run.stdout+run.stderr)
    print('PASS actual FP conversions: 3496 differential/boundary cases,six native entries,C/JIT guest,both polls'+
          ('; positives only' if '--positive-only' in sys.argv else '; four semantic controls'))


if __name__ == '__main__':
    main()
