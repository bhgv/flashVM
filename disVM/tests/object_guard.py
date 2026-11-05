"""Compile actual object guards, contexts/worlds and native OS ports."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile


# Change one actual function in an isolated source copy. Controls must fail a
# semantic predicate, never by timeout/signal/compiler or sanitizer failure.
def weaken(source, signature, old, new):
    begin = source.index(signature)
    end = source.index('\n}\n', begin) + 3
    part = source[begin:end]
    if part.count(old) != 1:
        raise RuntimeError('guard control marker drift: ' + signature)
    return source[:begin] + part.replace(old, new) + source[end:]


# Execute real native objects and state transitions. Installed SDK paths are
# explicit; neither branch imports external Inferno binaries or a second guard.
def main():
    root = Path(__file__).resolve().parents[2]
    installed = not (root / 'disVM/host/object_guard.c').exists()
    host = root / ('source/host' if installed else 'disVM/host')
    tests = root / ('source/tests' if installed else 'disVM/tests')
    include = root / ('include' if installed else 'disVM/include')
    ports = root / ('source/ports' if installed else 'flashVM/ports')
    originals = {name: (host / name).read_text() for name in ('object_guard.c', 'native_context.c', 'native_world.c')}
    early_context = '    if (dvm_native_context_object_count(context))\n    {\n        return 0;\n    }\n'
    early_world = '    if (dvm_native_context_object_count(dvm_native_context_current()))\n    {\n        return 0;\n    }\n'
    controls = {
        'recursive': ('object_guard.c', 'DVM_ObjectResult dvm_object_guard_try_enter(', 'owner_load(guard) == context ||', ''),
        'wait_role': ('object_guard.c', 'DVM_ObjectResult dvm_object_guard_wait_available(', '!can_wait(context)', '(!context || (can_wait(context) && 0))'),
        'poll': ('native_context.c', 'int dvm_native_context_poll(', '!context || context->object_locks', '!context'),
        'unbind': ('native_context.c', 'int dvm_native_context_unbind(', early_context, ''),
        'turn': ('native_context.c', 'int dvm_native_context_release_turn(', early_context, ''),
        'world_leave': ('native_world.c', 'int dvm_world_leave(', early_world, ''),
        'world_enter': ('native_world.c', 'static int enter_world(', early_world, ''),
        'depth': ('native_context.c', 'int dvm_native_context_object_acquire(', '== ULONG_MAX', '== 1'),
    }
    with tempfile.TemporaryDirectory(prefix='native object guards ') as directory:
        work = Path(directory)
        for backend in ('native', 'locked'):
            for control in (None, *controls, 'affinity'):
                sources = dict(originals)
                if control == 'affinity':
                    name = 'object_guard.c'
                    signature = 'int dvm_object_guard_leave('
                    sources[name] = weaken(sources[name], signature, 'owner_load(guard) != context', '0')
                    sources[name] = weaken(sources[name], signature, 'owner_swap(guard, context, NULL)', 'owner_swap(guard, owner_load(guard), NULL)')
                elif control:
                    name, signature, old, new = controls[control]
                    sources[name] = weaken(sources[name], signature, old, new)
                for name, source in sources.items():
                    (work / name).write_text(source)
                command = shlex.split(os.environ.get('CC', 'cc'))
                command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
                command += ['-std=c99', '-Wall', '-Wextra', '-Werror', '-pthread']
                if backend == 'locked':
                    command += ['-DDVM_OBJECT_FORCE_LOCKED=1', '-DDVM_REFCOUNT_FORCE_LOCKED=1']
                command += ['-I' + str(include), '-I' + str(ports), '-I' + str(host)]
                command += [str(tests / 'object_guard.c'), str(work / 'object_guard.c'),
                    str(host / 'refcount.c'), str(work / 'native_context.c'),
                    str(host / 'native_owner.c'), str(work / 'native_world.c'),
                    str(ports / 'linux/thread.c'), str(ports / 'linux/channel_sync.c')]
                binary = work / 'guards'
                built = subprocess.run(command + ['-o', str(binary)], capture_output=True, text=True)
                if built.returncode:
                    raise RuntimeError('guard compile failed: ' + backend + '/' + str(control) + '\n' + built.stdout + built.stderr)
                run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=45)
                if control:
                    if run.returncode != 10 or 'FAIL object guard predicate:' not in run.stderr:
                        raise RuntimeError('guard control missed: ' + backend + '/' + control + run.stdout + run.stderr)
                elif run.returncode or (backend == 'locked' and 'guards: locked,' not in run.stdout):
                    raise RuntimeError(run.stdout + run.stderr)
                else:
                    print(run.stdout, end='')
        print('PASS eighteen guard controls across native/locked backends; no managed multi-mutator activation')


if __name__ == '__main__':
    main()
