"""Compile real RC admission and real native worlds; test guard removal semantically."""
from pathlib import Path
import os
import shlex
import signal
import subprocess
import tempfile


# All isolated units borrow current repository headers/ports. Sentinel callbacks
# reject payload access; they do not implement allocation/GC. Control status10
# is required, so compiler failure or a signal cannot count as guard detection.
def main():
    root = Path(__file__).resolve().parents[2]
    runtime = root / 'disVM/runtime'
    source = (runtime / 'runtime_rc.c').read_text()
    controls = {
        'phase': ('(state.phase == DVM_HEAP_ACTIVE || (allow_shared && state.phase == DVM_HEAP_SHARED)) &&', ''),
        'progress': ('!state.pending_progress &&', ''),
        'world': ('dvm_native_owner_world(current) == world', '1'),
    }
    with tempfile.TemporaryDirectory(prefix='RC owner roles ') as directory:
        work = Path(directory)
        for mode in ('normal', *controls):
            unit = source
            if mode != 'normal':
                old, new = controls[mode]
                if unit.count(old) != 1:
                    raise RuntimeError('RC control marker drift: ' + mode)
                unit = unit.replace(old, new)
            path = work / (mode + '.c')
            path.write_text(unit)
            command = shlex.split(os.environ.get('CC', 'cc'))
            command += shlex.split(os.environ.get('CFLAGS', '-O1 -g'))
            command += ['-DDVM_NATIVE_REGISTERS=1', '-pthread']
            command += ['-I' + str(root / name) for name in (
                'disVM/runtime', 'disVM/host/include', 'disVM/upstream/include',
                'disVM/upstream/libinterp', 'disVM/upstream/emu/port',
                'disVM/include', 'flashVM/include', 'flashVM/ports')]
            command += [str(runtime / 'test_rc_roles.c'), str(runtime / 'retirement_queue.c'),
                        str(runtime / 'root_batch.c'), str(runtime / 'root_registry.c'),
                        str(runtime / 'gc/common/graph_storage.c'), str(runtime / 'gc/common/graph_capture.c'),
                        str(runtime / 'gc/duke/snapshot_growth.c'), str(runtime / 'gc/duke/large_graph_pass.c'),
                        str(runtime / 'gc/fast/graph_confirmation.c'),
                        str(runtime / 'gc/fast/packet_budget.c'), str(runtime / 'gc/fast/file_retirement.c'), str(runtime / 'gc/fast/table_retirement.c'), str(path)]
            command += [str(root / name) for name in (
                'disVM/host/refcount.c', 'disVM/host/object_guard.c',
                'disVM/host/cell_access.c', 'disVM/host/range_access.c', 'disVM/host/native_registers.c', 'disVM/host/native_context.c',
                'disVM/host/native_owner.c', 'disVM/host/native_world.c',
                'disVM/host/flashvm_thread.c', 'flashVM/ports/linux/thread.c',
                'flashVM/ports/linux/channel_sync.c')]
            binary = work / mode
            build = subprocess.run(command + ['-o', str(binary)], capture_output=True, text=True)
            if build.returncode:
                raise RuntimeError(build.stdout + build.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=30)
            if mode == 'normal':
                if result.returncode:
                    raise RuntimeError(result.stdout + result.stderr)
                print(result.stdout, end='')
                invalid = subprocess.run([str(binary), '--invalid-drain'], capture_output=True,
                                         text=True, timeout=30)
                if (invalid.returncode != -signal.SIGABRT or
                        'disvm RC retirement outside stopped collector' not in invalid.stderr):
                    raise RuntimeError('wrong-role drain control missed: ' + invalid.stderr)
                print('PASS ACTIVE mutator cannot drain the collector retirement queue')
            elif result.returncode != 10 or 'RC role reached forbidden payload operation' not in result.stderr:
                raise RuntimeError('RC guard control missed: ' + mode + result.stdout + result.stderr)
        print('PASS three active RC role controls: phase, pending progress, world identity')


if __name__ == '__main__':
    main()
