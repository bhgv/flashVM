"""Check capture admission using real worlds and semantic root-operation sentinels."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


# Compile actual adapter/TLS/world/collector services. No arguments/result; no
# managed heap is simulated. Controls require explicit semantic status10, never
# compiler, signal, sanitizer or timeout as proof of refusal detection.
def main():
    positive_only = sys.argv[1:] == ['--positive-only']
    if sys.argv[1:] and not positive_only:
        raise RuntimeError('expected optional --positive-only')
    root = Path(__file__).resolve().parents[2]
    runtime = root / 'disVM/runtime'
    source = (runtime / 'managed_owner.c').read_text()
    controls = {
        'phase': ('state.phase != DVM_HEAP_ACTIVE || ', ''),
        'progress': ('state.pending_progress ||', '0 ||'),
        'world': ('dvm_native_owner_world(owner) == owners.world', '1'),
    }
    with tempfile.TemporaryDirectory(prefix='managed owner roles ') as directory:
        work = Path(directory)
        for name in (('normal',) if positive_only else ('normal', *controls)):
            unit = source
            if name != 'normal':
                old, new = controls[name]
                begin = unit.index('static int admitted(void)')
                end = unit.index('\n}\n', begin) + 3
                part = unit[begin:end]
                if part.count(old) != 1:
                    raise RuntimeError('managed owner control marker drift: ' + name)
                unit = unit[:begin] + part.replace(old, new, 1) + unit[end:]
            path = work / 'managed_owner.c'
            path.write_text(unit)
            command = shlex.split(os.environ.get('CC', 'cc'))
            command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
            command += ['-std=c99', '-DDVM_NATIVE_REGISTERS=1', '-pthread']
            command += ['-I' + str(root / p) for p in ('disVM/runtime', 'disVM/host/include',
                'disVM/upstream/include', 'disVM/upstream/libinterp', 'disVM/upstream/emu/port',
                'disVM/include', 'flashVM/ports')]
            command += [str(runtime / 'test_managed_owner_roles.c'), str(path)]
            command += [str(root / p) for p in ('disVM/host/refcount.c', 'disVM/host/guard_registry.c', 'disVM/host/object_guard.c', 'disVM/host/cell_access.c', 'disVM/host/range_access.c', 'disVM/host/small_range.c', 'disVM/host/owner_scope.c', 'disVM/host/native_context.c',
                'disVM/host/native_registers.c', 'disVM/host/native_owner.c', 'disVM/host/native_world.c',
                'disVM/host/gc_workers.c', 'flashVM/ports/linux/thread.c', 'flashVM/ports/linux/channel_sync.c')]
            binary = work / 'roles'
            built = subprocess.run(command + ['-o', str(binary)], capture_output=True, text=True)
            if built.returncode:
                raise RuntimeError(built.stdout + built.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=30)
            if 'WARNING: ThreadSanitizer:' in result.stderr or 'runtime error:' in result.stderr:
                raise RuntimeError('managed owner role sanitizer failure: ' + result.stdout + result.stderr)
            if name == 'normal':
                if result.returncode:
                    raise RuntimeError(result.stdout + result.stderr)
                print(result.stdout, end='')
            elif result.returncode != 10 or 'managed owner role reached forbidden root/payload operation' not in result.stderr:
                raise RuntimeError('managed owner control missed: ' + name + result.stdout + result.stderr)
        if positive_only:
            print('PASS joined managed role fixture; fatal controls excluded')
        else:
            print('PASS three managed capture role controls: phase, pending reentry, world identity')


if __name__ == '__main__':
    main()
