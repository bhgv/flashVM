# Copyright (c) 2026 bhgv
# EN: Require actual native table/key destruction while admitted peers exclude quiescence.
# RU: Требовать освобождение таблиц/ключей, пока допущенные соседи исключают общий покой.
from pathlib import Path
import os
import re
import selectors
import subprocess
import sys
import tempfile
import time

from relink_private import relink
from resident_evidence import evidence


# EN: Confirm both real peers are admitted before allowing native owner production.
# Own child/pipes through exit. binary/image/work/mode/locked are borrowed inputs;
# return copied output or raise on timeout, protocol failure or VM failure.
# RU: Подтвердить допуск двух соседей до разрешения создания владельцев.
# Владеть процессом/каналами до выхода; входы заимствованы. Вернуть копию вывода
# либо вызвать исключение при таймауте, ошибке протокола или VM.
def execute(binary, image, work, mode, locked):
    process = subprocess.Popen([str(binary), '--native-spawn', '--gc-no-cycles', '--stats', mode,
                                *(['--gc-locked-poll'] if locked else []), str(image)],
                               cwd=work, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
    output = bytearray()
    errors = bytearray()
    try:
        with selectors.DefaultSelector() as poll:
            poll.register(process.stdout, selectors.EVENT_READ, output)
            poll.register(process.stderr, selectors.EVENT_READ, errors)
            deadline = time.monotonic() + 30
            while b'TABLE_RETIRE_PEERS_ADMITTED\n' not in errors:
                if time.monotonic() >= deadline or not poll.get_map():
                    raise RuntimeError('admitted peer protocol failed\n' + (output + errors).decode(errors='replace')[-12000:])
                for key, unused in poll.select(1):
                    chunk = os.read(key.fileobj.fileno(), 4096)
                    if chunk:
                        key.data.extend(chunk)
                    else:
                        poll.unregister(key.fileobj)
        after, diagnostics = process.communicate(input=b'1', timeout=30)
        output.extend(after)
        errors.extend(diagnostics)
    except BaseException:
        process.kill()
        process.communicate()
        raise
    text = (output + errors).decode(errors='replace')
    if process.returncode or 'PASS native table retirement tables=512 keys=256 anchors=2 owners=6' not in text:
        raise RuntimeError(f'{mode} locked={locked}\n{text[-12000:]}')
    return text


# EN: Compile real VM bytecode and a private native admission relay for both modes.
# CLI supplies runtime/compiler/includes; own temporary files until all children exit.
# RU: Собрать байткод VM и частную эстафету допуска для обоих режимов.
# CLI задаёт runtime/compiler/includes; владеть временными файлами до выхода детей.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix='shared table retirement ') as temporary:
        work = Path(temporary)
        image = work / 'table-retirement.dis'
        subprocess.run([str(compiler), '-I', str(include), '-I', str(root.parent / 'module'),
                        '-o', str(image), str(root / 'fixtures/resident-table-retirement/main.b')],
                       check=True, capture_output=True, text=True)
        binary = relink(runtime, work, Path(__file__).with_suffix('.c'), 0,
                        ['dvm_runtime_guest_sleep', 'Sys_werrstr'])
        for mode in ('--no-jit', '--jit'):
            for locked in (False, True):
                text = execute(binary, image, work, mode, locked)
                evidence(text, children=6)
                if 'PASS shared native table retirement tables=2 keys=2 admitted=2' not in text:
                    raise RuntimeError('shared retirement was not observed\n' + text[-12000:])
                native = re.search(r'NATIVETABLERC tables=(\d+) keys=(\d+) with_peer=(\d+)', text)
                if not native or int(native[1]) < 512 or int(native[2]) < 256 or int(native[3]) < 768:
                    raise RuntimeError('retirement was not executed by shared Fast with active peers\n' + text[-12000:])
                if (not re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0', text)
                        or 'tables_live=0 keys_live=0' not in text):
                    raise RuntimeError('native owner retirement did not drain\n' + text[-12000:])
    print('PASS shared native table retirement: 2048 dead tables, 1024 keys, eight table/key anchor pairs, '
          'C/JIT, both polls, active peers, complete native/RC retirement')


if __name__ == '__main__':
    main()
