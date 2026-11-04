"""Run actual raw/scalar candidate preparation with real graph collection."""
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink
from test_pointer_prepare import GUEST


# CLI supplies matching runtime/compiler/include. Test immutable private payload
# extents across C/JIT surrounding guests and both collector polls. These are
# helper tests, not compiled raw/scalar constructor integration evidence.
def main():
    runtime,compiler,include = (Path(v).resolve() for v in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    symbols = ['dvm_runtime_root_registry_probe','dvm_runtime_root_batch_publish',
               'dvm_runtime_root_add','calloc','poolalloc','dvm_refcount_retain_i']
    with tempfile.TemporaryDirectory(prefix='value-prepare-') as directory:
        work = Path(directory)
        guest,image,wrapper = work/'main.b',work/'main.dis',work/'wrapper.c'
        guest.write_text(GUEST)
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(guest)],check=True)
        fixture = (root/'test_value_prepare.c').read_text()
        for size in (0,1,4,8,17,257):
            wrapper.write_text('#define DVM_VALUE_TEST_SIZE '+str(size)+'\n'+fixture)
            binary = relink(runtime,work,wrapper,size,symbols)
            for mode in ('--no-jit','--jit'):
                for locked in (False,True):
                    args = [str(binary),mode,'--guarded-copies','--gc-worker','--managed-owner-probe']
                    if locked:
                        args += ['--gc-locked-poll']
                    run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=90)
                    output = run.stdout+run.stderr
                    if run.returncode or 'PASS value prepare:' not in output:
                        raise RuntimeError(output)
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
        if '--positive-only' not in sys.argv:
            wrapper.write_text('#define DVM_VALUE_TEST_SIZE 17\n'+fixture)
            original = (root/'value_prepare.c').read_text()
            controls = [
                ('state->tail_owner &&','state->tail_owner && !dvm_pointer_prepare_control() &&','refusal restores owners'),
                ('memmove(node->data, state->bytes, state->size);',
                 'if (dvm_pointer_prepare_control()) { memset(node->data, 0, state->size); } else { memmove(node->data, state->bytes, state->size); }','coherent staged node'),
                ('ptrdel(state.node);','if (!dvm_pointer_prepare_control()) { ptrdel(state.node); }','complete rooted candidate'),
                ('dvm_runtime_root_batch_publish(state->roots, &key, 1)',
                 '(dvm_pointer_prepare_control() ? 1 : dvm_runtime_root_batch_publish(state->roots, &key, 1))','injected refusal raises')]
            for index,(before,after,marker) in enumerate(controls):
                if original.count(before) != 1:
                    raise RuntimeError('value control anchor drift')
                private = work/'value_prepare.c'
                private.write_text('extern int dvm_pointer_prepare_control(void);\n'+original.replace(before,after,1))
                bad = relink(runtime,work,wrapper,index+100,symbols,value_prepare_source=private)
                run = subprocess.run([str(bad),'--no-jit','--guarded-copies','--gc-worker',
                    '--managed-owner-probe',str(image)],capture_output=True,text=True,timeout=90)
                if run.returncode != 10 or 'FAIL value prepare: '+marker not in run.stdout+run.stderr:
                    raise RuntimeError('value control missed: '+run.stdout+run.stderr)
    print('PASS value prepare: six extents,seven refusals,stale rebuild,real GC,C/JIT guests,both polls')


if __name__ == '__main__':
    main()
