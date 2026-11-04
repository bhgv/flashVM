"""Execute rooted typed-list preparation against actual runtime and collectors."""
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink
from test_raw_cons import GUEST


# Matching runtime/compiler/include are explicit. C/JIT denotes surrounding guest,
# not a new compiled CONSMP; preparation is an internal C prerequisite.
def main():
    runtime,compiler,include = (Path(v).resolve() for v in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    symbols = ['dvm_runtime_root_registry_probe','malloc','calloc','poolalloc',
               'dvm_runtime_root_add','dvm_runtime_root_batch_publish',
               'dvm_refcount_retain_i','dvm_refcount_retain_u','dvm_gc_field_mark']
    with tempfile.TemporaryDirectory(prefix='mapped-prepare-') as directory:
        work = Path(directory)
        guest,image = work/'main.b',work/'main.dis'
        guest.write_text(GUEST)
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(guest)],check=True)
        binary = relink(runtime,work,root/'test_mapped_prepare.c',0,symbols)
        for mode in ('--no-jit','--jit'):
            for locked in (False,True):
                args = [str(binary),mode,'--guarded-copies','--gc-worker','--managed-owner-probe']
                if locked:
                    args += ['--gc-locked-poll']
                run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=90)
                output = run.stdout+run.stderr
                if run.returncode or 'PASS mapped prepare:' not in output:
                    raise RuntimeError(output)
                if 'runtime error:' in output or 'WARNING: ThreadSanitizer:' in output:
                    raise RuntimeError(output)
        if '--positive-only' not in sys.argv:
            original = (root/'mapped_prepare.c').read_text()
            controls = [
                ('if (state->copied)', 'if (state->copied && !dvm_mapped_prepare_control())',
                 'all refused owners restored'),
                ('    ptrdel(state->node);\n    state->node = NULL;',
                 '    if (!dvm_mapped_prepare_control()) { ptrdel(state->node); }\n    state->node = NULL;',
                 'candidate root before monitor'),
                ('dvm_runtime_root_batch_publish(state->roots, &key, 1)',
                 '(dvm_mapped_prepare_control() ? 1 : dvm_runtime_root_batch_publish(state->roots, &key, 1))',
                 'candidate root before monitor')]
            for index,(before,after,marker) in enumerate(controls):
                if original.count(before) != 1:
                    raise RuntimeError('mapped preparation control anchor drift')
                private = work/'mapped_prepare.c'
                private.write_text('extern int dvm_mapped_prepare_control(void);\n'+original.replace(before,after,1))
                bad = relink(runtime,work,root/'test_mapped_prepare.c',index+1,symbols,mapped_prepare_source=private)
                run = subprocess.run([str(bad),'--no-jit','--guarded-copies','--gc-worker',
                    '--managed-owner-probe',str(image)],capture_output=True,text=True,timeout=90)
                if run.returncode != 10 or 'FAIL mapped prepare: '+marker not in run.stdout+run.stderr:
                    raise RuntimeError('mapped preparation control missed: '+run.stdout+run.stderr)
    print('PASS mapped prepare: actual typed graph,17 refusals,C/JIT guests,both polls')


if __name__ == '__main__':
    main()
