"""Exercise actual mapped constructor transaction, C dispatch and selected JIT."""
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = r'''implement Mapped;
include "sys.m";
Mapped: module {init: fn();};
Pair: adt { label: string; count: int; };
# A mapped aggregate list drives the real CONSMP C operation and x64 tail.
init() {
    sys := load Sys Sys->PATH;
    values: list of Pair;
    pair := Pair("mapped",77);
    for(i := 0; i < 12; i++) { values = pair :: values; }
    result := hd values;
    if(result.label != "mapped" || result.count != 77) { raise "mapped head mismatch"; }
    sys->print("PASS mapped guest\n");
}
'''



def main():
    """Use matching runtime/compiler; exact native failures never count as success."""
    runtime,compiler,include = (Path(v).resolve() for v in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='mapped-cons-') as directory:
        work = Path(directory)
        guest,image = work/'main.b',work/'main.dis'
        guest.write_text(GUEST)
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(guest)],check=True)
        binary = relink(runtime,work,root/'test_mapped_cons.c',0,
            ['dvm_managed_ranges_begin','dvm_runtime_root_registry_probe','dvm_runtime_mapped_cons','calloc','poolalloc',
             'dvm_refcount_retain_i','destroy'])
        for mode in ('--no-jit','--jit'):
            for locked in (False,True):
                args = [str(binary),mode,'--guarded-copies','--gc-worker','--managed-owner-probe']
                if locked:
                    args += ['--gc-locked-poll']
                run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=90)
                output = run.stdout+run.stderr
                if run.returncode or 'PASS mapped cons:' not in output or 'PASS mapped guest' not in output or \
                        'PASS scheduled constructor replay' not in output:
                    raise RuntimeError(output)
                if 'runtime error:' in output or 'WARNING: ThreadSanitizer:' in output:
                    raise RuntimeError(output)
        if '--positive-only' not in sys.argv:
            original = (root/'mapped_cons.c').read_text()
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
            symbols = ['dvm_managed_ranges_begin','dvm_runtime_root_registry_probe','dvm_runtime_mapped_cons','calloc','poolalloc',
                       'dvm_refcount_retain_i','destroy']
            for index,(before,after,marker) in enumerate(controls):
                if original.count(before) != 1:
                    raise RuntimeError('mapped control anchor drift')
                private = work/'mapped_cons.c'
                private.write_text('extern int dvm_pointer_transaction_control(void);\nextern int dvm_pointer_transaction_error_control(void);\n'+original.replace(before,after,1))
                bad = relink(runtime,work,root/'test_mapped_cons.c',index+1,symbols,mapped_cons_source=private)
                run = subprocess.run([str(bad),'--no-jit','--guarded-copies','--gc-worker',
                    '--managed-owner-probe',str(image)],capture_output=True,text=True,timeout=90)
                if run.returncode != 10 or 'FAIL mapped cons: '+marker not in run.stdout+run.stderr:
                    raise RuntimeError('mapped control missed: '+run.stdout+run.stderr)
    print('PASS mapped CONSMP: C/JIT,both polls,traced snapshot/retry/cleanup/GC')


if __name__ == '__main__':
    main()
