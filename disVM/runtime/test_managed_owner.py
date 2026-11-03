"""Run real managed roots, native owner scopes and Fast retirement in full runtime."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile


# runtime/compiler/include/font are explicit products. Compile a small normal
# Limbo application, run optional real-heap probe in C/JIT and both poll modes,
# then check exact ticket/root retirement and normal process completion. No
# replacement heap, elapsed-time proof or native pointer crosses a VM boundary.
def main():
    runtime, compiler, include, font = (Path(value).resolve() for value in sys.argv[1:])
    with tempfile.TemporaryDirectory(prefix='managed rooted owners ') as directory:
        work = Path(directory)
        source = work / 'main.b'
        image = work / 'main.dis'
        source.write_text('''implement TicketProbe;
include "sys.m";
TicketProbe: module {init: fn();};
# The host probe runs before init; this normal application then proves that
# the caught native error did not contaminate its process environment.
init() {
    sys := load Sys Sys->PATH;
    sys->print("PASS normal Dis after native managed cleanup\\n");
}
''')
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(source)],
                       check=True, capture_output=True, text=True)
        options = ['--font', str(font)] if runtime.name == 'disvm-editor' else []
        env = dict(os.environ, TMPDIR=str(work), SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
        for mode in ('--jit', '--no-jit'):
            for locked in (False, True):
                arguments = [str(runtime), *options, mode, '--gc-worker', '--managed-owner-probe', '--stats']
                if locked:
                    arguments += ['--gc-locked-poll']
                result = subprocess.run(arguments + [str(image)], capture_output=True, text=True, timeout=45, env=env)
                text = result.stdout + result.stderr
                if 'WARNING: ThreadSanitizer:' in result.stderr or 'runtime error:' in result.stderr:
                    raise RuntimeError('managed root sanitizer failure: ' + text[-6000:])
                if result.returncode or 'PASS managed owner probe:' not in text or 'PASS Type-count probe:' not in text or 'PASS normal Dis after native managed cleanup' not in text:
                    raise RuntimeError(text[-6000:])
                if not re.search(r'MANAGEDOWNERS captured=2 released=2 live=0 pending=0 tickets=0 producers=0 failed=0 rooted_until_fast=1', text):
                    raise RuntimeError('ticket/root retirement mismatch: ' + text[-6000:])
                if not re.search(r'NATIVEGC .*joined=1 failed=0', text):
                    raise RuntimeError('collector lifecycle mismatch: ' + text[-6000:])
        refused = subprocess.run([str(runtime), *options, '--gc-inline', '--managed-owner-probe', str(image)],
                                 capture_output=True, text=True, timeout=15, env=env)
        if refused.returncode != 2 or '--managed-owner-probe requires --gc-worker' not in refused.stderr:
            raise RuntimeError('missing inline refusal: ' + refused.stdout + refused.stderr)
    print('PASS managed roots: C/JIT, atomic/locked polling, 8 native receivers, 32 retained cycles, native error/Fast retirement')


if __name__ == '__main__':
    main()
