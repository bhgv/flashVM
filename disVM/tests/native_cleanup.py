"""Run actual native error macros, hosted transfer bodies and ownership callbacks."""
from pathlib import Path
import importlib.util
import os
import shlex
import subprocess
import tempfile
from owner_scope import weaken


# Extract one exact hosted function, preserving its body and real calls. text
# and signature borrowed; return source copy or raise if marker is absent.
def function(text, signature):
    begin = text.index(signature)
    end = text.index('\n}\n', begin) + 3
    return text[begin:end]


# Generate the SAME runtime dat.h view and compile actual adapters/ports. No CLI
# arguments/result; failures propagate. Controls require semantic status10 only.
def main():
    root = Path(__file__).resolve().parents[1]
    fvm = root.parent / 'flashVM'
    spec = importlib.util.spec_from_file_location('native_errors', root / 'host/prepare_native_errors.py')
    prepare = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(prepare)
    dat = prepare.error_header((root / 'upstream/emu/port/dat.h').read_text())
    original = (root / 'runtime/main.c').read_text()
    signatures = ('static char *owner_error(void)', 'void error(char *message)', 'void nexterror(void)',
                  'int kprotect(int (*callback)(void *), void *argument)')
    bodies = '\n\n'.join(function(original, signature) for signature in signatures)
    # Extracted nexterror must link the actual transfer module, never a test stub.
    fixture = (root / 'tests/native_cleanup.c').read_text().replace('PRODUCTION_ERROR_FUNCTIONS', '#include "native_jump.h"\n'+bodies)
    originals = {name: (root / ('host/' + name)).read_text() for name in ('native_context.c', 'native_registers.c')}
    controls = {
        'boundary': ('native_context.c', 'int dvm_native_cleanup_unwind(', 'context->cleanup_newest > mark->serial', 'context->cleanup_newest > context->cleanup_serial'),
        'mark': ('native_context.c', 'int dvm_native_cleanup_mark(', 'mark->serial = context->cleanup_serial;', 'mark->serial = 0;'),
        'normal-pop': ('native_registers.c', 'int dvm_native_error_pop(', '!dvm_native_cleanup_clean(&process->error_mark[process->Proc.nerr - 1])', '0'),
        'guard': ('native_context.c', 'int dvm_native_cleanup_unwind(', 'context->object_locks ||', '0 ||'),
        'reentry': ('native_context.c', 'int dvm_native_cleanup_link(', 'context->cleanup_running ||', '0 ||'),
    }
    with tempfile.TemporaryDirectory(prefix='native error cleanup ') as directory:
        work = Path(directory)
        (work / 'dat.h').write_text(dat)
        (work / 'test.c').write_text(fixture)
        for backend in ('native', 'locked'):
            for control in (None, *controls):
                sources = dict(originals)
                if control:
                    name, signature, old, new = controls[control]
                    sources[name] = weaken(sources[name], signature, old, new)
                for name, source in sources.items():
                    (work / name).write_text(source)
                command = shlex.split(os.environ.get('CC', 'cc'))
                command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
                command += ['-std=c99', '-pthread', '-DDVM_NATIVE_REGISTERS=1']
                if backend == 'locked':
                    command += ['-DDVM_REFCOUNT_FORCE_LOCKED=1', '-DDVM_OBJECT_FORCE_LOCKED=1']
                command += ['-I' + str(path) for path in (work, root / 'runtime', root / 'host', root / 'host/include', root / 'upstream/include',
                    root / 'upstream/libinterp', root / 'upstream/emu/port', root / 'include', fvm / 'ports')]
                command += [str(work / 'test.c'), str(work / 'native_context.c'), str(work / 'native_registers.c'), str(root / 'runtime/native_jump.c')]
                command += [str(root / ('host/' + name)) for name in ('owner_scope.c', 'object_guard.c', 'refcount.c', 'native_owner.c', 'native_world.c')]
                command += [str(fvm / 'ports/linux/thread.c'), str(fvm / 'ports/linux/channel_sync.c')]
                binary = work / 'cleanup'
                built = subprocess.run(command + ['-o', str(binary)], capture_output=True, text=True)
                if built.returncode:
                    raise RuntimeError('cleanup compile failed: ' + str(control) + '\n' + built.stdout + built.stderr)
                run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=45)
                if control:
                    if run.returncode != 10 or 'FAIL native cleanup predicate:' not in run.stderr:
                        raise RuntimeError('cleanup control missed: ' + backend + '/' + control + run.stdout + run.stderr)
                elif run.returncode:
                    raise RuntimeError(run.stdout + run.stderr)
                else:
                    print(backend + ': ' + run.stdout, end='')
        print('PASS ten semantic cleanup controls, native/locked; no managed unwind/root claim')


if __name__ == '__main__':
    main()
