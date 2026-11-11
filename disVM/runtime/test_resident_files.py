# Copyright (c) 2026 bhgv
# EN: Verify concurrent slot rollback/publication, actual I/O and native retirement.
# RU: Проверить параллельные откат/публикацию слотов, I/O и нативное завершение.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence


# EN: Compile real bytecode and a private scheduling barrier, then run both paths.
# CLI supplies runtime/compiler/includes. Own temporary files and all subprocesses;
# require unique retained slots, exact bytes, drained roots/handles and fixed owners.
# RU: Собрать байткод и частный барьер исполнения, проверить оба пути.
# CLI задаёт runtime/compiler/includes. Владеть временными файлами и процессами;
# требовать уникальные слоты, точные байты, очистку корней/handles и постоянство Prog.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    source = Path(__file__).parent / 'fixtures/resident-files/main.b'
    with tempfile.TemporaryDirectory(prefix='resident files ') as temporary:
        work = Path(temporary)
        image = work / 'files.dis'
        barrier = relink(runtime, work, Path(__file__).with_name('test_file_slot_barrier.c'), 0,
                         ['dvm_file_gate_create'])
        for id in range(1, 16, 2):
            (work / f'dir-{id}').mkdir()
        for profile in range(3):
            selected = work / 'selected.b'
            selected.write_text(source.read_text().replace('ReadMode: con 0;', f'ReadMode: con {profile};'))
            subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(selected)],
                           check=True, capture_output=True, text=True)
            for binary in (runtime, barrier):
                for mode in ('--no-jit', '--jit'):
                    for locked in (False, True):
                        result = subprocess.run([str(binary), '--native-spawn', mode, '--stats',
                                                 *(['--gc-locked-poll'] if locked else []), str(image)],
                                                capture_output=True, text=True, cwd=work, timeout=45)
                        text = result.stdout + result.stderr
                        if result.returncode or 'PASS resident files owners=16 files=8 directories=8 rollback=16' not in text:
                            raise RuntimeError(f'{binary.name} {mode} locked={locked}\n{text[-10000:]}')
                        evidence(text, children=16)
                        if not re.search(r'NATIVEFILES created=(\d+) closed=\1 live=0 .*pending=0', text):
                            raise RuntimeError('native gates did not retire\n' + text[-10000:])
                        if not re.search(r'NATIVECLOSE submitted=(\d+) completed=\1 pending=0 .*joined=1', text):
                            raise RuntimeError('detached native handles did not close\n' + text[-10000:])
                        for label in ('NATIVEOPEN', 'NATIVEMETADATA', 'NATIVEOUTPUT', 'NATIVEINPUT'):
                            line = next((line for line in text.splitlines() if line.startswith(label + ' ')), '')
                            if 'staging=0 ' not in line or (label != 'NATIVEOPEN' and 'pins=0 ' not in line):
                                raise RuntimeError('native accounting did not drain: ' + line)
                        if not re.search(r'NATIVEOPEN attempts=\d+ releases=45 staging=0 ', text):
                            raise RuntimeError('concurrent open accounting lost an attempt\n' + text[-10000:])
                        if not re.search(r'NATIVEMETADATA paths=0 descriptors=16 releases=16 pins=0 staging=0 ', text):
                            raise RuntimeError('metadata accounting lost a completion\n' + text[-10000:])
                        for label, active in [('NATIVEOUTPUT', profile == 0), ('NATIVEINPUT', profile == 1)]:
                            expected = 'fragments=8 copied=512' if active else 'fragments=0 copied=0'
                            if not re.search(label + r' pins=0 staging=0 peak=\d+ ' + expected, text):
                                raise RuntimeError('I/O accounting lost a fragment\n' + text[-10000:])
                        expected = 9 if profile == 2 else 1
                        if not re.search(f'NATIVEFORMATTED pins=0 calls={expected} releases={expected}', text):
                            raise RuntimeError('formatted accounting lost a completion\n' + text[-10000:])
                        if (work / 'quota-must-not-create').exists():
                            raise RuntimeError('quota refusal performed a destructive native open')
                        for id in range(0, 16, 2):
                            if (work / f'file-{id}').read_bytes() != bytes([65 + id]) * 64:
                                raise RuntimeError(f'private file {id} was overwritten by another resident')
    print('PASS resident files: C/JIT, both GC polls, normal/aligned opens, 384 owners, files/directories, read/write/format, rollback, quota and retirement')


if __name__ == '__main__':
    main()
