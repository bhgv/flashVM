"""Build actual slot/ledger/port sources, including semantic ownership controls."""
from pathlib import Path
import argparse
import os
import shlex
import subprocess
import tempfile


# Build isolated actual production sources, not a replacement evaluator. Negative
# mutations must reach exact semantic status10; compiler/signal/timeouts refuse.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--positive-only', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    installed = not (root / 'disVM/host/owner_slot.c').exists()
    host = root / ('source/host' if installed else 'disVM/host')
    tests = root / ('source/tests' if installed else 'disVM/tests')
    include = root / ('include' if installed else 'disVM/include')
    ports = root / ('source/ports' if installed else 'flashVM/ports')
    mutations = (
        ('replace', 'owner_scope.c', '*transfer = previous;', '(void)previous;'),
        ('replace_entry', 'owner_scope.c', 'entry->object = transfer->object;', '(void)transfer;'),
        ('put', 'owner_slot.c', 'moved = dvm_owner_scope_export(entry, &slot->owner);', 'moved = 1; (void)entry;'),
        ('take', 'owner_slot.c', 'moved = dvm_owner_scope_accept(scope, entry, &slot->owner);', 'moved = 1; (void)scope; (void)entry;'),
        ('publish', 'owner_slot.c', 'slot->owner = *transfer;', '(void)transfer;'),
        ('move', 'owner_slot.c', '*transfer = previous;', '(void)previous;'),
        ('retain', 'owner_scope.c', 'retained = ops->retain(object);', 'retained = 1;'),
    )
    with tempfile.TemporaryDirectory(prefix='native owner slots ') as directory:
        work = Path(directory)
        for backend in ('native', 'locked'):
            for mutation in (None, *(() if args.positive_only else mutations)):
                for name in ('owner_slot.c', 'owner_scope.c'):
                    source = (host / name).read_text()
                    if mutation and mutation[1] == name:
                        if source.count(mutation[2]) != 1:
                            raise RuntimeError('owner slot marker drift')
                        source = source.replace(mutation[2], mutation[3], 1)
                    (work / name).write_text(source)
                command = shlex.split(os.environ.get('CC', 'cc'))
                command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
                command += ['-std=c99', '-Wall', '-Wextra', '-Werror', '-pthread']
                if backend == 'locked':
                    command += ['-DDVM_REFCOUNT_FORCE_LOCKED=1', '-DDVM_OBJECT_FORCE_LOCKED=1']
                command += ['-I'+str(include), '-I'+str(host), '-I'+str(ports)]
                command += [str(tests / 'owner_slot.c'), str(work / 'owner_slot.c'), str(work / 'owner_scope.c')]
                command += [str(host / name) for name in ('native_context.c', 'refcount.c', 'object_guard.c', 'native_owner.c', 'native_world.c')]
                command += [str(ports / 'linux/thread.c'), str(ports / 'linux/channel_sync.c'), '-o', str(work / 'slots')]
                build = subprocess.run(command, capture_output=True, text=True)
                if build.returncode:
                    raise RuntimeError('slot compile failure: '+build.stdout+build.stderr)
                run = subprocess.run([str(work / 'slots')], capture_output=True, text=True, timeout=45)
                if 'WARNING: ThreadSanitizer:' in run.stderr or 'runtime error:' in run.stderr:
                    raise RuntimeError('slot sanitizer failure: '+run.stdout+run.stderr)
                if mutation:
                    if run.returncode != 10 or 'FAIL owner slot predicate:' not in run.stderr:
                        raise RuntimeError('missed semantic slot control: '+str(mutation)+run.stdout+run.stderr)
                elif run.returncode:
                    raise RuntimeError(run.stdout+run.stderr)
                else:
                    print(backend+': '+run.stdout, end='')
            # Rebuild unmodified sources for the independent exclusive-move run.
            for name in ('owner_slot.c', 'owner_scope.c'):
                (work / name).write_text((host / name).read_text())
            command = [str(tests / 'owner_mailbox.c') if value == str(tests / 'owner_slot.c') else value for value in command]
            built = subprocess.run(command, capture_output=True, text=True)
            if built.returncode:
                raise RuntimeError('mailbox compile failure: '+built.stdout+built.stderr)
            run = subprocess.run([str(work / 'slots')], capture_output=True, text=True, timeout=45)
            if run.returncode or 'WARNING: ThreadSanitizer:' in run.stderr or 'runtime error:' in run.stderr:
                raise RuntimeError('mailbox execution failure: '+run.stdout+run.stderr)
            print(backend+': '+run.stdout, end='')
    print('PASS slot checks; '+('joined positives only' if args.positive_only else 'fourteen exact semantic controls'))


if __name__ == '__main__':
    main()
