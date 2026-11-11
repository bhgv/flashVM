# Copyright (c) 2026 bhgv
# EN: Real mandatory RC publication with successful drain and fatal root OOM.
# RU: Настоящая обязательная передача RC с очисткой очередей и fatal OOM корня.
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

from relink_private import relink


# EN: Compile a real guest and audit its producer's mandatory GC publication.
# CLI supplies runtime/compiler/includes; optional --positive-only skips the fault.
# Own private artifacts and children through exit. Compiler failure, unrelated
# abort and timeout never count as successful OOM detection.
# RU: Собрать настоящую программу и проверить обязательную передачу в GC.
# CLI задаёт runtime/compiler/includes; --positive-only отключает отказ.
# Владеть частными файлами и процессами до выхода. Ошибка сборки, посторонний
# abort и таймаут не считаются обнаружением OOM.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:]
                                if value != '--positive-only')
    with tempfile.TemporaryDirectory(prefix='mandatory GC publication ') as temporary:
        work = Path(temporary)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text('implement Probe; include "sys.m"; Probe: module {init: fn();};\n'
                         'init() {s := load Sys Sys->PATH; s->print("PASS ordinary guest\\n");}\n')
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)],
                       capture_output=True, text=True, check=True)
        for fault in ((0,) if '--positive-only' in sys.argv else (0, 1)):
            binary = relink(runtime, work, Path(__file__).with_suffix('.c'), fault,
                            ['dvm_runtime_rc_defer', 'dvm_runtime_root_add', 'abort'])
            for mode in ('--jit', '--no-jit'):
                for locked in (False, True):
                    command = [str(binary), mode, '--gc-worker', '--managed-owner-probe', '--stats']
                    if locked:
                        command.append('--gc-locked-poll')
                    result = subprocess.run(command + [str(image)], capture_output=True, text=True,
                                            timeout=45, env=dict(os.environ, TMPDIR=str(work)))
                    text = result.stdout + result.stderr
                    if 'WARNING: ThreadSanitizer:' in text or 'runtime error:' in text:
                        raise RuntimeError(text)
                    if fault:
                        required = ('INJECT mandatory GC root allocation failure',
                                    'out of memory rooting mandatory GC release',
                                    'PASS mandatory GC root OOM terminated publication')
                        if result.returncode != 73 or any(marker not in text for marker in required):
                            raise RuntimeError('mandatory OOM did not terminate publication\n' + text)
                    elif (result.returncode or 'PASS ordinary guest' not in text
                          or not re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0', text)
                          or not re.search(r'NATIVEGC .*joined=1 failed=0', text)):
                        raise RuntimeError('mandatory GC publication did not drain\n' + text)
    print('PASS mandatory GC publication: C/JIT, both polls, real RC drain and fatal root OOM')


if __name__ == '__main__':
    main()
