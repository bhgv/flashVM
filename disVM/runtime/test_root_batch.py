"""Build actual root metadata and require semantic detection of source controls."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


def main():
    """Test both native publication backends; no fake managed keys reach a GC."""
    runtime = Path(__file__).resolve().parent
    root = runtime.parents[1]
    ports = root / 'flashVM/ports'
    if not ports.exists():
        ports = runtime.parent / 'ports'
    original = (runtime / 'root_batch.c').read_text()
    registry = (runtime / 'root_registry.c').read_text()
    changes = {
        'publish': ('        *head = entry;', '        (void)head;', 'complete publication'),
        'clear': ('    batch->count = count;', '    batch->count = 0;', 'exact batch removal'),
        'remove': ('entry->key == key && !entry->batch', 'entry->key == key', 'ordinary remove preserves batch'),
        'allocate': ('    batch->count = count;', '    void *volatile attempted = malloc(1);\n    free(attempted);\n    batch->count = count;', 'publication allocation attempt'),
    }
    with tempfile.TemporaryDirectory(prefix='native root batches ') as temporary:
        work = Path(temporary)
        for backend in ('atomic', 'locked'):
            variants = [None] if '--positive-only' in sys.argv else [None, *changes]
            for variant in variants:
                code, roots = original, registry
                if variant:
                    before, after, marker = changes[variant]
                    selected = roots if variant == 'remove' else code
                    if selected.count(before) != 1:
                        raise RuntimeError('root batch control drift: '+variant)
                    selected = selected.replace(before, after)
                    if variant == 'remove':
                        roots = selected
                    else:
                        code = selected
                (work / 'root_batch.c').write_text(code)
                (work / 'root_registry.c').write_text(roots)
                command = shlex.split(os.environ.get('CC', 'cc'))
                command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
                command += ['-std=c99', '-Wall', '-Wextra', '-Werror', '-pthread', '-I'+str(runtime), '-I'+str(ports)]
                if backend == 'locked':
                    command += ['-DDVM_ROOT_FORCE_LOCKED=1']
                command += [str(runtime / 'test_root_batch.c'), str(work / 'root_batch.c'),
                            str(work / 'root_registry.c'), str(ports / 'linux/thread.c'),
                            str(ports / 'linux/channel_sync.c'), '-Wl,--wrap=malloc', '-Wl,--wrap=calloc',
                            '-o', str(work / 'batch')]
                built = subprocess.run(command, capture_output=True, text=True)
                if built.returncode:
                    raise RuntimeError('batch compile failed: '+built.stdout+built.stderr)
                for flags in ([['--serial-only']] if variant else [[], ['--serial-only']]):
                    run = subprocess.run([str(work / 'batch'), *flags], capture_output=True, text=True, timeout=45)
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant:
                        if run.returncode != 10 or 'FAIL root batch: '+marker not in output:
                            raise RuntimeError('batch control missed '+variant+': '+output)
                    elif run.returncode:
                        raise RuntimeError('batch positive failed: '+output)
                    else:
                        print(backend+': '+run.stdout, end='')
    print('PASS actual root batches'+(' positives only' if '--positive-only' in sys.argv else '; eight semantic controls'))


if __name__ == '__main__':
    main()
