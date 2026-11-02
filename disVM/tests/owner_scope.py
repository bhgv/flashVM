"""Build/run real native reference scopes; semantic controls must exit with 10."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile


# Mutate exactly one function in a private copy. source/signature/old/new are
# borrowed strings; return changed source or raise on marker drift, no I/O.
def weaken(source, signature, old, new):
    begin = source.index(signature)
    end = source.index('\n}\n', begin) + 3
    part = source[begin:end]
    if part.count(old) != 1:
        raise RuntimeError('owner scope control marker drift: ' + signature)
    return source[:begin] + part.replace(old, new) + source[end:]


# Use repository or installed source closure, actual OS ports and TLS contexts.
# No arguments/result; compiler/process/signal/timeout failures raise. Negative
# copies are temporary and never alter production files or vendor snapshots.
def main():
    root = Path(__file__).resolve().parents[2]
    installed = not (root / 'disVM/host/owner_scope.c').exists()
    host = root / ('source/host' if installed else 'disVM/host')
    tests = root / ('source/tests' if installed else 'disVM/tests')
    include = root / ('include' if installed else 'disVM/include')
    ports = root / ('source/ports' if installed else 'flashVM/ports')
    originals = {name: (host / name).read_text() for name in ('owner_scope.c', 'native_context.c')}
    controls = {
        'retain': ('owner_scope.c', 'int dvm_owner_scope_retain(', 'retained = ops->retain(object);', 'retained = 1;'),
        'cleanup': ('owner_scope.c', 'int dvm_owner_scope_close(', 'ops->drop(object);', '(void)ops;\n        (void)object;'),
        'export': ('owner_scope.c', 'int dvm_owner_scope_export(', 'detach(entry);', '(void)entry;'),
        'accept': ('owner_scope.c', 'int dvm_owner_scope_accept(', 'transfer->object = 0;', '(void)transfer;'),
        'unbind': ('native_context.c', 'int dvm_native_context_unbind(', '&& context->owner_scopes', '&& 0'),
        'affinity': ('owner_scope.c', 'static int available(', 'scope->context == context', 'scope->context != 0'),
        'guard': ('owner_scope.c', 'static int available(', '!dvm_native_context_object_count(context)', '1'),
    }
    with tempfile.TemporaryDirectory(prefix='native owner scopes ') as directory:
        work = Path(directory)
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
                command += ['-std=c99', '-Wall', '-Wextra', '-Werror', '-pthread']
                if backend == 'locked':
                    command += ['-DDVM_REFCOUNT_FORCE_LOCKED=1', '-DDVM_OBJECT_FORCE_LOCKED=1']
                command += ['-I' + str(include), '-I' + str(ports), '-I' + str(host)]
                command += [str(tests / 'owner_scope.c'), str(work / 'owner_scope.c'),
                    str(work / 'native_context.c'), str(host / 'refcount.c'), str(host / 'object_guard.c'),
                    str(host / 'native_owner.c'), str(host / 'native_world.c'),
                    str(ports / 'linux/thread.c'), str(ports / 'linux/channel_sync.c')]
                binary = work / 'scopes'
                built = subprocess.run(command + ['-o', str(binary)], capture_output=True, text=True)
                if built.returncode:
                    raise RuntimeError('scope compile failed: ' + str(control) + '\n' + built.stdout + built.stderr)
                run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=45)
                if control:
                    if run.returncode != 10 or 'FAIL owner scope predicate:' not in run.stderr:
                        raise RuntimeError('scope control missed: ' + backend + '/' + control + run.stdout + run.stderr)
                elif run.returncode:
                    raise RuntimeError(run.stdout + run.stderr)
                else:
                    print(backend + ': ' + run.stdout, end='')
        print('PASS fourteen semantic scope controls; no managed root/automatic unwinder claim')


if __name__ == '__main__':
    main()
