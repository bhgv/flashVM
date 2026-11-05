"""Exercise actual scalar constructor transaction, C dispatch and selected JIT."""
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = r'''implement Scalars;
include "sys.m";
Scalars: module {init: fn();};
# Build all four scalar list kinds, preserving negative integer and real values.
init() {
    sys := load Sys Sys->PATH;
    b: list of byte;
    w: list of int;
    l: list of big;
    f: list of real;
    for(i := 0; i < 12; i++) {
        b = byte 233 :: b;
        w = -12345 :: w;
        l = big -123456789 :: l;
        f = -3.25 :: f;
    }
    if(hd b != byte 233 || hd w != -12345 || hd l != big -123456789 || hd f != -3.25) {
        raise "scalar head mismatch";
    }
    sys->print("PASS scalar guest\n");
}
'''


def main():
    """Use matching runtime/compiler; exact native failures never count as success."""
    runtime,compiler,include = (Path(v).resolve() for v in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='scalar-cons-') as directory:
        work = Path(directory)
        guest,image = work/'main.b',work/'main.dis'
        guest.write_text(GUEST)
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(guest)],check=True)
        binary = relink(runtime,work,root/'test_scalar_cons.c',0,
            ['dvm_managed_cells_begin','dvm_managed_ranges_begin','dvm_runtime_root_registry_probe','dvm_runtime_scalar_cons','calloc','poolalloc',
             'dvm_refcount_retain_i','destroy'])
        for mode in ('--no-jit','--jit'):
            for locked in (False,True):
                args = [str(binary),mode,'--guarded-copies','--gc-worker','--managed-owner-probe']
                if locked:
                    args += ['--gc-locked-poll']
                run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=90)
                output = run.stdout+run.stderr
                if run.returncode or 'PASS scalar transaction:' not in output or 'PASS scalar guest' not in output or \
                        'PASS scheduled constructor replay' not in output or \
                        'PASS scheduled constructor tail replay' not in output:
                    raise RuntimeError(output)
                if 'runtime error:' in output or 'WARNING: ThreadSanitizer:' in output:
                    raise RuntimeError(output)
        if '--positive-only' not in sys.argv:
            original = (root/'scalar_cons.c').read_text()
            controls = [
                ('dvm_runtime_heap_finish(&state->candidate, 1);',
                 'if (!dvm_pointer_transaction_control()) { dvm_runtime_heap_finish(&state->candidate, 1); }',
                 'successful attempt count'),
                ('index < sizeof(holds)/sizeof(holds[0]);',
                 'index < sizeof(holds)/sizeof(holds[0]) && (!dvm_pointer_transaction_error_control() || !failed);',
                 'all cleanup preserves primary'),
                ('!kprotect(finish_one, holds[index]) && !failed',
                 '!kprotect(finish_one, holds[index]) && (!failed || dvm_pointer_transaction_error_control())',
                 'all cleanup preserves primary')]
            symbols = ['dvm_managed_cells_begin','dvm_managed_ranges_begin','dvm_runtime_root_registry_probe','dvm_runtime_scalar_cons','calloc','poolalloc',
                       'dvm_refcount_retain_i','destroy']
            for index,(before,after,marker) in enumerate(controls):
                if original.count(before) != 1:
                    raise RuntimeError('scalar control anchor drift')
                private = work/'scalar_cons.c'
                private.write_text('extern int dvm_pointer_transaction_control(void);\nextern int dvm_pointer_transaction_error_control(void);\n'+original.replace(before,after,1))
                bad = relink(runtime,work,root/'test_scalar_cons.c',index+1,symbols,scalar_cons_source=private)
                run = subprocess.run([str(bad),'--no-jit','--guarded-copies','--gc-worker',
                    '--managed-owner-probe',str(image)],capture_output=True,text=True,timeout=90)
                if run.returncode != 10 or 'FAIL scalar transaction: '+marker not in run.stdout+run.stderr:
                    raise RuntimeError('scalar control missed: '+run.stdout+run.stderr)
    print('PASS scalar CONSB/W/L/F: C/JIT,both polls,actual transaction/GC/error cases')


if __name__ == '__main__':
    main()
