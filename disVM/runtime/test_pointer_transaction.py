# Copyright (c) 2026 bhgv
"""Link and execute the actual guarded pointer-constructor coordinator fixture."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from relink_private import relink
from resident_evidence import evidence


# Guest constructors run after the native coordinator/GC diagnostic. Both C and
# existing x64 tail entries must preserve the same list/string ownership result.
GUEST = '''implement Transaction;
include "sys.m";
Transaction: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    values: list of string;
    for(i := 0; i < 12; i++) { values = "value" :: values; }
    if(len values != 12 || hd values != "value") { raise "bad constructor"; }
    sys->print("PASS transaction guest\\n");
}
'''


CONCURRENT_GUEST = r'''implement Constructors;
include "sys.m";
Constructors: module {init: fn();};

# EN: Build and validate owner-local lists, then publish completion to borrowed done.
# RU: Построить и проверить собственные списки, отправить ответ в заимствованный done.
worker(done: chan of int)
{
    pointers: list of string;
    bytes: list of byte;
    words: list of int;
    longs: list of big;
    reals: list of real;
    raw: list of (int, int);
    mapped: list of (string, int);
    for (i := 0; i < 64; i++)
    {
        pointers = "value" :: pointers;
        bytes = byte i :: bytes;
        words = i :: words;
        longs = big i :: longs;
        reals = real i :: reals;
        raw = (i, i+1) :: raw;
        mapped = ("value", i) :: mapped;
    }
    if (len pointers != 64 || hd pointers != "value" ||
        len bytes != 64 || hd bytes != byte 63 ||
        len words != 64 || hd words != 63 ||
        len longs != 64 || hd longs != big 63 ||
        len reals != 64 || hd reals != real 63 ||
        len raw != 64 || len mapped != 64)
        raise "concurrent constructor changed a list";
    (first, second) := hd raw;
    (text, value) := hd mapped;
    if (first != 63 || second != 64 || text != "value" || value != 63)
        raise "concurrent constructor changed a mapped value";
    done <-= 1;
}

# EN: Start forty residents and wait for one validated result from each.
# RU: Запустить сорок потоков и дождаться проверенного результата каждого.
init()
{
    sys := load Sys Sys->PATH;
    done := chan[40] of int;
    for (i := 0; i < 40; i++)
        spawn worker(done);
    for (i = 0; i < 40; i++)
        if (<-done != 1)
            raise "constructor worker did not complete";
    sys->print("PASS forty concurrent list constructors\n");
}
'''


def main():
    """Borrow matching runtime/compiler/module path; fail on any native diagnostic."""
    runtime, compiler, include = (Path(v).resolve() for v in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='pointer-transaction-') as directory:
        work = Path(directory)
        guest, image = work/'main.b', work/'main.dis'
        guest.write_text(GUEST)
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(guest)],check=True)
        symbols = ['dvm_range_access_begin','dvm_managed_cells_begin','dvm_runtime_root_registry_probe','dvm_runtime_pointer_transaction',
                   'calloc','poolalloc','dvm_refcount_retain_i','destroy']
        binary = relink(runtime,work,root/'test_pointer_transaction.c',0,symbols)
        for mode in ('--no-jit','--jit'):
            for locked in (False,True):
                args = [str(binary),mode,'--guarded-copies','--gc-worker','--managed-owner-probe']
                if locked:
                    args += ['--gc-locked-poll']
                run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=90)
                output = run.stdout+run.stderr
                if run.returncode or 'PASS pointer transaction:' not in output or 'PASS transaction guest' not in output or \
                        'PASS scheduled CONSP source replay' not in output or 'PASS scheduled CONSP tail replay' not in output or \
                        'PASS scheduled CONSP commit replay' not in output:
                    raise RuntimeError(output)
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError(output)
        concurrent_source, concurrent_image = work/'concurrent.b', work/'concurrent.dis'
        concurrent_source.write_text(CONCURRENT_GUEST)
        subprocess.run([str(compiler), '-I', str(include), '-o', str(concurrent_image),
                        str(concurrent_source)], check=True)
        for mode in ('--no-jit', '--jit'):
            for locked in (False, True):
                command = [str(runtime), mode, '--native-spawn', '--stats']
                if locked:
                    command.append('--gc-locked-poll')
                run = subprocess.run(command + [str(concurrent_image)], capture_output=True,
                                     text=True, timeout=60)
                output = run.stdout + run.stderr
                if run.returncode or 'PASS forty concurrent list constructors' not in output:
                    raise RuntimeError(output)
                evidence(output, children=40)
                retired = re.search(r"NATIVERC enqueued=(\d+) released=(\d+) pending=0", output)
                if not retired or int(retired[1]) == 0 or retired[1] != retired[2]:
                    raise RuntimeError("constructor owners did not drain:\n" + output)
        if '--positive-only' not in sys.argv:
            original = (root/'pointer_transaction.c').read_text()
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
            for index,(before,after,marker) in enumerate(controls):
                if original.count(before) != 1:
                    raise RuntimeError('constructor control anchor drift')
                private = work/'pointer_transaction.c'
                private.write_text('extern int dvm_pointer_transaction_control(void);\nextern int dvm_pointer_transaction_error_control(void);\n'+
                                   original.replace(before,after,1))
                bad = relink(runtime,work,root/'test_pointer_transaction.c',index+1,symbols,
                             pointer_transaction_source=private)
                run = subprocess.run([str(bad),'--no-jit','--guarded-copies','--gc-worker',
                    '--managed-owner-probe',str(image)],capture_output=True,text=True,timeout=90)
                if run.returncode != 10 or 'FAIL pointer transaction: '+marker not in run.stdout+run.stderr:
                    raise RuntimeError('constructor control missed: '+run.stdout+run.stderr)
    print('PASS actual guarded CONSP: C/JIT guests,both polls,refusal/cleanup/GC'+
          ('; positives only' if '--positive-only' in sys.argv else '; three source controls'))


if __name__ == '__main__':
    main()
