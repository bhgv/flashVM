"""Compile the production per-file gate and active semantic removal controls."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


# Compile actual gate/OS ports/context code, with no alternate mutex scheduler.
# Controls alter private temporary copies only; compiler/signal/timeout failures
# never count as successful semantic refusal. No parameters; raise on failure.
def main():
    root = Path(__file__).resolve().parents[2]
    runtime = Path(__file__).resolve().parent
    source = (runtime / 'file_gate.c').read_text()
    controls = [
        ('fifo', '    while (ticket != gate->serving || gate->busy)\n    {\n'
         '        fvm_port_sync_wait(gate->sync);\n    }', '',
         'FIFO file admission order violated'),
        ('owner', ' || gate->owner != dvm_native_context_current()', '',
         'foreign file completion accepted'),
        ('lifetime', 'gate->busy || gate->next != gate->serving', '0',
         'busy file gate retired'),
    ]
    with tempfile.TemporaryDirectory(prefix='native file FIFO ') as directory:
        work = Path(directory)
        base = shlex.split(os.environ.get('CC', 'cc'))
        base += shlex.split(os.environ.get('CFLAGS', '-O2 -g -fsanitize=undefined -fno-sanitize-recover=all'))
        base += ['-std=c99', '-pthread', '-I' + str(runtime), '-I' + str(root / 'disVM/include'),
                 '-I' + str(root / 'flashVM/ports')]
        sources = [runtime / 'test_file_gate.c', runtime / 'file_gate.c',
                   root / 'disVM/host/native_context.c', root / 'flashVM/ports/linux/thread.c',
                   root / 'flashVM/ports/linux/channel_sync.c']
        for name, before, after, marker in [('normal', None, None, None)] + controls:
            selected = list(sources)
            if before is not None:
                if source.count(before) != 1:
                    raise RuntimeError('file gate control target changed: ' + name)
                changed = work / (name + '.c')
                changed.write_text(source.replace(before, after))
                selected[1] = changed
            output = work / name
            subprocess.run(base + [str(path) for path in selected] + ['-o', str(output)], check=True)
            run = subprocess.run([str(output)], capture_output=True, text=True, timeout=20)
            if name == 'normal':
                if run.returncode or 'PASS native file FIFO:' not in run.stdout:
                    raise RuntimeError(run.stdout + run.stderr)
                print(run.stdout.strip())
            elif run.returncode != 1 or marker not in run.stderr:
                raise RuntimeError('inactive semantic file gate control: ' + name + run.stdout + run.stderr)
        print('PASS three active file gate controls: FIFO, exact native owner, pending lifetime')


if __name__ == '__main__':
    main()
