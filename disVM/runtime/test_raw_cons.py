"""Exercise actual raw constructor transaction, C dispatch and selected JIT."""
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = r'''implement Raw;
include "sys.m";
Raw: module {init: fn();};
Pair: adt { a: int; b: int; };
# Construct an unmapped aggregate list through actual CONSM and read it back.
init() {
    sys := load Sys Sys->PATH;
    values: list of Pair;
    pair := Pair(-12345,77);
    for(i := 0; i < 12; i++) { values = pair :: values; }
    result := hd values;
    if(result.a != -12345 || result.b != 77) { raise "raw head mismatch"; }
    sys->print("PASS raw guest\n");
}
'''


def main():
    """Use matching runtime/compiler; exact native failures never count as success."""
    runtime,compiler,include = (Path(v).resolve() for v in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='raw-cons-') as directory:
        work = Path(directory)
        guest,image = work/'main.b',work/'main.dis'
        guest.write_text(GUEST)
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(guest)],check=True)
        binary = relink(runtime,work,root/'test_raw_cons.c',0,
            ['dvm_managed_ranges_begin','dvm_runtime_root_registry_probe','dvm_runtime_raw_cons','calloc','poolalloc',
             'dvm_refcount_retain_i','destroy','malloc','free'])
        for mode in ('--no-jit','--jit'):
            for locked in (False,True):
                args = [str(binary),mode,'--guarded-copies','--gc-worker','--managed-owner-probe']
                if locked:
                    args += ['--gc-locked-poll']
                run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=90)
                output = run.stdout+run.stderr
                if run.returncode or 'PASS raw transaction:' not in output or 'PASS raw guest' not in output or \
                        'PASS scheduled constructor replay' not in output:
                    raise RuntimeError(output)
                if 'runtime error:' in output or 'WARNING: ThreadSanitizer:' in output:
                    raise RuntimeError(output)
        if '--positive-only' not in sys.argv:
            original = (root/'raw_cons.c').read_text()
            controls = [
                ('dvm_runtime_heap_finish(&state->candidate, 1);',
                 'if (!dvm_pointer_transaction_control()) { dvm_runtime_heap_finish(&state->candidate, 1); }',
                 'successful attempt count'),
                ('index < sizeof(holds)/sizeof(holds[0]);',
                 'index < sizeof(holds)/sizeof(holds[0]) && (!dvm_pointer_transaction_error_control() || !failed);',
                 'all cleanup preserves primary'),
                ('free(state->snapshot);','if (!dvm_pointer_transaction_control()) { free(state->snapshot); }','native snapshot drained once'),
                ('!kprotect(finish_one, holds[index]) && !failed',
                 '!kprotect(finish_one, holds[index]) && (!failed || dvm_pointer_transaction_error_control())',
                 'all cleanup preserves primary')]
            symbols = ['dvm_managed_ranges_begin','dvm_runtime_root_registry_probe','dvm_runtime_raw_cons','calloc','poolalloc',
                       'dvm_refcount_retain_i','destroy','malloc','free']
            for index,(before,after,marker) in enumerate(controls):
                if original.count(before) != 1:
                    raise RuntimeError('raw control anchor drift')
                private = work/'raw_cons.c'
                private.write_text('extern int dvm_pointer_transaction_control(void);\nextern int dvm_pointer_transaction_error_control(void);\n'+original.replace(before,after,1))
                bad = relink(runtime,work,root/'test_raw_cons.c',index+1,symbols,raw_cons_source=private)
                run = subprocess.run([str(bad),'--no-jit','--guarded-copies','--gc-worker',
                    '--managed-owner-probe',str(image)],capture_output=True,text=True,timeout=90)
                if run.returncode != 10 or 'FAIL raw transaction: '+marker not in run.stdout+run.stderr:
                    raise RuntimeError('raw control missed: '+run.stdout+run.stderr)
    print('PASS raw CONSM: C/JIT,both polls,actual transaction/snapshot/GC/error cases')


if __name__ == '__main__':
    main()
